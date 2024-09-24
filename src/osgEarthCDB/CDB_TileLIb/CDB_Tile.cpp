// 2019-2024 GAJ Geospatial Enterprises, Orlando FL
// This file is based on the Common Database (CDB) Specification for USSOCOM
// Version 3.0 � October 2008 and OGC CDB standard 1.0 - 1.2
//
// Copyright (c) 2016-2017 Visual Awareness Technologies and Consulting Inc, St Petersburg FL

// CDB_Tile is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// CDB_Tile is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with CDB_Tile.  If not, see <http://www.gnu.org/licenses/>.

// 2015 GAJ Geospatial Enterprises, Orlando FL
// Modified for General Incorporation of Common Database (CDB) support within osgEarth
//
#include "CDB_Tile"
#include <osgEarth/XmlUtils>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

#ifdef _WIN32
#include <Windows.h>
#endif

#define GEOTRSFRM_TOPLEFT_X            0
#define GEOTRSFRM_WE_RES               1
#define GEOTRSFRM_ROTATION_PARAM1      2
#define GEOTRSFRM_TOPLEFT_Y            3
#define GEOTRSFRM_ROTATION_PARAM2      4
#define GEOTRSFRM_NS_RES               5

#define JP2DRIVERCNT 5

osgEarth::CDBTile::CDB_GDAL_Drivers Gbl_TileDrivers;


const int Gbl_CDB_Tile_Sizes[11] = {1024, 512, 256, 128, 64, 32, 16, 8, 4, 2, 1};
//Caution this only goes down to CDB Level 17
const double Gbl_CDB_Tiles_Per_LOD[18] = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0, 256.0, 512.0, 1024.0, 2048.0, 4096.0, 8192.0, 16384.0, 32768.0, 65536.0, 131072.0};

osgEarth::CDBTile::OGR_File  Ogr_File_Instance;
osgEarth::CDBTile::CDB_Data_Dictionary  CDB_Data_Dictionary_Instance;


osgEarth::CDBTile::CDB_Tile::CDB_Tile(std::string cdbRootDir, std::string cdbCacheDir, CDB_Tile_Type TileType, std::string dataset, CDB_Tile_Extent *TileExtent, bool lightmap, bool material, bool material_mask, int NLod, bool DataFromGlobal) :
	               m_cdbRootDir(cdbRootDir), m_cdbCacheDir(cdbCacheDir),
				   m_DataSet(dataset), m_TileExtent(*TileExtent), m_TileType(TileType), m_ImageContent_Status(NotSet), m_Tile_Status(Created), m_FileName(""), m_LayerName(""), m_FileExists(false),
				   m_CDB_LOD_Num(0), m_Subordinate_Exists(false), m_SubordinateName(""), m_lat_str(""), m_lon_str(""), m_lod_str(""), m_uref_str(""), m_rref_str(""), m_Subordinate_Tile(false),
				   m_Use_Spatial_Rect(false), m_SubordinateName2(""), m_Subordinate2_Exists(false), m_Have_MaterialMaskData(false), m_Have_MaterialData(false), m_EnableLightMap(lightmap),
				   m_EnableMaterials(material), m_EnableMaterialMask(material_mask), m_DataFromGlobal(DataFromGlobal), m_GlobalDataset(NULL), m_HaveDataDictionary(false), m_GlobalTile(NULL),
				   m_Globalcontype(ConnNone)
{
	m_GTModelSet.clear();
	CDB_Global * gbls = CDB_Global::getInstance();
	if (NLod > 0)
	{
		m_Pixels.pixX = Gbl_CDB_Tile_Sizes[NLod];
		m_Pixels.pixY = Gbl_CDB_Tile_Sizes[NLod]; 
		m_CDB_LOD_Num = -NLod;
	}

	std::stringstream	buf;
	std::stringstream	subordinatebuf;
	std::stringstream	subordinatebuf2;

	m_CDB_LOD_Num = GetPathComponents(m_lat_str, m_lon_str, m_lod_str, m_uref_str, m_rref_str);

	std::string filetype;
	std::string sub2filetype;
	std::string datasetstr;
	std::string subordinatedatasetstr;
	std::string subordinatedatasetstr2;
	std::string subord2LayerName;

	if (m_DataFromGlobal)
	{
		double clat;
		double clon;
		m_TileExtent.Center(clat, clon);
		m_Globalcontype = gbls->Get_ConnectionType();
		if (m_Globalcontype == ConnWFS)
		{
			m_GlobalDataset = gbls->Get_Dataset();
			m_GlobalTile = gbls->FindVectorTile(clat, clon);
		}
		else
		{
			m_GlobalTile = gbls->FindVectorTile(clat, clon);
			if (m_GlobalTile)
			{
				m_GlobalDataset = m_GlobalTile->Get_Dataset();
				m_Globalcontype = m_GlobalTile->GetConType();
			}
			else
				m_GlobalDataset = NULL;
		}
	}

	CDB_Data_Dictionary* datDict = CDB_Data_Dictionary::GetInstance();
	if (datDict->Init_Feature_Data_Dictionary(m_cdbRootDir))
		m_HaveDataDictionary = true;
	

	if (m_TileType == Elevation)
	{
		m_LayerName = "001_Elevation";
		if (m_CDB_LOD_Num < 0 && NLod == 0)
		{
			m_TileType = ElevationCache;
			filetype = ".img";
		}
		else
		{
			filetype = ".tif";
		}

		datasetstr = "_D001" + m_DataSet;
		subordinatedatasetstr = "_D001_S100_T001_";
		m_Pixels.pixType = GDT_Float32;
		m_Pixels.bands = 1;
	}
	else if (m_TileType == Imagery)
	{
		m_LayerName = "004_Imagery";
		subord2LayerName = "005_RMTexture";
		if (m_CDB_LOD_Num < 0 && NLod == 0)
		{
			m_TileType = ImageryCache;
			filetype = ".tif";
		}
		else
		{
			filetype = ".jp2";
		}
		datasetstr = "_D004" + m_DataSet;
		subordinatedatasetstr = "_D004_S005_T001_";
		subordinatedatasetstr2 = "_D005_S001_T001_";
		sub2filetype = ".tif";
	}
	else if (m_TileType == GeoPackageMap)
	{
		m_LayerName = "901_VectorBase";
		if (m_CDB_LOD_Num < gbls->BaseMapLodNum())
			filetype = ".xml";
		else
			filetype = ".gpkg";
		datasetstr = "_D901" + m_DataSet;
	}
	else if (m_TileType == GeoSpecificModel)
	{
		m_LayerName = "100_GSFeature";	
		if(gbls->Get_Use_GeoPackage_Features())
			filetype = ".gpkg";
		else
			filetype = ".shp";
		datasetstr = "_D100" + m_DataSet;
	}
	else if (m_TileType == GeoTypicalModel)
	{
		m_LayerName = "101_GTFeature";	
		if (gbls->Get_Use_GeoPackage_Features())
			filetype = ".gpkg";
		else
			filetype = ".shp";
		datasetstr = "_D101" + m_DataSet;
	}
	else
	{
		m_TileType = CDB_Unknown;
		filetype = ".unk";
		datasetstr = "_DUNK_SUNK_TUNK_";
	}

	//Set tile size for lower levels of detail

	if (!m_DataFromGlobal)
	{
		if (m_CDB_LOD_Num < 0)
		{
			if (NLod == 0)
			{
				buf << cdbCacheDir
					<< "\\" << m_LayerName
					<< "\\" << m_lat_str << m_lon_str << datasetstr << m_lod_str
					<< "_" << m_uref_str << "_" << m_rref_str << filetype;

				if ((m_TileType == Elevation) || (m_TileType == ElevationCache) || (m_TileType == Imagery))
				{
					subordinatebuf << cdbRootDir
						<< cdbCacheDir
						<< "\\" << m_LayerName
						<< "\\" << m_lat_str << m_lon_str << subordinatedatasetstr << m_lod_str
						<< "_" << m_uref_str << "_" << m_rref_str << filetype;
				}
				if (m_TileType == Imagery)
				{
					subordinatebuf2 << cdbRootDir
						<< cdbCacheDir
						<< "\\" << subord2LayerName
						<< "\\" << m_lat_str << m_lon_str << subordinatedatasetstr2 << m_lod_str
						<< "_" << m_uref_str << "_" << m_rref_str << sub2filetype;
				}
			}
			else
			{
				buf << cdbRootDir
					<< "\\Tiles"
					<< "\\" << m_lat_str
					<< "\\" << m_lon_str
					<< "\\" << m_LayerName
					<< "\\LC"
					<< "\\" << m_uref_str
					<< "\\" << m_lat_str << m_lon_str << datasetstr << m_lod_str
					<< "_" << m_uref_str << "_" << m_rref_str << filetype;
				if ((m_TileType == Elevation) || (m_TileType == ElevationCache) || (m_TileType == Imagery))
				{
					subordinatebuf << cdbRootDir
						<< "\\Tiles"
						<< "\\" << m_lat_str
						<< "\\" << m_lon_str
						<< "\\" << m_LayerName
						<< "\\LC"
						<< "\\" << m_uref_str
						<< "\\" << m_lat_str << m_lon_str << subordinatedatasetstr << m_lod_str
						<< "_" << m_uref_str << "_" << m_rref_str << filetype;

				}
				if (m_TileType == Imagery)
				{
					subordinatebuf2 << cdbRootDir
						<< "\\Tiles"
						<< "\\" << m_lat_str
						<< "\\" << m_lon_str
						<< "\\" << subord2LayerName
						<< "\\LC"
						<< "\\" << m_uref_str
						<< "\\" << m_lat_str << m_lon_str << subordinatedatasetstr2 << m_lod_str
						<< "_" << m_uref_str << "_" << m_rref_str << sub2filetype;

				}
			}
		}
		else
		{
			buf << cdbRootDir
				<< "\\Tiles"
				<< "\\" << m_lat_str
				<< "\\" << m_lon_str
				<< "\\" << m_LayerName
				<< "\\" << m_lod_str
				<< "\\" << m_uref_str
				<< "\\" << m_lat_str << m_lon_str << datasetstr << m_lod_str
				<< "_" << m_uref_str << "_" << m_rref_str << filetype;

			if ((m_TileType == Elevation) || (m_TileType == ElevationCache) || (m_TileType == Imagery))
			{
				subordinatebuf << cdbRootDir
					<< "\\Tiles"
					<< "\\" << m_lat_str
					<< "\\" << m_lon_str
					<< "\\" << m_LayerName
					<< "\\" << m_lod_str
					<< "\\" << m_uref_str
					<< "\\" << m_lat_str << m_lon_str << subordinatedatasetstr << m_lod_str
					<< "_" << m_uref_str << "_" << m_rref_str << filetype;
			}
			if (m_TileType == Imagery)
			{
				subordinatebuf2 << cdbRootDir
					<< "\\Tiles"
					<< "\\" << m_lat_str
					<< "\\" << m_lon_str
					<< "\\" << subord2LayerName
					<< "\\" << m_lod_str
					<< "\\" << m_uref_str
					<< "\\" << m_lat_str << m_lon_str << subordinatedatasetstr2 << m_lod_str
					<< "_" << m_uref_str << "_" << m_rref_str << sub2filetype;
			}
		}
	}
	else
	{
		buf << m_LayerName << datasetstr.substr(5) << "Pnt_" << m_lod_str; //Global Layer Name
	}
	m_FileName = buf.str();

	if (m_TileType == GeoSpecificModel)
	{
		std::string dataset2str = "_D100_S001_T002_";
		std::string filetype2str = ".dbf";
		std::stringstream dbfbuf;
		if (!m_DataFromGlobal)
		{
			dbfbuf << cdbRootDir
				<< "\\Tiles"
				<< "\\" << m_lat_str
				<< "\\" << m_lon_str
				<< "\\" << m_LayerName
				<< "\\" << m_lod_str
				<< "\\" << m_uref_str
				<< "\\" << m_lat_str << m_lon_str << dataset2str << m_lod_str
				<< "_" << m_uref_str << "_" << m_rref_str << filetype2str;
		}
		else
		{
			dbfbuf << m_LayerName << dataset2str.substr(5) << "Cls_" << m_lod_str;
		}
		CDB_Model_Tile_Set ModelSet;

		ModelSet.ModelDbfName = dbfbuf.str();

		dataset2str = "_D300_S001_T001_";
		filetype2str = ".zip";
		std::string layerName2 = "300_GSModelGeometry";
		std::stringstream geombuf;
		if (!m_DataFromGlobal)
		{
			geombuf << cdbRootDir
				<< "\\Tiles"
				<< "\\" << m_lat_str
				<< "\\" << m_lon_str
				<< "\\" << layerName2
				<< "\\" << m_lod_str
				<< "\\" << m_uref_str
				<< "\\" << m_lat_str << m_lon_str << dataset2str << m_lod_str
				<< "_" << m_uref_str << "_" << m_rref_str << filetype2str;
		}
		else
		{
			geombuf << layerName2 << dataset2str.substr(5) << "Mda_" << m_lod_str;
		}
		ModelSet.ModelGeometryName = geombuf.str();

		dataset2str = "_D301_S001_T001_";
		filetype2str = ".zip";
		layerName2 = "301_GSModelTexture";
		std::stringstream txbuf;
		if (!m_DataFromGlobal)
		{
			txbuf << cdbRootDir
				<< "\\Tiles"
				<< "\\" << m_lat_str
				<< "\\" << m_lon_str
				<< "\\" << layerName2
				<< "\\" << m_lod_str
				<< "\\" << m_uref_str
				<< "\\" << m_lat_str << m_lon_str << dataset2str << m_lod_str
				<< "_" << m_uref_str << "_" << m_rref_str << filetype2str;
		}
		else
		{
			txbuf << layerName2 << dataset2str.substr(5) << "Mda_" << m_lod_str;
		}

		ModelSet.ModelTextureName = txbuf.str();

		int Tnum = 1;
		int i = 1;
		std::stringstream laybuf;
		laybuf << "100_GSFeature_S" << std::setfill('0') << std::setw(3) << abs(i) << "_T"
			<< std::setfill('0') << std::setw(3) << abs(Tnum) << "_Pnt";
		ModelSet.PrimaryLayerName = laybuf.str();

		std::stringstream clslaybuf;
		clslaybuf << "100_GTFeature_S" << std::setfill('0') << std::setw(3) << abs(i) << "_T"
			<< std::setfill('0') << std::setw(3) << abs(Tnum + 1) << "_Cls";
		ModelSet.ClassLayerName = clslaybuf.str();
		m_ModelSet.push_back(ModelSet);
	}
	else if (m_TileType == GeoTypicalModel)
	{
		int Tnum = 1;
		std::string filetype2str = ".dbf";

		for (int i = 1; i < 4; ++i)
		{
			CDB_GT_Model_Tile_Selector t;

			//			"_S001_T001_";

			std::stringstream dsbuf;
			dsbuf << "_D101_S" << std::setfill('0')
				  << std::setw(3) << abs(i) << "_T" << std::setfill('0')
				  << std::setw(3) << abs(Tnum) << "_";
			datasetstr = dsbuf.str();

			std::stringstream f1buf;
			if (!m_DataFromGlobal)
			{
				f1buf << cdbRootDir
					<< "\\Tiles"
					<< "\\" << m_lat_str
					<< "\\" << m_lon_str
					<< "\\" << m_LayerName
					<< "\\" << m_lod_str
					<< "\\" << m_uref_str
					<< "\\" << m_lat_str << m_lon_str << datasetstr << m_lod_str
					<< "_" << m_uref_str << "_" << m_rref_str << filetype;
			}
			else
			{
				f1buf << m_LayerName << datasetstr.substr(5) << "Pnt_" << m_lod_str;
			}
			t.TilePrimaryShapeName = f1buf.str();

			std::stringstream laybuf;
			laybuf << "101_GTFeature_S" << std::setfill('0') << std::setw(3) << abs(i) << "_T"
				<< std::setfill('0') << std::setw(3) << abs(Tnum) << "_Pnt";
			t.PrimaryLayerName = laybuf.str();

			std::stringstream ds2buf;
			ds2buf << "_D101_S" << std::setfill('0')
				<< std::setw(3) << abs(i) << "_T" << std::setfill('0')
				<< std::setw(3) << abs(Tnum + 1) << "_";
			datasetstr = ds2buf.str();

			std::stringstream f2buf;
			if (!m_DataFromGlobal)
			{
				f2buf << cdbRootDir
					<< "\\Tiles"
					<< "\\" << m_lat_str
					<< "\\" << m_lon_str
					<< "\\" << m_LayerName
					<< "\\" << m_lod_str
					<< "\\" << m_uref_str
					<< "\\" << m_lat_str << m_lon_str << datasetstr << m_lod_str
					<< "_" << m_uref_str << "_" << m_rref_str << filetype2str;
			}
			else
			{
				f2buf << m_LayerName << datasetstr.substr(5) << "Cls_" << m_lod_str;
			}
			t.TileSecondaryShapeName = f2buf.str();

			std::stringstream clslaybuf;
			clslaybuf << "101_GTFeature_S" << std::setfill('0') << std::setw(3) << abs(i) << "_T"
				<< std::setfill('0') << std::setw(3) << abs(Tnum + 1) << "_Cls";
			t.ClassLayerName = clslaybuf.str();
			if (!m_DataFromGlobal)
			{
				t.PrimaryExists = validate_tile_name(t.TilePrimaryShapeName);
				t.ClassExists = validate_tile_name(t.TileSecondaryShapeName);
			}
			else
			{
				__int64 tileKey = 0;
				if (m_Globalcontype == ConnGPKG && m_GlobalTile)
					tileKey = m_GlobalTile->Get_Ref();
				t.PrimaryExists = gbls->Has_Layer(t.TilePrimaryShapeName, tileKey);
				t.ClassExists = t.PrimaryExists;
//				t.ClassExists = gbls->Has_Layer(t.TileSecondaryShapeName);
			}
			t.RealSel = i - 1;
			if (t.PrimaryExists && t.ClassExists)
				m_GTModelSet.push_back(t);
		}
	}
	if ((m_TileType == Elevation) || (m_TileType == ElevationCache) || (m_TileType == Imagery))
	{
		m_SubordinateName = subordinatebuf.str();
	}
	if (m_TileType == Imagery)
	{
		m_SubordinateName2 = subordinatebuf2.str();
	}

	if (m_TileType == GeoTypicalModel)
	{
		m_FileExists = false;
		for (size_t id = 0; id < m_GTModelSet.size(); ++id)
		{
			if(m_GTModelSet[id].PrimaryExists)
				m_FileExists = true;
		}
	}
	else if (m_TileType == GeoSpecificModel)
	{
		if (!m_DataFromGlobal)
		{
			m_FileExists = validate_tile_name(m_FileName);
			if(!gbls->Get_Use_GeoPackage_Features())
			{
				m_ModelSet[0].ModelDbfNameExists = validate_tile_name(m_ModelSet[0].ModelDbfName);
			}
			m_ModelSet[0].ModelGeometryNameExists = validate_tile_name(m_ModelSet[0].ModelGeometryName);
			m_ModelSet[0].ModelTextureNameExists = validate_tile_name(m_ModelSet[0].ModelTextureName);
		}
		else
		{
			__int64 tileKey = 0;
			if(m_Globalcontype == ConnGPKG && m_GlobalTile)
				tileKey = m_GlobalTile->Get_Ref();

			m_FileExists = gbls->Has_Layer(m_FileName, tileKey);
//			m_ModelSet[0].ModelDbfNameExists = gbls->Has_Layer(m_ModelSet[0].ModelDbfName);
			m_ModelSet[0].ModelDbfNameExists = m_FileExists;


			std::string temp = "gpkg:" + m_ModelSet[0].ModelGeometryName + ":" + m_uref_str + ":" + m_rref_str + ".zip";
			m_ModelSet[0].ModelGeometryName = temp;
			m_ModelSet[0].ModelGeometryNameExists = gbls->Load_Media(m_ModelSet[0].ModelGeometryName, tileKey);

			temp = "gpkg:" + m_ModelSet[0].ModelTextureName + ":" + m_uref_str + ":" + m_rref_str + ".zip";
			m_ModelSet[0].ModelTextureName = temp;
			m_ModelSet[0].ModelTextureNameExists = gbls->Load_Media(m_ModelSet[0].ModelTextureName, tileKey);
		}
		m_ModelSet[0].ModelWorkingName = m_FileName;
		m_ModelSet[0].ModelWorkingNameExists = m_FileExists;
	}
	else
	{
		m_FileExists = validate_tile_name(m_FileName);
		if ((m_TileType == Elevation) || (m_TileType == ElevationCache))
		{
			if (gbls->EnableBathymetry())
				m_Subordinate_Exists = validate_tile_name(m_SubordinateName);
			else
				m_Subordinate_Exists = false;
		}
		if (m_TileType == Imagery)
		{
			if (m_EnableLightMap)
				m_Subordinate_Exists = validate_tile_name(m_SubordinateName);
			else
				m_Subordinate_Exists = false;
			if (m_EnableMaterials)
				m_Subordinate2_Exists = validate_tile_name(m_SubordinateName2);
			else
				m_Subordinate2_Exists = false;
		}
	}

	if (((m_TileType == GeoPackageMap)) && (!m_FileExists))
	{
		size_t spos = m_FileName.find_last_of(".gpkg");
		std::string xmlname = m_FileName;
		if (spos != std::string::npos)
		{
			xmlname = Xml_Name(m_FileName);
		}
		if (validate_tile_name(xmlname))
		{
			m_FileName = xmlname;
			m_FileExists = true;
		}
	}

	if (m_CDB_LOD_Num == 0)
	{
		if (!m_DataFromGlobal)
		{
			if (m_TileType == GeoSpecificModel)
			{
				if (gbls->LOD0_GS_FullStack())
					Build_GS_Stack();
			}
			else if (m_TileType == GeoTypicalModel)
			{
				if (gbls->LOD0_GT_FullStack())
					Build_GT_Stack();
			}
		}
	}

	m_Pixels.degPerPix.Xpos = (m_TileExtent.East - m_TileExtent.West) / (double)(m_Pixels.pixX);
	m_Pixels.degPerPix.Ypos = (m_TileExtent.North - m_TileExtent.South) / (double)(m_Pixels.pixY);


}


osgEarth::CDBTile::CDB_Tile::~CDB_Tile()
{
	Close_Dataset();

	Free_Buffers();
}

void osgEarth::CDBTile::CDB_Tile::Set_DataFromGlobal(bool value)
{
	m_DataFromGlobal = value;
	if (m_DataFromGlobal)
	{
		double clat;
		double clon;
		m_TileExtent.Center(clat, clon);
		CDB_Global * gbls = CDB_Global::getInstance();
		m_Globalcontype = gbls->Get_ConnectionType();
		if (m_Globalcontype == ConnWFS)
		{
			m_GlobalDataset = gbls->Get_Dataset();
			m_GlobalTile = gbls->FindVectorTile(clat, clon);
		}
		else
		{
			m_GlobalTile = gbls->FindVectorTile(clat, clon);
			if (m_GlobalTile)
				m_GlobalDataset = m_GlobalTile->Get_Dataset();
			else
				m_GlobalDataset = NULL;
		}
	}
}

bool osgEarth::CDBTile::CDB_Tile::DataFromGlobal(void)
{
	return m_DataFromGlobal;
}

bool osgEarth::CDBTile::CDB_Tile::Build_GS_Stack(void)
{
	CDB_Global * gbls = CDB_Global::getInstance();

	for (int nlod = 1; nlod <= 10; ++nlod)
	{
		int cdbLod = nlod * -1;
		std::stringstream lod_stream;
		std::string lod_str;
		lod_stream << "LC" << std::setfill('0') << std::setw(2) << abs(cdbLod);
		lod_str = lod_stream.str();

		std::string dataset2str = "_D100_S001_T002_";
		std::string filetype2str = ".dbf";
		std::stringstream dbfbuf;
		std::stringstream buf;
		std::string filetype;
		if(gbls->Get_Use_GeoPackage_Features())
			filetype = ".gpkg";
		else
			filetype = ".shp";
		std::string datasetstr = "_D100" + m_DataSet;
		CDB_Model_Tile_Set ModelSet;

		buf << m_cdbRootDir
			<< "\\Tiles"
			<< "\\" << m_lat_str
			<< "\\" << m_lon_str
			<< "\\" << m_LayerName
			<< "\\LC"
			<< "\\" << m_uref_str
			<< "\\" << m_lat_str << m_lon_str << datasetstr << lod_str
			<< "_" << m_uref_str << "_" << m_rref_str << filetype;

		ModelSet.ModelWorkingName = buf.str();

		dbfbuf << m_cdbRootDir
			<< "\\Tiles"
			<< "\\" << m_lat_str
			<< "\\" << m_lon_str
			<< "\\" << m_LayerName
			<< "\\LC" 
			<< "\\" << m_uref_str
			<< "\\" << m_lat_str << m_lon_str << dataset2str << lod_str
			<< "_" << m_uref_str << "_" << m_rref_str << filetype2str;


		ModelSet.ModelDbfName = dbfbuf.str();

		dataset2str = "_D300_S001_T001_";
		filetype2str = ".zip";
		std::string layerName2 = "300_GSModelGeometry";
		std::stringstream geombuf;
		geombuf << m_cdbRootDir
			<< "\\Tiles"
			<< "\\" << m_lat_str
			<< "\\" << m_lon_str
			<< "\\" << layerName2
			<< "\\LC" 
			<< "\\" << m_uref_str
			<< "\\" << m_lat_str << m_lon_str << dataset2str << lod_str
			<< "_" << m_uref_str << "_" << m_rref_str << filetype2str;

		ModelSet.ModelGeometryName = geombuf.str();

		dataset2str = "_D301_S001_T001_";
		filetype2str = ".zip";
		layerName2 = "301_GSModelTexture";
		std::stringstream txbuf;
		txbuf << m_cdbRootDir
			<< "\\Tiles"
			<< "\\" << m_lat_str
			<< "\\" << m_lon_str
			<< "\\" << layerName2
			<< "\\LC" 
			<< "\\" << m_uref_str
			<< "\\" << m_lat_str << m_lon_str << dataset2str << lod_str
			<< "_" << m_uref_str << "_" << m_rref_str << filetype2str;

		ModelSet.ModelTextureName = txbuf.str();

		int Tnum = 1;
		int i = 1;
		std::stringstream laybuf;
		laybuf << "100_GSFeature_S" << std::setfill('0') << std::setw(3) << abs(i) << "_T"
			<< std::setfill('0') << std::setw(3) << abs(Tnum) << "_Pnt";
		ModelSet.PrimaryLayerName = laybuf.str();

		std::stringstream clslaybuf;
		clslaybuf << "100_GTFeature_S" << std::setfill('0') << std::setw(3) << abs(i) << "_T"
			<< std::setfill('0') << std::setw(3) << abs(Tnum + 1) << "_Cls";
		ModelSet.ClassLayerName = clslaybuf.str();

		ModelSet.ModelWorkingNameExists = validate_tile_name(ModelSet.ModelWorkingName);;
		ModelSet.ModelDbfNameExists = validate_tile_name(ModelSet.ModelDbfName);
		ModelSet.ModelGeometryNameExists = validate_tile_name(ModelSet.ModelGeometryName);
		ModelSet.ModelTextureNameExists = validate_tile_name(ModelSet.ModelTextureName);
		ModelSet.LodStr = lod_str;
		if (ModelSet.ModelWorkingNameExists && (ModelSet.ModelDbfNameExists || gbls->Get_Use_GeoPackage_Features()))
		{
			m_ModelSet.insert(m_ModelSet.begin(), ModelSet);
		}
		else
			break;
	}
	return true;
}

bool osgEarth::CDBTile::CDB_Tile::Build_GT_Stack(void)
{
	CDB_Global* gbls = CDB_Global::getInstance();
	std::string filetype;
	if(gbls->Get_Use_GeoPackage_Features())
		filetype = ".gpkg";
	else
		filetype = ".shp";
	std::string datasetstr = "_D101" + m_DataSet;

	for (int nlod = 1; nlod <= 10; ++nlod)
	{
		int cdbLod = nlod * -1;
		std::stringstream lod_stream;
		std::string lod_str;
		lod_stream << "LC" << std::setfill('0') << std::setw(2) << abs(cdbLod);
		lod_str = lod_stream.str();
		int Tnum = 1;

		std::string filetype2str = ".dbf";

		bool have_this_lod = false;
		for (int i = 1; i < 4; ++i)
		{
			CDB_GT_Model_Tile_Selector t;

			//			"_S001_T001_";

			std::stringstream dsbuf;
			dsbuf << "_D101_S" << std::setfill('0')
				<< std::setw(3) << abs(i) << "_T" << std::setfill('0')
				<< std::setw(3) << abs(Tnum) << "_";
			datasetstr = dsbuf.str();

			std::stringstream f1buf;
			f1buf << m_cdbRootDir
				<< "\\Tiles"
				<< "\\" << m_lat_str
				<< "\\" << m_lon_str
				<< "\\" << m_LayerName
				<< "\\LC"
				<< "\\" << m_uref_str
				<< "\\" << m_lat_str << m_lon_str << datasetstr << lod_str
				<< "_" << m_uref_str << "_" << m_rref_str << filetype;
			t.TilePrimaryShapeName = f1buf.str();

			std::stringstream laybuf;
			laybuf << "101_GTFeature_S" << std::setfill('0') << std::setw(3) << abs(i) << "_T"
				<< std::setfill('0') << std::setw(3) << abs(Tnum) << "_Pnt";
			t.PrimaryLayerName = laybuf.str();

			std::stringstream ds2buf;
			ds2buf << "_D101_S" << std::setfill('0')
				<< std::setw(3) << abs(i) << "_T" << std::setfill('0')
				<< std::setw(3) << abs(Tnum + 1) << "_";
			datasetstr = ds2buf.str();

			std::stringstream f2buf;
			f2buf << m_cdbRootDir
				<< "\\Tiles"
				<< "\\" << m_lat_str
				<< "\\" << m_lon_str
				<< "\\" << m_LayerName
				<< "\\LC"
				<< "\\" << m_uref_str
				<< "\\" << m_lat_str << m_lon_str << datasetstr << lod_str
				<< "_" << m_uref_str << "_" << m_rref_str << filetype2str;
			t.TileSecondaryShapeName = f2buf.str();

			std::stringstream clslaybuf;
			clslaybuf << "101_GTFeature_S" << std::setfill('0') << std::setw(3) << abs(i) << "_T"
				<< std::setfill('0') << std::setw(3) << abs(Tnum + 1) << "_Cls";
			t.ClassLayerName = clslaybuf.str();
			t.PrimaryExists = validate_tile_name(t.TilePrimaryShapeName);
			t.ClassExists = validate_tile_name(t.TileSecondaryShapeName);
			t.RealSel = i - 1;
			if (t.PrimaryExists && (t.ClassExists || gbls->Get_Use_GeoPackage_Features()))
			{
				m_GTModelSet.insert(m_GTModelSet.begin(), t);
				have_this_lod = true;
			}
		}
		if (!have_this_lod)
			break;
	}
	return true;
}

std::string osgEarth::CDBTile::CDB_Tile::FileName(int sel)
{
	if ((sel < 0) || (!m_FileExists))
		return m_FileName;
	else if (m_TileType == GeoTypicalModel)
	{
		return m_GTModelSet[sel].TilePrimaryShapeName;
	}
	else
		return m_ModelSet[sel].ModelWorkingName;
}

int osgEarth::CDBTile::CDB_Tile::Realsel(int sel)
{
	if (sel < 0)
		return sel;
	else if (m_TileType == GeoTypicalModel)
	{
		return m_GTModelSet[sel].RealSel;
	}
	else
		return sel;
}

int osgEarth::CDBTile::CDB_Tile::CDB_LOD_Num(void)
{
	return m_CDB_LOD_Num;
}

void osgEarth::CDBTile::CDB_Tile::Free_Resources(void)
{
	Close_Dataset();
	Free_Buffers();
}

int osgEarth::CDBTile::CDB_Tile::LodNumFromExtent(CDB_Tile_Extent &Tile_Extent)
{
	int cdbLod = 0;

	//Determine the CDB LOD
	double keylonspace = Tile_Extent.East - Tile_Extent.West;
	double keylatspace = Tile_Extent.North - Tile_Extent.South;

	double tilesperdeg = 1.0 / keylatspace;
	double tilesperdegX = 1.0 / keylonspace;

	if (tilesperdeg < 0.99)
	{
		//This is a multi-tile cash tile
		double lnum = 1.0 / tilesperdeg;
		int itiles = (int)(round(lnum / 2.0));
		cdbLod = -1;
		while (itiles > 1)
		{
			itiles /= 2;
			--cdbLod;
		}
	}
	else
	{
		cdbLod = 0;
		int itiles = (int)round(tilesperdeg);
		while (itiles > 1)
		{
			itiles /= 2;
			++cdbLod;
		}
	}
	return cdbLod;
}

__int64 osgEarth::CDBTile::CDB_Tile::Get_TileKeyValue(std::string FileName)
{
	std::string workingName = FileName;
	size_t pos = workingName.find("\\");
	while (pos != std::string::npos)
	{
		workingName.replace(pos, 1, "/");
		pos = workingName.find("\\");
	}

	pos = workingName.find("/");
	while (pos != std::string::npos)
	{
		if (pos < workingName.length())
		{
			workingName = workingName.substr(pos + 1);
		}
		else
			break;
		pos = workingName.find("/");
	}
	std::string latstr = workingName.substr(1, 2);
	std::string lonstr = workingName.substr(4, 3);

	std::string ustr;
	pos = workingName.find("_U");
	if (pos != std::string::npos)
	{
		if (pos + 2 < workingName.length())
		{
			std::string uworkstr = workingName.substr(pos + 2);
			size_t nt = uworkstr.find("_");
			if (nt != std::string::npos)
			{
				ustr = uworkstr.substr(0, nt);
			}
		}
	}

	std::string rstr;
	pos = workingName.find("_R");
	if (pos != std::string::npos)
	{
		if (pos + 2 < workingName.length())
		{
			std::string rworkstr = workingName.substr(pos + 2);
			size_t nt = rworkstr.find(".");
			if (nt != std::string::npos)
			{
				rstr = rworkstr.substr(0, nt);
			}
		}
	}
	__int64 retval = -1;
	if (!latstr.empty() && !lonstr.empty() && !ustr.empty() && !rstr.empty())
	{
		int ilat = atoi(latstr.c_str());
		if (workingName.substr(0, 1) == "S")
			ilat *= -1;
		ilat += 90;

		int ilon = atoi(lonstr.c_str());
		if (workingName.substr(3, 1) == "W")
			ilon *= -1;
		ilon += 180;

		int iunum = atoi(ustr.c_str());
		int irnum = atoi(rstr.c_str());
		std::stringstream	buf;
		buf << std::setfill('0')
			<< std::setw(2) << ilat << std::setfill('0')
			<< std::setw(3) << ilon << std::setfill('0')
			<< std::setw(3) << iunum << std::setfill('0')
			<< std::setw(3) << irnum;
		std::string keystring = buf.str();
		retval = _atoi64(keystring.c_str());
	}
	return retval;
}

int osgEarth::CDBTile::CDB_Tile::GetPathComponents(std::string& lat_str, std::string& lon_str, std::string& lod_str,
								std::string& uref_str, std::string& rref_str)
{

	int cdbLod = 0;

	//Determine the CDB LOD
	double keylonspace = m_TileExtent.East - m_TileExtent.West;
	double keylatspace = m_TileExtent.North - m_TileExtent.South;

	double tilesperdeg = 1.0 / keylatspace;
	double tilesperdegX = 1.0 / keylonspace;

	if (tilesperdeg < 0.99)
	{
		//This is a multi-tile cash tile
		double lnum = 1.0 / tilesperdeg;
		int itiles = (int)(round(lnum / 2.0));
		cdbLod = -1;
		while (itiles > 1)
		{
			itiles /= 2;
			--cdbLod;
		}
	}
	else
	{
		if (m_Pixels.pixX < 1024)
		{
			// In this case the lod num has already been passed into the class
			// just use it.
			cdbLod = m_CDB_LOD_Num;
		}
		else
		{
			cdbLod = 0;
			int itiles = (int)round(tilesperdeg);
			while (itiles > 1)
			{
				itiles /= 2;
				++cdbLod;
			}

		}
	}
	int tile_x, tile_y;

	double Base_lon = m_TileExtent.West;

	if (cdbLod > 0)
	{
		double Base_lat = (double)((int)m_TileExtent.South);
		if (m_TileExtent.South < Base_lat)
			Base_lat -= 1.0;
		double off = m_TileExtent.South - Base_lat;
		tile_y = (int)round(off * tilesperdeg);

		double lon_step = Get_Lon_Step(m_TileExtent.South);
		Base_lon = (double)((int)m_TileExtent.West);

		if (lon_step != 1.0)
		{
			int checklon = (int)abs(Base_lon);
			if (checklon % (int)lon_step)
			{
				double sign = Base_lon < 0.0 ? -1.0 : 1.0;
				checklon = (checklon / (int)lon_step) * (int)lon_step;
				Base_lon = (double)checklon * sign;
				if (sign < 0.0)
					Base_lon -= lon_step;
			}
		}

		if (m_TileExtent.West < Base_lon)
			Base_lon -= lon_step;

		off = m_TileExtent.West - Base_lon;
		tile_x = (int)round(off * tilesperdegX);
	}
	else
	{
		tile_x = tile_y = 0;
	}
	//Determine the base lat lon directory
	double lont = (double)((int)Base_lon);
	//make sure there wasn't a rounding error
	if (abs((lont + 1.0) - Base_lon) < DBL_EPSILON)
		lont += 1.0;
	else if (Base_lon < lont)//Where in the Western Hemisphere round down.
		lont -= 1.0;


	int londir = (int)lont;
	std::stringstream format_stream_1;
	format_stream_1 << ((londir < 0) ? "W" : "E") << std::setfill('0')
		<< std::setw(3) << abs(londir);
	lon_str = format_stream_1.str();


	double latt = (double)((int)m_TileExtent.South);
	//make sure there wasn't a rounding error
	if (abs((latt + 1.0) - m_TileExtent.South) < DBL_EPSILON)
		latt += 1.0;
	else if (m_TileExtent.South < latt) //Where in the Southern Hemisphere round down.
		latt -= 1.0;

	int latdir = (int)latt;
	std::stringstream format_stream_2;
	format_stream_2 << ((latdir < 0) ? "S" : "N") << std::setfill('0')
		<< std::setw(2) << abs(latdir);
	lat_str = format_stream_2.str();

	// Set the LOD of the request
	std::stringstream lod_stream;
	if (cdbLod < 0)
		lod_stream << "LC" << std::setfill('0') << std::setw(2) << abs(cdbLod);
	else
		lod_stream << "L" << std::setfill('0') << std::setw(2) << cdbLod;
	lod_str = lod_stream.str();

	if (cdbLod < 1)
	{
		//There is only one tile in cdb levels 0 and below
		//
		uref_str = "U0";
		rref_str = "R0";
	}
	else
	{
		// Determine UREF
		std::stringstream uref_stream;
		uref_stream << "U" << tile_y;
		uref_str = uref_stream.str();

		// Determine RREF
		std::stringstream rref_stream;
		rref_stream << "R" << tile_x;
		rref_str = rref_stream.str();
	}
	return cdbLod;
}


void osgEarth::CDBTile::CDB_Tile::Allocate_Buffers(void)
{
	int bandbuffersize = m_Pixels.pixX * m_Pixels.pixY;
	if ((m_TileType == Imagery) || (m_TileType == ImageryCache))
	{
		if (!m_Subordinate_Exists && !m_Subordinate2_Exists)
		{
			if (!m_GDAL.reddata)
			{
				m_GDAL.reddata = new unsigned char[bandbuffersize * 3];
				m_GDAL.greendata = m_GDAL.reddata + bandbuffersize;
				m_GDAL.bluedata = m_GDAL.greendata + bandbuffersize;
			}
		}
		else if (m_Subordinate_Exists)
		{
			if (!m_GDAL.lightmapdatar)
			{
				m_GDAL.lightmapdatar = new unsigned char[bandbuffersize * 3];
				m_GDAL.lightmapdatag = m_GDAL.lightmapdatar + bandbuffersize;
				m_GDAL.lightmapdatab = m_GDAL.lightmapdatag + bandbuffersize;
			}
		}
		else if (m_Subordinate2_Exists)
		{
			if (!m_GDAL.materialdata)
				m_GDAL.materialdata = new unsigned char[bandbuffersize];
			if (m_EnableMaterialMask)
			{
				if(!m_GDAL.materialmaskdata)
					m_GDAL.materialmaskdata = new unsigned char[bandbuffersize];
			}
		}
	}
	else if ((m_TileType == Elevation) || (m_TileType == ElevationCache))
	{
		if (!m_GDAL.elevationdata)
			m_GDAL.elevationdata = new float[bandbuffersize];
		if (m_Subordinate_Exists || m_Subordinate_Tile)
		{
			if (!m_GDAL.subord_elevationdata)
				m_GDAL.subord_elevationdata = new float[bandbuffersize];
		}
	}
}

void osgEarth::CDBTile::CDB_Tile::Free_Buffers(void)
{
	if (m_GDAL.reddata)
	{
		delete m_GDAL.reddata;
		m_GDAL.reddata = NULL;
		m_GDAL.greendata = NULL;
		m_GDAL.bluedata = NULL;
	}
	if (m_GDAL.elevationdata)
	{
		delete m_GDAL.elevationdata;
		m_GDAL.elevationdata = NULL;
	}
	if (m_GDAL.subord_elevationdata)
	{
		delete m_GDAL.subord_elevationdata;
		m_GDAL.subord_elevationdata = NULL;
	}
	if (m_GDAL.lightmapdatar)
	{
		delete m_GDAL.lightmapdatar;
		m_GDAL.lightmapdatar = NULL;
		m_GDAL.lightmapdatag = NULL;
		m_GDAL.lightmapdatab = NULL;
	}
	if (m_GDAL.materialdata)
	{
		delete m_GDAL.materialdata;
		m_GDAL.materialdata = NULL;
	}
	if (m_GDAL.materialmaskdata)
	{
		delete m_GDAL.materialmaskdata;
		m_GDAL.materialmaskdata = NULL;
	}
	if (m_Tile_Status == Loaded)
	{
		if (m_GDAL.poDataset)
			m_Tile_Status = Opened;
		else
			m_Tile_Status = Created;
	}
}

void osgEarth::CDBTile::CDB_Tile::Close_Dataset(void)
{

	if (m_GDAL.poDataset)
	{
		GDALClose(m_GDAL.poDataset);
		m_GDAL.poDataset = NULL;
	}
	if (m_GDAL.soDataset)
	{
		GDALClose(m_GDAL.soDataset);
		m_GDAL.soDataset = NULL;
	}
	if (m_TileType == GeoTypicalModel)
		Close_GT_Model_Tile();
	else if (m_TileType == GeoSpecificModel)
		Close_GS_Model_Tile();

	m_Tile_Status = Created;

}

bool osgEarth::CDBTile::CDB_Tile::Open_Tile(void)
{
	if (!m_DataFromGlobal)
	{
		if (m_GDAL.poDataset)
			return true;
	}
	GDALOpenInfo oOpenInfo(m_FileName.c_str(), GA_ReadOnly);
	if (m_TileType == Imagery)
	{
		m_GDAL.poDriver = Gbl_TileDrivers.cdb_JP2Driver;
		m_GDAL.so2Driver = Gbl_TileDrivers.cdb_GTIFFDriver;
	}
	else if (m_TileType == ImageryCache)
	{
		m_GDAL.poDriver = Gbl_TileDrivers.cdb_GTIFFDriver;
		m_GDAL.so2Driver = Gbl_TileDrivers.cdb_GTIFFDriver;
	}
	else if (m_TileType == Elevation)
	{
		m_GDAL.poDriver = Gbl_TileDrivers.cdb_GTIFFDriver;
	}
	else if (m_TileType == ElevationCache)
	{
		m_GDAL.poDriver = Gbl_TileDrivers.cdb_HFADriver;
	}
	else if (m_TileType == GeoTypicalModel)
	{
		if(m_DataFromGlobal)
			m_GDAL.poDriver = m_GlobalDataset->GetDriver();
		else
		{
			CDB_Global* gbls = CDB_Global::getInstance();
			if (gbls->Get_Use_GeoPackage_Features())
				m_GDAL.poDriver = Gbl_TileDrivers.cdb_GeoPackageDriver;
			else
				m_GDAL.poDriver = Gbl_TileDrivers.cdb_ShapefileDriver;
		}
		return Open_GT_Model_Tile();
	}
	else if (m_TileType == GeoSpecificModel)
	{
		if (m_DataFromGlobal)
			m_GDAL.poDriver = m_GlobalDataset->GetDriver();
		else
		{
			CDB_Global * gbls = CDB_Global::getInstance();
			if(gbls->Get_Use_GeoPackage_Features())
				m_GDAL.poDriver = Gbl_TileDrivers.cdb_GeoPackageDriver;
			else
				m_GDAL.poDriver = Gbl_TileDrivers.cdb_ShapefileDriver;
		}
		return Open_GS_Model_Tile();
	}
	else if (m_TileType == GeoPackageMap)
	{
		m_GDAL.poDriver = Gbl_TileDrivers.cdb_GeoPackageDriver;
		return Open_GP_Map_Tile();
	}
	if ((m_TileType == Elevation) || (m_TileType == ElevationCache))
	{
		m_GDAL.poDataset = (GDALDataset *)m_GDAL.poDriver->pfnOpen(&oOpenInfo);
		if (m_Subordinate_Exists)
		{
			GDALOpenInfo sOpenInfo(m_SubordinateName.c_str(), GA_ReadOnly);
			m_GDAL.soDataset = (GDALDataset *)m_GDAL.poDriver->pfnOpen(&sOpenInfo);
		}
		if (m_Subordinate2_Exists)
		{
			GDALOpenInfo sOpenInfo(m_SubordinateName2.c_str(), GA_ReadOnly);
			m_GDAL.so2Dataset = (GDALDataset *)m_GDAL.so2Driver->pfnOpen(&sOpenInfo);
		}
		if (!m_GDAL.poDataset)
		{
			return false;
		}
		if (m_Subordinate_Exists && !m_GDAL.soDataset)
		{
			return false;
		}
		if (m_Subordinate2_Exists && !m_GDAL.so2Dataset)
		{
			return false;
		}
		m_GDAL.poDataset->GetGeoTransform(m_GDAL.adfGeoTransform);
	}
	else if ((m_TileType == Imagery) || (m_TileType == ImageryCache))
	{
		if (!m_Subordinate_Exists && !m_Subordinate2_Exists)
		{
			m_GDAL.poDataset = (GDALDataset *)m_GDAL.poDriver->pfnOpen(&oOpenInfo);
			if (!m_GDAL.poDataset)
			{
				return false;
			}
			m_GDAL.poDataset->GetGeoTransform(m_GDAL.adfGeoTransform);
		}
		else if (m_Subordinate_Exists)
		{
			GDALOpenInfo sOpenInfo(m_SubordinateName.c_str(), GA_ReadOnly);
			m_GDAL.soDataset = (GDALDataset *)m_GDAL.poDriver->pfnOpen(&sOpenInfo);
			if ( !m_GDAL.soDataset)
			{
				return false;
			}
			m_GDAL.soDataset->GetGeoTransform(m_GDAL.adfGeoTransform);
		}
		else if (m_Subordinate2_Exists)
		{
			GDALOpenInfo sOpenInfo(m_SubordinateName2.c_str(), GA_ReadOnly);
			m_GDAL.so2Dataset = (GDALDataset *)m_GDAL.so2Driver->pfnOpen(&sOpenInfo);
			if (!m_GDAL.so2Dataset)
			{
				return false;
			}
			m_GDAL.so2Dataset->GetGeoTransform(m_GDAL.adfGeoTransform);
		}
	}
	m_Tile_Status = Opened;
	return true;
}

bool osgEarth::CDBTile::CDB_Tile::Open_GS_Model_Tile(void)
{
	bool Have_a_Valid_set = false;
	for(unsigned int i = 0; i < m_ModelSet.size(); ++i)
	{
		bool valid_set = true;
		if(!m_DataFromGlobal)
		{
			CDB_Global * gbls = CDB_Global::getInstance();
			if(gbls->Get_Use_GeoPackage_Features())
			{
				if(m_ModelSet[i].ModelWorkingNameExists)
				{
					char* drivers[2];
					drivers[0] = "GPKG";
					drivers[1] = NULL;
					m_ModelSet[i].PrimaryTileOgr = (GDALDataset*)GDALOpenEx(m_ModelSet[i].ModelWorkingName.c_str(), GDAL_OF_VECTOR | GA_ReadOnly | GDAL_OF_SHARED, drivers, NULL, NULL);
					if (!m_ModelSet[i].PrimaryTileOgr)
					{
						valid_set = false;
						continue;
					}
				}
				else
					valid_set = false;
			}
			else
			{
				if (m_ModelSet[i].ModelWorkingNameExists && m_ModelSet[i].ModelDbfNameExists && m_ModelSet[i].ModelGeometryNameExists)
				{
					GDALOpenInfo oOpenInfoP(m_ModelSet[i].ModelWorkingName.c_str(), GA_ReadOnly);
					m_ModelSet[i].PrimaryTileOgr = m_GDAL.poDriver->pfnOpen(&oOpenInfoP);
					if (!m_ModelSet[i].PrimaryTileOgr)
					{
						valid_set = false;
						continue;
					}

					GDALOpenInfo oOpenInfoC(m_ModelSet[i].ModelDbfName.c_str(), GA_ReadOnly);
					m_ModelSet[i].ClassTileOgr = m_GDAL.poDriver->pfnOpen(&oOpenInfoC);
					if (!m_ModelSet[i].ClassTileOgr)
					{
						//Check for junk files clogging up the works
						std::string shx = Set_FileType(m_ModelSet[i].ModelDbfName, ".shx");
						if (validate_tile_name(shx))
						{
							if (::DeleteFile(shx.c_str()) == 0)
							{
								return false;
							}
						}
						std::string shp = Set_FileType(m_ModelSet[i].ModelDbfName, ".shp");
						if (validate_tile_name(shp))
						{
							if (::DeleteFile(shp.c_str()) == 0)
							{
								return false;
							}
						}
						m_ModelSet[i].ClassTileOgr = m_GDAL.poDriver->pfnOpen(&oOpenInfoC);
						if (!m_ModelSet[i].ClassTileOgr)
						{
							valid_set = false;
							if (m_ModelSet[i].PrimaryTileOgr)
							{
								GDALClose(m_ModelSet[i].PrimaryTileOgr);
								m_ModelSet[i].PrimaryTileOgr = NULL;
							}
							continue;
						}
					}
				}
				else
					valid_set = false;
			}
			if (valid_set)
				Have_a_Valid_set = true;
		}
		else
		{
			if (m_ModelSet[i].ModelWorkingNameExists && m_ModelSet[i].ModelDbfNameExists && m_ModelSet[i].ModelGeometryNameExists)
				valid_set = true;
			if (valid_set)
				Have_a_Valid_set = true;
		}
	}
	if(Have_a_Valid_set)
		m_Tile_Status = Opened;
	return Have_a_Valid_set;
}

bool osgEarth::CDBTile::CDB_Tile::Open_GT_Model_Tile(void)
{
	bool have_an_opening = false;
	for (size_t i = 0; i < m_GTModelSet.size(); ++i)
	{
		if (!m_DataFromGlobal)
		{
			if (m_GTModelSet[i].PrimaryExists && m_GTModelSet[i].ClassExists)
			{
				GDALOpenInfo oOpenInfoP(m_GTModelSet[i].TilePrimaryShapeName.c_str(), GA_ReadOnly);
				m_GTModelSet[i].PrimaryTileOgr = m_GDAL.poDriver->pfnOpen(&oOpenInfoP);
				if (!m_GTModelSet[i].PrimaryTileOgr)
					continue;
				GDALOpenInfo oOpenInfoC(m_GTModelSet[i].TileSecondaryShapeName.c_str(), GA_ReadOnly);
				m_GTModelSet[i].ClassTileOgr = m_GDAL.poDriver->pfnOpen(&oOpenInfoC);
				if (!m_GTModelSet[i].ClassTileOgr)
				{
					//Check for junk files clogging up the works
					std::string shx = Set_FileType(m_GTModelSet[i].TileSecondaryShapeName, ".shx");
					if (validate_tile_name(shx))
					{
						if (::DeleteFile(shx.c_str()) == 0)
						{
							continue;
						}
					}
					std::string shp = Set_FileType(m_GTModelSet[i].TileSecondaryShapeName, ".shp");
					if (validate_tile_name(shp))
					{
						if (::DeleteFile(shp.c_str()) == 0)
						{
							continue;
						}
					}
					m_GTModelSet[i].ClassTileOgr = m_GDAL.poDriver->pfnOpen(&oOpenInfoC);
					if (!m_GTModelSet[i].ClassTileOgr)
						continue;
				}
				if (m_GTModelSet[i].PrimaryTileOgr && m_GTModelSet[i].ClassTileOgr)
					have_an_opening = true;
			}
		}
		else
		{
			if (m_GTModelSet[i].PrimaryExists && m_GTModelSet[i].ClassExists)
			{
				have_an_opening = true;
			}
		}
	}
	if(have_an_opening)
		m_Tile_Status = Opened;
	return have_an_opening;
}

bool osgEarth::CDBTile::CDB_Tile::Open_GP_Map_Tile(void)
{
	if (m_FileExists && (m_FileName.find(".gpkg") != std::string::npos))
	{
//		GDALOpenInfo oOpenInfoP(m_FileName.c_str(), GA_ReadOnly | GDAL_OF_VECTOR);
//		m_GDAL.poDataset = m_GDAL.poDriver->pfnOpen(&oOpenInfoP);
		char * drivers[2];
		drivers[0] = "GPKG";
		drivers[1] = NULL;
		m_GDAL.poDataset = (GDALDataset *)GDALOpenEx(m_FileName.c_str(), GDAL_OF_VECTOR | GA_ReadOnly | GDAL_OF_SHARED, drivers, NULL, NULL);
		if (!m_GDAL.poDataset)
			return false;
	}
	else
		return false;
	m_Tile_Status = Opened;
	return true;
}

OGRLayer * osgEarth::CDBTile::CDB_Tile::Map_Tile_Layer(std::string LayerName)
{
	if (m_TileType != GeoPackageMap)
		return NULL;
	if (!m_GDAL.poDataset)
		return NULL;
	return m_GDAL.poDataset->GetLayerByName(LayerName.c_str());
}

GDALDataset * osgEarth::CDBTile::CDB_Tile::Map_Tile_Dataset(void)
{
	if (m_TileType != GeoPackageMap)
		return NULL;
	return m_GDAL.poDataset;
}
void osgEarth::CDBTile::CDB_Tile::Close_GT_Model_Tile(void)
{
	for (size_t i = 0; i < m_GTModelSet.size(); ++i)
	{
		if (m_GTModelSet[i].PrimaryTileOgr)
		{
			GDALClose(m_GTModelSet[i].PrimaryTileOgr);
			m_GTModelSet[i].PrimaryTileOgr = NULL;
			m_GTModelSet[i].PrimaryLayer = NULL;
		}
		if (m_GTModelSet[i].ClassTileOgr)
		{
			GDALClose(m_GTModelSet[i].ClassTileOgr);
			m_GTModelSet[i].ClassTileOgr = NULL;
		}
		m_GTModelSet[i].clsMap.clear();
	}
}

void osgEarth::CDBTile::CDB_Tile::Close_GS_Model_Tile(void)
{
	for (unsigned int i = 0; i < m_ModelSet.size(); ++i)
	{
		if (m_ModelSet[i].PrimaryTileOgr)
		{
			GDALClose(m_ModelSet[i].PrimaryTileOgr);
			m_ModelSet[i].PrimaryTileOgr = NULL;
			m_ModelSet[i].PrimaryLayer = NULL;
		}

		if (m_ModelSet[i].ClassTileOgr)
		{
			GDALClose(m_ModelSet[i].ClassTileOgr);
			m_ModelSet[i].ClassTileOgr = NULL;
		}

		m_ModelSet[i].clsMap.clear();
		m_ModelSet[i].archiveFileList.clear();
	}
}


bool osgEarth::CDBTile::CDB_Tile::Init_Model_Tile(int sel)
{
	if (m_Tile_Status != Opened)
	{
		if (!Open_Tile())
			return false;
	}

	if (m_TileType == GeoSpecificModel)
	{
		return Init_GS_Model_Tile(sel);
	}
	else if (m_TileType == GeoTypicalModel)
	{
		return Init_GT_Model_Tile(sel);
	}
	return false;
}

bool osgEarth::CDBTile::CDB_Tile::Init_Map_Tile(void)
{
	if (m_Tile_Status != Opened)
	{
		if (!Open_Tile())
			return false;
	}
	return true;
}

bool osgEarth::CDBTile::CDB_Tile::Model_Geometry_Name(std::string &GeometryName, unsigned int pos)
{
	if (m_TileType != GeoSpecificModel)
	{
		GeometryName = "";
		return false;
	}
	GeometryName = m_ModelSet[pos].ModelGeometryName;
	return m_ModelSet[pos].ModelGeometryNameExists;
}

bool osgEarth::CDBTile::CDB_Tile::Model_Texture_Directory(std::string &TextureDir)
{
	if (m_TileType != GeoSpecificModel)
	{
		TextureDir = "";
		return false;
	}
	TextureDir = Model_TextureDir();
	return validate_tile_name(TextureDir);
}

bool osgEarth::CDBTile::CDB_Tile::Model_Texture_Archive(std::string &TextureArchive, unsigned int pos)
{
	if (m_TileType != GeoSpecificModel)
	{
		TextureArchive = "";
		return false;
	}
	TextureArchive = m_ModelSet[pos].ModelTextureName;
	return m_ModelSet[pos].ModelTextureNameExists;
}

bool osgEarth::CDBTile::CDB_Tile::DestroyCurrentFeature(int sel)
{
	if (m_TileType == GeoSpecificModel)
		return DestroyCurrent_Geospecific_Feature(sel);
	else
		return DestroyCurrent_GeoTypical_Feature(sel);
}

bool osgEarth::CDBTile::CDB_Tile::DestroyCurrent_Geospecific_Feature(int sel)
{
	m_ModelSet[sel].FeatureSet.DestroyCurFeature();
	return true;
}

bool osgEarth::CDBTile::CDB_Tile::DestroyCurrent_GeoTypical_Feature(int sel)
{
	m_GTModelSet[sel].FeatureSet.DestroyCurFeature();
	return true;
}

OGRFeature * osgEarth::CDBTile::CDB_Tile::Next_Valid_Feature(int sel, bool inflated, std::string &ModelKeyName, std::string &FullModelName, std::string &ArchiveFileName,
										  bool &Model_in_Archive)
{
	if (m_TileType == GeoSpecificModel)
		return Next_Valid_Geospecific_Feature(inflated, ModelKeyName, FullModelName, ArchiveFileName, Model_in_Archive, sel);
	else
		return Next_Valid_GeoTypical_Feature(sel, ModelKeyName, FullModelName, ArchiveFileName, Model_in_Archive);
}

OGRFeature * osgEarth::CDBTile::CDB_Tile::Next_Valid_Geospecific_Feature(bool inflated, std::string &ModelKeyName, std::string &FullModelName, std::string &ArchiveFileName,
													   bool &Model_in_Archive, unsigned int pos)
{
	bool valid = false;
	bool done = false;
	OGRFeature *f = NULL;
	CDB_Model_RuntimeMapP clsmap;
	CDB_Global * gbls = CDB_Global::getInstance();

	if (m_DataFromGlobal)
		clsmap = NULL; // m_GlobalTile->GetGSClassMap(m_CDB_LOD_Num);
	else
	{
		if(gbls->Get_Use_GeoPackage_Features())
			clsmap = NULL;
		else
			clsmap = &m_ModelSet[pos].clsMap;
	}
	while (!valid && !done)
	{
		f = m_ModelSet[pos].FeatureSet.GetNextFeature();
		if (!f)
		{
			done = true;
			break;
		}
		valid = true;
#if 0
		if (m_Use_Spatial_Rect)
		{
			OGRPoint *poPoint = (OGRPoint *)f->GetGeometryRef();
			if (!m_SpatialRectExtent.Contains(poPoint->getX(), poPoint->getY()))
				valid = false;
		}
#endif
		std::string cnam = f->GetFieldAsString("CNAM");
		if (!cnam.empty() || gbls->Get_Use_GeoPackage_Features())
		{
			if (clsmap)
			{
				CDB_Model_RuntimeMap::iterator mi = clsmap->find(cnam);
				if (mi == clsmap->end())
					valid = false;
				else
				{
					m_CurFeatureClass = (*clsmap)[cnam];
				}
			}
			else
			{
				std::string myCNAM = m_CurFeatureClass.inst_set_class(m_ModelSet[pos].PrimaryLayer, f);
				if (myCNAM.empty() && !gbls->Get_Use_GeoPackage_Features())
					valid = false;
			}
			if(valid)
			{
				ModelKeyName = Model_KeyName(m_CurFeatureClass.FACC_value, m_CurFeatureClass.FSC_value, m_CurFeatureClass.Model_Base_Name);
				if (inflated)
				{
					FullModelName = Model_FullFileName(m_CurFeatureClass.FACC_value, m_CurFeatureClass.FSC_value, m_CurFeatureClass.Model_Base_Name);
					Model_in_Archive = validate_tile_name(FullModelName);
				}
				else
				{
					FullModelName = Model_FileName(m_CurFeatureClass.FACC_value, m_CurFeatureClass.FSC_value, m_CurFeatureClass.Model_Base_Name);
					ArchiveFileName = archive_validate_modelname(m_ModelSet[pos].archiveFileList, FullModelName);
					if (ArchiveFileName.empty())
						Model_in_Archive = false;
					else
						Model_in_Archive = true;
				}
			}
		}
		else
		{
			valid = false;
		}
		if (!valid)
		{
			m_ModelSet[pos].FeatureSet.DestroyCurFeature();
		}
	}
	return f;
}

OGRFeature * osgEarth::CDBTile::CDB_Tile::Next_Valid_GeoTypical_Feature(int sel, std::string &ModelKeyName, std::string &ModelFullName, std::string &ArchiveFileName, bool &Model_in_Archive)
{
	bool valid = false;
	bool done = false;
	OGRFeature *f = NULL;
	CDB_Model_RuntimeMapP clsmap;
	CDB_Global* gbls = CDB_Global::getInstance();
	if (m_DataFromGlobal)
		clsmap = NULL; //m_GlobalTile->GetGTClassMap(m_CDB_LOD_Num, sel);
	else
	{
		if(gbls->Get_Use_GeoPackage_Features())
			clsmap = NULL;
		else
			clsmap = &m_GTModelSet[sel].clsMap;
	}

	while (!valid && !done)
	{
		f = m_GTModelSet[sel].FeatureSet.GetNextFeature();
		if (!f)
		{
			done = true;
			break;
		}
		valid = true;
#if 0
		if (m_Use_Spatial_Rect)
		{
			OGRPoint *poPoint = (OGRPoint *)f->GetGeometryRef();
			if (!m_SpatialRectExtent.Contains(poPoint->getX(), poPoint->getY()))
				valid = false;
		}
#endif
		std::string cnam = f->GetFieldAsString("CNAM");
		if (!cnam.empty() || gbls->Get_Use_GeoPackage_Features())
		{
			if (clsmap)
			{
				CDB_Model_RuntimeMap::iterator mi = clsmap->find(cnam);
				if (mi == clsmap->end())
					valid = false;
				else
				{
					m_CurFeatureClass = (*clsmap)[cnam];
				}
			}
			else
			{
				std::string myCNAM = m_CurFeatureClass.inst_set_class(m_GTModelSet[sel].PrimaryLayer, f);
				if (myCNAM.empty() && !gbls->Get_Use_GeoPackage_Features())
					valid = false;
			}
			if(valid)
			{
				ModelKeyName = Model_KeyName(m_CurFeatureClass.FACC_value, m_CurFeatureClass.FSC_value, m_CurFeatureClass.Model_Base_Name);
				ModelFullName = GeoTypical_FullFileName(ModelKeyName);
				if(!m_DataFromGlobal)
					Model_in_Archive = validate_tile_name(ModelFullName);
				else
				{
					ArchiveFileName = archive_validate_modelname(m_GTGeomerty_archiveFileList, ModelFullName);
					Model_in_Archive = !ArchiveFileName.empty();
				}
				if (!Model_in_Archive)
				{
					if (CDB_Global::getInstance()->CDB_Tile_Be_Verbose())
					{
						OSG_WARN << "File " << ModelFullName << " reference from " << m_FileName << " not found" << std::endl;
					}
				}
			}
		}
		else
			valid = false;
		if (!valid)
		{
			m_GTModelSet[sel].FeatureSet.DestroyCurFeature();
		}
	}
	return f;
}

CDB_Model_Runtime_Class osgEarth::CDBTile::CDB_Tile::Current_Feature_Class_Data(void)
{
	return m_CurFeatureClass;
}

std::string osgEarth::CDBTile::CDB_Tile::Model_KeyName(std::string &FACC_value, std::string &FSC_Value, std::string &BaseFileName)
{
	std::stringstream modbuf;
	modbuf << FACC_value << "_" << FSC_Value << "_"
		<< BaseFileName << ".flt";
	return modbuf.str();
}

bool osgEarth::CDBTile::CDB_Tile::Init_GS_Model_Tile(unsigned int pos)
{
#ifdef _DEBUG
	int fubar = 0;
#endif
	CDB_Global* gbls = CDB_Global::getInstance();
	if (!m_DataFromGlobal)
	{
		if(!gbls->Get_Use_GeoPackage_Features())
		{
			if (!m_ModelSet[pos].ClassTileOgr)
				return false;
		}
		m_ModelSet[pos].PrimaryLayer = m_ModelSet[pos].PrimaryTileOgr->GetLayer(0);
		if (!m_ModelSet[pos].PrimaryLayer)
			return false;
		if (m_Use_Spatial_Rect)
		{
			m_ModelSet[pos].PrimaryLayer->SetSpatialFilterRect(m_SpatialRectExtent.West, m_SpatialRectExtent.South, m_SpatialRectExtent.East, m_SpatialRectExtent.North);
		}
		m_ModelSet[pos].FeatureSet.LoadFeatureSet(m_ModelSet[pos].PrimaryLayer);
	}
	else
	{
		std::string LayerName = m_FileName;
		if (m_Globalcontype == ConnWFS)
		{
			LayerName = CDB_Global::getInstance()->ToWFSLayer(m_FileName);
		}
		m_ModelSet[pos].PrimaryLayer = m_GlobalDataset->GetLayerByName(LayerName.c_str());
		if (!m_ModelSet[pos].PrimaryLayer)
			return false;
		if (m_Use_Spatial_Rect)
		{
			m_ModelSet[pos].PrimaryLayer->SetSpatialFilterRect(m_SpatialRectExtent.West, m_SpatialRectExtent.South, m_SpatialRectExtent.East, m_SpatialRectExtent.North);
		}
		m_ModelSet[pos].FeatureSet.LoadFeatureSet(m_ModelSet[pos].PrimaryLayer);
		m_CurFeatureClass.Init();
#ifdef _DEBUG
		if (m_GlobalTile)
		{
			GDALDataset * poDataset = m_GlobalTile->Get_Dataset();
			if (poDataset)
			{
				OGRLayer * poLayer = poDataset->GetLayerByName(m_FileName.c_str());
				if (poLayer)
				{
					if (m_Use_Spatial_Rect)
					{
						poLayer->SetSpatialFilterRect(m_SpatialRectExtent.West, m_SpatialRectExtent.South, m_SpatialRectExtent.East, m_SpatialRectExtent.North);
					}
					m_ModelSet[pos].DebugFeatureSet.LoadFeatureSet(poLayer);
				}
			}
			if (abs((int)m_ModelSet[pos].FeatureSet.Size() - (int)m_ModelSet[pos].DebugFeatureSet.Size()) > 1)
			{
				++fubar;
			}
		}
#endif
	}


	OGRLayer *poLayer = NULL;
	if (!m_DataFromGlobal && !gbls->Get_Use_GeoPackage_Features())
		poLayer = m_ModelSet[pos].ClassTileOgr->GetLayer(0);
//	else
//		poLayer = m_GlobalDataset->GetLayerByName(m_ModelSet[pos].ModelDbfName.c_str());

	bool have_class = false;
	if (m_DataFromGlobal)
	{
		//CDB_Model_RuntimeMapP clsmap = NULL; // m_GlobalTile->GetGSClassMap(m_CDB_LOD_Num);
		//if (clsmap->size() == 0)
		//{
		//	have_class = Load_Class_Map(poLayer, *clsmap);
		//}
		//else
		have_class = true;
	}
	else
	{
		if(gbls->Get_Use_GeoPackage_Features())
			have_class = true;
		else
			have_class = Load_Class_Map(poLayer, m_ModelSet[pos].clsMap);
	}
	bool have_archive = Load_Archive(m_ModelSet[pos].ModelGeometryName, m_ModelSet[pos].archiveFileList);
	return (have_class && have_archive);
}

bool osgEarth::CDBTile::CDB_Tile::Init_GT_Model_Tile(int sel)
{
#ifdef _DEBUG
	int fubar = 0;
#endif
	if (!m_DataFromGlobal)
	{
		if (!m_GTModelSet[sel].ClassTileOgr)
			return false;
		m_GTModelSet[sel].PrimaryLayer = m_GTModelSet[sel].PrimaryTileOgr->GetLayer(0);
		if (m_Use_Spatial_Rect)
		{
			m_GTModelSet[sel].PrimaryLayer->SetSpatialFilterRect(m_SpatialRectExtent.West, m_SpatialRectExtent.South, m_SpatialRectExtent.East, m_SpatialRectExtent.North);
		}
		m_GTModelSet[sel].FeatureSet.LoadFeatureSet(m_GTModelSet[sel].PrimaryLayer);
	}
	else
	{
		std::string LayerName = m_GTModelSet[sel].TilePrimaryShapeName;
		if (m_Globalcontype == ConnWFS)
		{
			LayerName = CDB_Global::getInstance()->ToWFSLayer(m_GTModelSet[sel].TilePrimaryShapeName);
		}

		m_GTModelSet[sel].PrimaryLayer = m_GlobalDataset->GetLayerByName(LayerName.c_str());
		if (m_Use_Spatial_Rect)
		{
			m_GTModelSet[sel].PrimaryLayer->SetSpatialFilterRect(m_SpatialRectExtent.West, m_SpatialRectExtent.South, m_SpatialRectExtent.East, m_SpatialRectExtent.North);
		}
		m_GTModelSet[sel].FeatureSet.LoadFeatureSet(m_GTModelSet[sel].PrimaryLayer);
		m_CurFeatureClass.Init();
#ifdef _DEBUG
		if (m_GlobalTile)
		{
			GDALDataset * poDataset = m_GlobalTile->Get_Dataset();
			if (poDataset)
			{
				OGRLayer * poLayer = poDataset->GetLayerByName(m_GTModelSet[sel].TilePrimaryShapeName.c_str());
				if (poLayer)
				{
					if (m_Use_Spatial_Rect)
					{
						poLayer->SetSpatialFilterRect(m_SpatialRectExtent.West, m_SpatialRectExtent.South, m_SpatialRectExtent.East, m_SpatialRectExtent.North);
					}
					m_GTModelSet[sel].DebugFeatureSet.LoadFeatureSet(poLayer);
				}
			}
			if (abs((int)m_GTModelSet[sel].FeatureSet.Size() - (int)m_GTModelSet[sel].DebugFeatureSet.Size()) > 1)
			{
				++fubar;
			}
		}
#endif
	}
	if (!m_GTModelSet[sel].PrimaryLayer)
		return false;


	//	m_GTModelSet[sel].PrimaryLayer->ResetReading();
	OGRLayer *poLayer = NULL;
	if (!m_DataFromGlobal)
	{
		poLayer = m_GTModelSet[sel].ClassTileOgr->GetLayer(0);
	}
	//	else
	//	{
	//		poLayer = m_GlobalDataset->GetLayerByName(m_GTModelSet[sel].TileSecondaryShapeName.c_str());
	//	}

	bool have_class = false;
	if (m_DataFromGlobal)
	{
		//CDB_Model_RuntimeMapP clsMap = m_GlobalTile->GetGTClassMap(m_CDB_LOD_Num, sel);
		//if (clsMap->size() == 0)
		//{
		//	have_class = Load_Class_Map(poLayer, *clsMap);
		//}
		//else
		have_class = true;
	}
	else
		have_class = Load_Class_Map(poLayer, m_GTModelSet[sel].clsMap);
	bool have_archive;
	if (m_DataFromGlobal)
	{
		std::string tablename = "gpkg:GTModelGeometry_Mda.zip";
		have_archive = Load_Archive(tablename, m_GTGeomerty_archiveFileList);
	}
	else
		have_archive = true;
	return have_class & have_archive;
}

bool osgEarth::CDBTile::CDB_Tile::Load_Archive(std::string ArchiveName, osgDB::Archive::FileNameList &archiveFileList)
{
	osg::ref_ptr<osgDB::Archive> ar = NULL;
	ar = osgDB::openArchive(ArchiveName, osgDB::ReaderWriter::ArchiveStatus::READ);
	if (ar)
	{
		ar->getFileNames(archiveFileList);
		ar.release();
		return true;
	}
	else
		return false;
}

std::string osgEarth::CDBTile::CDB_Tile::archive_validate_modelname(osgDB::Archive::FileNameList &archiveFileList, std::string &filename)
{
	std::string result = "";
	for (osgDB::Archive::FileNameList::const_iterator f = archiveFileList.begin(); f != archiveFileList.end(); ++f)
	{
		const std::string comp = *f;
		if (comp.find(filename) != std::string::npos)
		{
			return comp;
		}
	}
	return result;
}

bool osgEarth::CDBTile::CDB_Tile::Load_Class_Map(OGRLayer * poLayer, CDB_Model_RuntimeMap &clsMap)
{
	OGRFeatureDefn * poFDefn = poLayer->GetLayerDefn();
	int name_attr_index = Find_Field_Index(poFDefn, "MODL", OFTString);
	if (name_attr_index < 0)
	{
		return false;
	}

	int cnam_attr_index = Find_Field_Index(poFDefn, "CNAM", OFTString);
	if (cnam_attr_index < 0)
	{
		return false;
	}

	int facc_index = Find_Field_Index(poFDefn, "FACC", OFTString);
	if (facc_index < 0)
	{
		return false;
	}

	int fsc_index = Find_Field_Index(poFDefn, "FSC", OFTInteger);
#if 0
	if (fsc_index < 0)
	{
		return false;
	}
#endif

	int bsr_index = Find_Field_Index(poFDefn, "BSR", OFTReal);
	if (bsr_index < 0)
	{
		return false;
	}

	int bbw_index = Find_Field_Index(poFDefn, "BBW", OFTReal);
	if (bbw_index < 0)
	{
		return false;
	}

	int bbl_index = Find_Field_Index(poFDefn, "BBL", OFTReal);
	if (bbl_index < 0)
	{
		return false;
	}

	int bbh_index = Find_Field_Index(poFDefn, "BBH", OFTReal);
	if (bbh_index < 0)
	{
		return false;
	}

	int ahgt_index = Find_Field_Index(poFDefn, "AHGT", OFTInteger);

	poLayer->ResetReading();
	OGRFeature* dbf_feature;
	while ((dbf_feature = poLayer->GetNextFeature()) != NULL)
	{
		CDB_Model_Runtime_Class nextEntry;
		std::string Key = nextEntry.set_class(dbf_feature, cnam_attr_index, name_attr_index, facc_index, fsc_index, bsr_index, bbw_index, bbl_index, bbh_index, ahgt_index);
		clsMap.insert(std::pair<std::string, CDB_Model_Runtime_Class>(Key, nextEntry));
		OGRFeature::DestroyFeature(dbf_feature);
	}
	return true;
}

int osgEarth::CDBTile::CDB_Tile::Find_Field_Index(OGRFeatureDefn *poFDefn, std::string fieldname, OGRFieldType Type)
{
	int dbfieldcnt = poFDefn->GetFieldCount();
	for (int dbffieldIdx = 0; dbffieldIdx < dbfieldcnt; ++dbffieldIdx)
	{
		OGRFieldDefn *po_FieldDefn = poFDefn->GetFieldDefn(dbffieldIdx);
		std::string thisname = po_FieldDefn->GetNameRef();
		if (thisname.compare(fieldname) == 0)
		{
			if (po_FieldDefn->GetType() != Type)
			{
				if (CDB_Global::getInstance()->CDB_Tile_Be_Verbose())
				{
					OSG_WARN << "Attribure " << fieldname << " is not field of type " << Type << " found " << po_FieldDefn->GetType() << std::endl;
				}
			}
			return dbffieldIdx;
		}
	}
	return -1;
}


int osgEarth::CDBTile::CDB_Tile::Model_Sel_Count(void)
{
	if (m_TileType == GeoSpecificModel)
		return (int)m_ModelSet.size();
	else if (m_TileType == GeoTypicalModel)
		return (int)m_GTModelSet.size();
	else
		return -1;
}

bool osgEarth::CDBTile::CDB_Tile::Read(void)
{
	if (!m_GDAL.poDataset && !m_GDAL.soDataset && !m_GDAL.so2Dataset)
		return false;

	if (m_Tile_Status == Loaded)
		return true;


	if ((m_TileType == Imagery) || (m_TileType == ImageryCache))
	{
		if (!m_Subordinate_Exists && !m_Subordinate2_Exists)
		{
			CPLErr gdal_err = m_GDAL.poDataset->RasterIO(GF_Read, 0, 0, m_Pixels.pixX, m_Pixels.pixY,
				m_GDAL.reddata, m_Pixels.pixX, m_Pixels.pixY, GDT_Byte, 3, NULL, 0, 0, 0);
			if (gdal_err == CE_Failure)
			{
				return false;
			}
		}
		else if (m_Subordinate_Exists)
		{
			CPLErr gdal_err = m_GDAL.poDataset->RasterIO(GF_Read, 0, 0, m_Pixels.pixX, m_Pixels.pixY,
														 m_GDAL.lightmapdatar, m_Pixels.pixX, m_Pixels.pixY, GDT_Byte, 3, NULL, 0, 0, 0);

			if (gdal_err == CE_Failure)
			{
				return false;
			}
		}
		else if (m_Subordinate2_Exists)
		{
			GDALRasterBand * MaterialBand = m_GDAL.so2Dataset->GetRasterBand(1);

			CPLErr gdal_err = MaterialBand->RasterIO(GF_Read, 0, 0, m_Pixels.pixX, m_Pixels.pixY,
											  m_GDAL.materialdata, m_Pixels.pixX, m_Pixels.pixY, GDT_Byte, 0, 0);
			if (gdal_err == CE_Failure)
			{
				return false;
			}
			m_Have_MaterialData = true;

			if (m_EnableMaterialMask)
			{
				m_Have_MaterialMaskData = false;
				int maskbandnum = m_GDAL.so2Dataset->GetRasterCount();
				if (maskbandnum > 1)
				{
					GDALRasterBand * MaterialMaskBand = m_GDAL.so2Dataset->GetRasterBand(maskbandnum);
					CPLErr gdal_err = MaterialMaskBand->RasterIO(GF_Read, 0, 0, m_Pixels.pixX, m_Pixels.pixY,
													     m_GDAL.materialmaskdata, m_Pixels.pixX, m_Pixels.pixY, GDT_Byte, 0, 0);
					if (gdal_err == CE_Failure)
					{
						return false;
					}
					m_Have_MaterialMaskData = true;
				}

			}
		}

	}
	else if ((m_TileType == Elevation) || (m_TileType == ElevationCache))
	{
		GDALRasterBand * ElevationBand = m_GDAL.poDataset->GetRasterBand(1);

		CPLErr gdal_err = ElevationBand->RasterIO(GF_Read, 0, 0, m_Pixels.pixX, m_Pixels.pixY,
			                                      m_GDAL.elevationdata, m_Pixels.pixX, m_Pixels.pixY, GDT_Float32, 0, 0);
		if (gdal_err == CE_Failure)
		{
			return false;
		}
		if (m_Subordinate_Exists)
		{
			if (!m_GDAL.soDataset)
				return false;

			GDALRasterBand * SubordElevationBand = m_GDAL.soDataset->GetRasterBand(1);

			gdal_err = SubordElevationBand->RasterIO(GF_Read, 0, 0, m_Pixels.pixX, m_Pixels.pixY,
													 m_GDAL.subord_elevationdata, m_Pixels.pixX, m_Pixels.pixY, GDT_Float32, 0, 0);
			if (gdal_err == CE_Failure)
			{
				return false;
			}
		}
	}
	m_Tile_Status = Loaded;

	return true;
}

void osgEarth::CDBTile::CDB_Tile::Fill_Tile(void)
{
	int buffsz = m_Pixels.pixX * m_Pixels.pixY;
	if (m_TileType == Imagery)
	{
		if (!m_Subordinate_Exists && m_Subordinate2_Exists)
		{
			for (int i = 0; i < buffsz; ++i)
			{
				m_GDAL.reddata[i] = 127;
				m_GDAL.greendata[i] = 127;
				m_GDAL.bluedata[i] = 127;
			}
		}
		else if (m_Subordinate_Exists)
		{
			for (int i = 0; i < buffsz; ++i)
			{
				m_GDAL.lightmapdatar[i] = 0;
				m_GDAL.lightmapdatag[i] = 0;
				m_GDAL.lightmapdatab[i] = 0;
			}
		}
		else if (m_Subordinate2_Exists)
		{
			for (int i = 0; i < buffsz; ++i)
			{
				m_GDAL.materialdata[i] = 0;
			}
			if (m_EnableMaterialMask)
			{
				for (int i = 0; i < buffsz; ++i)
				{
					m_GDAL.materialmaskdata[i] = 0;
				}
			}
		}
	}
	else if ((m_TileType == Elevation) || (m_TileType == ElevationCache))
	{
		for (int i = 0; i < buffsz; ++i)
		{
			m_GDAL.elevationdata[i] = 0.0f;
		}
		if (m_Subordinate_Tile)
		{
			for (int i = 0; i < buffsz; ++i)
			{
				m_GDAL.subord_elevationdata[i] = 0.0f;
			}
		}

	}
	else if (m_TileType == ImageryCache)
	{
		if (!m_Subordinate_Exists && !m_Subordinate2_Exists)
		{
			for (int i = 0; i < buffsz; ++i)
			{
				m_GDAL.reddata[i] = 0;
				m_GDAL.greendata[i] = 0;
				m_GDAL.bluedata[i] = 0;
			}
		}
		else if (m_Subordinate_Exists)
		{
			for (int i = 0; i < buffsz; ++i)
			{
				m_GDAL.lightmapdatar[i] = 0;
				m_GDAL.lightmapdatag[i] = 0;
				m_GDAL.lightmapdatab[i] = 0;
			}
		}
		else if (m_Subordinate2_Exists)
		{
			for (int i = 0; i < buffsz; ++i)
			{
				m_GDAL.materialdata[i] = 0;
			}
			if (m_EnableMaterialMask)
			{
				for (int i = 0; i < buffsz; ++i)
				{
					m_GDAL.materialmaskdata[i] = 0;
				}
			}
		}
	}
}

bool osgEarth::CDBTile::CDB_Tile::Tile_Exists(int sel)
{
	if(sel < 0)
		return m_FileExists;
	else if (m_TileType == GeoSpecificModel)
	{
		if (m_FileExists && m_ModelSet[sel].ModelDbfNameExists && m_ModelSet[sel].ModelGeometryNameExists)
			return true;
		else
			return false;
	}
	else if (m_TileType == GeoTypicalModel)
	{
		if (m_GTModelSet[sel].PrimaryExists && m_GTModelSet[sel].ClassExists)
			return true;
		else
			return false;
	}
	return m_FileExists;
}

bool osgEarth::CDBTile::CDB_Tile::Has_Mask_Data(void)
{
	return m_Have_MaterialMaskData;
}

bool osgEarth::CDBTile::CDB_Tile::Has_Material_Data(void)
{
	return m_Have_MaterialData;
}

bool osgEarth::CDBTile::CDB_Tile::Subordinate_Exists(void)
{
	return m_Subordinate_Exists;
}

bool osgEarth::CDBTile::CDB_Tile::Subordinate2_Exsits(void)
{
	return m_Subordinate2_Exists;
}

void osgEarth::CDBTile::CDB_Tile::Set_Subordinate(bool value)
{
	m_Subordinate_Tile = value;
}

bool osgEarth::CDBTile::CDB_Tile::Build_Cache_Tile(bool save_cache)
{
	//This is not actually part of the CDB specification but
	//necessary to support an osgEarth global profile
	//Build a list of the tiles to use for this cache tile

	double MinLat = m_TileExtent.South;
	double MinLon = m_TileExtent.West;
	double MaxLat = m_TileExtent.North;
	double MaxLon = m_TileExtent.East;

	CDB_Tile_Type subTileType;
	if (m_TileType == ImageryCache)
	{
		subTileType = Imagery;
	}
	else if (m_TileType == ElevationCache)
	{
		subTileType = Elevation;
	}
	CDB_TilePV Tiles;

	bool done = false;
	CDB_Tile_Extent thisTileExtent;
	thisTileExtent.West = MinLon;
	thisTileExtent.South = MinLat;
	double lonstep = Get_Lon_Step(thisTileExtent.South);
	double sign;
	while (!done)
	{
		if (lonstep != 1.0)
		{
			int checklon = (int)abs(thisTileExtent.West);
			if (checklon % (int)lonstep)
			{
				sign = thisTileExtent.West < 0.0 ? -1.0 : 1.0;
				checklon = (checklon / (int)lonstep) * (int)lonstep;
				thisTileExtent.West = (double)checklon * sign;
				if (sign < 0.0)
					thisTileExtent.West -= lonstep;
			}
		}
		thisTileExtent.East = thisTileExtent.West + lonstep;
		thisTileExtent.North = thisTileExtent.South + 1.0;

		CDB_TileP LodTile = new CDB_Tile(m_cdbRootDir, m_cdbCacheDir, subTileType, m_DataSet, &thisTileExtent, m_EnableLightMap, m_EnableMaterials, m_EnableMaterialMask, m_CDB_LOD_Num);

		if (LodTile->Tile_Exists())
		{
			Tiles.push_back(LodTile);
		}
		else
			delete LodTile;

		thisTileExtent.West += lonstep;
		if (thisTileExtent.West > MaxLon)
		{
			thisTileExtent.West = MinLon;
			thisTileExtent.South += 1.0;
			lonstep = Get_Lon_Step(thisTileExtent.South);
			if (thisTileExtent.South > MaxLat)
				done = true;
		}
	}

	int tilecnt = (int)Tiles.size();
	if (tilecnt <= 0)
		return false;

	Build_From_Tiles(&Tiles);

	if (save_cache && (m_Tile_Status == Loaded))
	{
		Save();
		m_FileExists = true;
	}

	//Clean up
	for each (CDB_TileP Tile in Tiles)
	{
		if (Tile)
		{
			delete Tile;
			Tile = NULL;
		}
	}
	Tiles.clear();

	return true;
}

CDB_Tile_Extent osgEarth::CDBTile::CDB_Tile::Actual_Extent_For_Tile(CDB_Tile_Extent &TileExtent)
{
	double MinLon = TileExtent.West;

	double lonstep = Get_Lon_Step(TileExtent.South);
	int CDB_LOD_Num = LodNumFromExtent(TileExtent);
	lonstep /= Gbl_CDB_Tiles_Per_LOD[CDB_LOD_Num];

	double incrs = floor(MinLon / lonstep);
	double test = incrs * lonstep;
	if (test != MinLon)
	{
		MinLon = test;
	}

	CDB_Tile_Extent thisTileExtent;
	thisTileExtent.West = MinLon;
	thisTileExtent.South = TileExtent.South;
	thisTileExtent.East = thisTileExtent.West + lonstep;
	thisTileExtent.North = TileExtent.North;

	return thisTileExtent;
}

bool osgEarth::CDBTile::CDB_Tile::Set_SpatialFilter_Extent(CDB_Tile_Extent &SpatialExtent)
{
	m_SpatialRectExtent = SpatialExtent;
	m_Use_Spatial_Rect = true;
	return true;
}

bool osgEarth::CDBTile::CDB_Tile::Build_Earth_Tile(void)
{
	//Build an Earth Profile tile for Latitudes above and below 50 deg

	double MinLon = m_TileExtent.West;

	CDB_Tile_Type subTileType = m_TileType;

	CDB_TilePV Tiles;

	bool done = false;

	OE_DEBUG << "Build_Earth_Tile with " << m_FileName.c_str() << std::endl;
	OE_DEBUG << "Build_Earth_Tile my extent " << m_TileExtent.North << " " <<m_TileExtent.South << " " << m_TileExtent.East << " " << m_TileExtent.West << std::endl;

	double lonstep = Get_Lon_Step(m_TileExtent.South);
	lonstep /= Gbl_CDB_Tiles_Per_LOD[m_CDB_LOD_Num];

	double incrs = floor(MinLon / lonstep);
	double test = incrs * lonstep;
	if (test != MinLon)
	{
		MinLon = test;
	}

	CDB_Tile_Extent thisTileExtent;
	thisTileExtent.West = MinLon;
	thisTileExtent.South = m_TileExtent.South;
	thisTileExtent.East = thisTileExtent.West + lonstep;
	thisTileExtent.North = m_TileExtent.North;

	OE_DEBUG << "Build_Earth_Tile cdb extent " << thisTileExtent.North << " " << thisTileExtent.South << " " << thisTileExtent.East << " " << thisTileExtent.West << std::endl;

	//Now get the actual cdb tile with the correct CDB extents
	CDB_TileP LodTile = new CDB_Tile(m_cdbRootDir, m_cdbCacheDir, subTileType, m_DataSet, &thisTileExtent, m_EnableLightMap, m_EnableMaterials, m_EnableMaterialMask);

	OE_DEBUG << "Build_Earth_Tile cdb tile " << LodTile->FileName().c_str() << std::endl;

	if (LodTile->Tile_Exists())
	{
		if (LodTile->Subordinate_Exists())
			m_Subordinate_Tile = true;
		OE_DEBUG << "CDB_Tile found " << LodTile->FileName().c_str() << std::endl;
		Tiles.push_back(LodTile);
	}
	else
		delete LodTile;

	int tilecnt = (int)Tiles.size();
	//if the tile does not exist
	//there is no tile to build
	if (tilecnt <= 0)
		return false;

	//Build the osgearth tile from the cdb tile
	Build_From_Tiles(&Tiles, true);

	//clean up
	for each (CDB_TileP Tile in Tiles)
	{
		if (Tile)
		{
			delete Tile;
			Tile = NULL;
		}
	}
	Tiles.clear();

	return true;
}

bool osgEarth::CDBTile::CDB_Tile::Get_BaseMap_Files(std::string rootDir, CDB_Tile_Extent &Extent, std::vector<std::string> &files)
{
	//
	//	Make Extent a CDB GeoCell
	// 
	CDB_Tile_Extent L0_Extent;
	L0_Extent.South = round(Extent.South);
	L0_Extent.North = L0_Extent.South + 1.0;
	L0_Extent.West = round(Extent.West);
	L0_Extent.East = L0_Extent.West + Get_Lon_Step(Extent.South);
	std::string dataset = "_S001_T001_";
	std::string CacheDir = "";

	CDB_Tile_Type Type = GeoPackageMap;
	CDB_Global * gbls = CDB_Global::getInstance();
	int BaseNumSave = gbls->BaseMapLodNum();
	gbls->Set_BaseMapLodNum(3);
	CDB_Tile * LOD0BM = new CDB_Tile(rootDir, CacheDir, Type, dataset, &L0_Extent, false, false, false);
	gbls->Set_BaseMapLodNum(BaseNumSave);
	bool ret = true;
	if (LOD0BM->Tile_Exists())
	{
		std::string xmlFileName = LOD0BM->FileName();
		osgEarth::XmlDocument * xmld = new osgEarth::XmlDocument();
		osgEarth::XmlDocument * xmlData = xmld->load(xmlFileName);
		if (xmlData)
		{
			osgEarth::XmlElement * LODE = (osgEarth::XmlElement *)xmlData->findElement("CDB_Map_Lod");
			if (LODE)
			{
				std::string LODstr = LODE->getText();
				int baseLod = atoi(LODstr.c_str());
				CDB_Global::getInstance()->Set_BaseMapLodNum(baseLod);
				//Adjust extent to be on baseMap lod tile boundary
				double degpertileY = 1.0 / Gbl_CDB_Tiles_Per_LOD[baseLod];
				double degpertileX = Get_Lon_Step(Extent.South) / Gbl_CDB_Tiles_Per_LOD[baseLod];
				bool done = false;
				CDB_Tile_Extent T;
				T.South = Extent.South;
				T.West = Extent.West;
				while (!done)
				{
					T.North = T.South + degpertileY;
					T.East = T.West + degpertileX;
					CDB_Tile * MapTile = new CDB_Tile(rootDir, CacheDir, Type, dataset, &T, false, false, false);
					if (MapTile->Tile_Exists())
					{
						files.push_back(MapTile->FileName());
					}
					delete MapTile;
					T.West += degpertileX;
					if (T.West >= Extent.East)
					{
						T.West = Extent.West;
						T.South += degpertileY;
						if (T.South >= Extent.North)
							done = true;
					}
				}
			}
		}
		delete xmlData;
		delete xmld;
	}
	else
		ret = false;

	delete LOD0BM;

	return ret;
}

double osgEarth::CDBTile::CDB_Tile::Get_Lon_Step(double Latitude)
{
	double test = abs(Latitude);
	double step;
	if (Latitude >= 0.0)
	{
		if (test < 50.0)
			step = 1.0;
		else if (test >= 50.0 && test < 70.0)
			step = 2.0;
		else if (test >= 70.0 && test < 75.0)
			step = 3.0;
		else if (test >= 75.0 && test < 80.0)
			step = 4.0;
		else if (test >= 80.0 && test < 89.0)
			step = 6.0;
		else
			step = 12.0;
	}
	else
	{
		if (test <= 50.0)
			step = 1.0;
		else if (test > 50.0 && test <= 70.0)
			step = 2.0;
		else if (test > 70.0 && test <= 75.0)
			step = 3.0;
		else if (test > 75.0 && test <= 80.0)
			step = 4.0;
		else if (test > 80.0 && test <= 89.0)
			step = 6.0;
		else
			step = 12.0;

	}
	return step;
}

void osgEarth::CDBTile::CDB_Tile::Disable_Bathyemtry(bool value)
{
	CDB_Global * gbls = CDB_Global::getInstance();
	if (value)
		gbls->Set_EnableBathymetry(false);
	else
		gbls->Set_EnableBathymetry(true);
}


void osgEarth::CDBTile::CDB_Tile::Set_LOD0_GS_Stack(bool value)
{
	CDB_Global * gbls = CDB_Global::getInstance();
	if (value)
		gbls->Set_LOD0_GS_FullStack(true);
	else
		gbls->Set_LOD0_GS_FullStack(false);
}

void osgEarth::CDBTile::CDB_Tile::Set_LOD0_GT_Stack(bool value)
{
	CDB_Global * gbls = CDB_Global::getInstance();
	if (value)
		gbls->Set_LOD0_GT_FullStack(true);
	else
		gbls->Set_LOD0_GT_FullStack(false);
}

void osgEarth::CDBTile::CDB_Tile::Set_Verbose(bool value)
{
	CDB_Global * gbls = CDB_Global::getInstance();
	if (value)
		gbls->Set_CDB_Tile_Be_Verbose(true);
	else
		gbls->Set_CDB_Tile_Be_Verbose(false);
}

void osgEarth::CDBTile::CDB_Tile::Set_Use_Gpkg_For_Features(bool value)
{
	CDB_Global* gbls = CDB_Global::getInstance();
	if(value)
		gbls->Set_Use_GeoPackage_Features(true);
	else
		gbls->Set_Use_GeoPackage_Features(false);
}

bool osgEarth::CDBTile::CDB_Tile::Initialize_Tile_Drivers(std::string &ErrorMsg)
{
	ErrorMsg = "";
	if (Gbl_TileDrivers.cdb_drivers_initialized)
		return true;

	std::string	cdb_JP2DriverNames[JP2DRIVERCNT];
	//The JP2 Driver Names should be ordered based on read performance however this has not been done yet.
	cdb_JP2DriverNames[1] = "JP2ECW";		//ERDAS supplied JP2 Plugin
	cdb_JP2DriverNames[0] = "JP2OpenJPEG";  //LibOpenJPEG2000
	cdb_JP2DriverNames[2] = "JPEG2000";	    //JASPER
	cdb_JP2DriverNames[3] = "JP2KAK";		//Kakadu Library
	cdb_JP2DriverNames[4] = "JP2MrSID";	    //MR SID SDK

	//Find a jpeg2000 driver for the image layer.
	int dcount = 0;
	while ((Gbl_TileDrivers.cdb_JP2Driver == NULL) && (dcount < JP2DRIVERCNT))
	{
		Gbl_TileDrivers.cdb_JP2Driver = GetGDALDriverManager()->GetDriverByName(cdb_JP2DriverNames[dcount].c_str());
		if (Gbl_TileDrivers.cdb_JP2Driver == NULL)
			++dcount;
		else if (Gbl_TileDrivers.cdb_JP2Driver->pfnOpen == NULL)
		{
			Gbl_TileDrivers.cdb_JP2Driver = NULL;
			++dcount;
		}
	}
	if (Gbl_TileDrivers.cdb_JP2Driver == NULL)
	{
		ErrorMsg = "No GDAL JP2 Driver Found";
		return false;
	}

	//Get the GeoTiff driver for the Elevation data
	Gbl_TileDrivers.cdb_GTIFFDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (Gbl_TileDrivers.cdb_GTIFFDriver == NULL)
	{
		ErrorMsg = "GDAL GeoTiff Driver Not Found";
		return false;
	}
	else if (Gbl_TileDrivers.cdb_GTIFFDriver->pfnOpen == NULL)
	{
		ErrorMsg = "GDAL GeoTiff Driver has no open function";
		return false;
	}

	//The Erdas Imagine dirver is currently being used for the
	//Elevation cache files
	Gbl_TileDrivers.cdb_HFADriver = GetGDALDriverManager()->GetDriverByName("HFA");
	if (Gbl_TileDrivers.cdb_HFADriver == NULL)
	{
		ErrorMsg = "GDAL ERDAS Imagine Driver Not Found";
		return false;
	}
	else if (Gbl_TileDrivers.cdb_HFADriver->pfnOpen == NULL)
	{
		ErrorMsg = "GDAL ERDAS Imagine Driver has no open function";
		return false;
	}

	Gbl_TileDrivers.cdb_ShapefileDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
	if (Gbl_TileDrivers.cdb_ShapefileDriver == NULL)
	{
		ErrorMsg = "GDAL ESRI Shapefile Driver Not Found";
		return false;
	}
	else if (Gbl_TileDrivers.cdb_ShapefileDriver->pfnOpen == NULL)
	{
		ErrorMsg = "GDAL ESRI Shapefile Driver has no open function";
		return false;
	}

	Gbl_TileDrivers.cdb_GeoPackageDriver = GetGDALDriverManager()->GetDriverByName("GPKG");
	if (Gbl_TileDrivers.cdb_GeoPackageDriver == NULL)
	{
		ErrorMsg = "GeoPackageDriver Driver Not Found";
		return false;
	}
	else if (Gbl_TileDrivers.cdb_GeoPackageDriver->pfnOpen == NULL)
	{
		ErrorMsg = "GDAL GeoPackageDriver Driver has no open function";
		return false;
	}

	Gbl_TileDrivers.cdb_drivers_initialized = true;
	return true;
}

osgEarth::CDBTile::Image_Contrib osgEarth::CDBTile::CDB_Tile::Get_Contribution(CDB_Tile_Extent &TileExtent)
{
	//Does the Image fall entirly within the Tile
	Image_Contrib retval = Image_Is_Inside_Tile(TileExtent);
	if (retval != None)
		return retval;

	//Check the tile contents against the Image
	if ((TileExtent.North < m_TileExtent.South) || (TileExtent.South > m_TileExtent.North) ||
		(TileExtent.East < m_TileExtent.West) || (TileExtent.West > m_TileExtent.East))
		retval = None;
	else if ((TileExtent.North <= m_TileExtent.North) && (TileExtent.South >= m_TileExtent.South) &&
		(TileExtent.East <= m_TileExtent.East) && (TileExtent.West >= m_TileExtent.West))
	{
		retval = Full;
	}
	else
		retval = Partial;

	return retval;
}

osgEarth::CDBTile::Image_Contrib osgEarth::CDBTile::CDB_Tile::Image_Is_Inside_Tile(CDB_Tile_Extent &TileExtent)
{
	int count = 0;
	coord2d point;
	point.Xpos = m_TileExtent.West;
	point.Ypos = m_TileExtent.North;
	if (Point_is_Inside_Tile(point, TileExtent))
		++count;

	point.Xpos = m_TileExtent.East;
	if (Point_is_Inside_Tile(point, TileExtent))
		++count;

	point.Ypos = m_TileExtent.South;
	if (Point_is_Inside_Tile(point, TileExtent))
		++count;

	point.Xpos = m_TileExtent.West;
	if (Point_is_Inside_Tile(point, TileExtent))
		++count;

	if (count > 0)
		return Partial;
	else
		return None;
}

bool osgEarth::CDBTile::CDB_Tile::Point_is_Inside_Tile(coord2d &Point, CDB_Tile_Extent &TileExtent)
{
	if ((Point.Xpos < TileExtent.East) && (Point.Xpos > TileExtent.West) && (Point.Ypos > TileExtent.South) && (Point.Ypos < TileExtent.North))
		return true;
	else
		return false;
}

bool osgEarth::CDBTile::CDB_Tile::Load_Tile(void)
{
	if (m_Tile_Status == Loaded)
		return true;

	if (!m_FileExists)
		return false;

	Allocate_Buffers();

	if (!Open_Tile())
		return false;

	if (!Read())
		return false;

	return true;
}

coord2d osgEarth::CDBTile::CDB_Tile::LL2Pix(coord2d LLPoint)
{
	coord2d PixCoord;
	if ((m_Tile_Status == Loaded) || (m_Tile_Status == Opened))
	{
		double xRel = LLPoint.Xpos - m_TileExtent.West;
		double yRel = m_TileExtent.North - LLPoint.Ypos;
		PixCoord.Xpos = xRel / m_GDAL.adfGeoTransform[GEOTRSFRM_WE_RES];
		PixCoord.Ypos = yRel / abs(m_GDAL.adfGeoTransform[GEOTRSFRM_NS_RES]);
	}
	else
	{
		PixCoord.Xpos = -1.0;
		PixCoord.Ypos = -1.0;
	}
	return PixCoord;
}

bool osgEarth::CDBTile::CDB_Tile::Get_Lightmap_Pixel(coord2d ImPix, unsigned char &RedPix, unsigned char &GreenPix, unsigned char &BluePix)
{
	int tx = (int)ImPix.Xpos;
	int ty = (int)ImPix.Ypos;

	if ((tx < 0) || (tx > m_Pixels.pixX - 1) || (ty < 0) || (ty > m_Pixels.pixY - 1))
	{
		return false;
	}

	int bpos1 = (((int)ImPix.Ypos) * m_Pixels.pixX) + (int)ImPix.Xpos;
	int bpos2 = bpos1 + 1;
	int bpos3 = bpos1 + (int)m_Pixels.pixX;
	int bpos4 = bpos3 + 1;

	if (tx == m_Pixels.pixX - 1)
	{
		bpos2 = bpos1;
		bpos4 = bpos3;
	}

	if (ty == m_Pixels.pixY - 1)
	{
		bpos3 = bpos1;
		if (tx == m_Pixels.pixX - 1)
			bpos4 = bpos1;
		else
			bpos4 = bpos2;
	}

	float rat2 = (float)(ImPix.Xpos - double(tx));
	float rat1 = 1.0f - rat2;
	float rat4 = (float)(ImPix.Ypos - double(ty));
	float rat3 = 1.0f - rat4;

	float p1p = ((float)m_GDAL.lightmapdatar[bpos1] * rat1) + ((float)m_GDAL.lightmapdatar[bpos2] * rat2);
	float p2p = ((float)m_GDAL.lightmapdatar[bpos3] * rat1) + ((float)m_GDAL.lightmapdatar[bpos4] * rat2);
	float red = (p1p * rat3) + (p2p * rat4);

	p1p = ((float)m_GDAL.lightmapdatag[bpos1] * rat1) + ((float)m_GDAL.lightmapdatag[bpos2] * rat2);
	p2p = ((float)m_GDAL.lightmapdatag[bpos3] * rat1) + ((float)m_GDAL.lightmapdatag[bpos4] * rat2);
	float green = (p1p * rat3) + (p2p * rat4);

	p1p = ((float)m_GDAL.lightmapdatab[bpos1] * rat1) + ((float)m_GDAL.lightmapdatab[bpos2] * rat2);
	p2p = ((float)m_GDAL.lightmapdatab[bpos3] * rat1) + ((float)m_GDAL.lightmapdatab[bpos4] * rat2);
	float blue = (p1p * rat3) + (p2p * rat4);

	red = round(red) < 255.0f ? round(red) : 255.0f;
	green = round(green) < 255.0f ? round(green) : 255.0f;
	blue = round(blue) < 255.0f ? round(blue) : 255.0f;

	RedPix = (unsigned char)red;
	GreenPix = (unsigned char)green;
	BluePix = (unsigned char)blue;
	return true;

}

bool osgEarth::CDBTile::CDB_Tile::Get_Image_Pixel(coord2d ImPix, unsigned char &RedPix, unsigned char &GreenPix, unsigned char &BluePix)
{
	int tx = (int)ImPix.Xpos;
	int ty = (int)ImPix.Ypos;

	if ((tx < 0) || (tx > m_Pixels.pixX - 1) || (ty < 0) || (ty > m_Pixels.pixY - 1))
	{
		return false;
	}

	int bpos1 = (((int)ImPix.Ypos) * m_Pixels.pixX) + (int)ImPix.Xpos;
	int bpos2 = bpos1 + 1;
	int bpos3 = bpos1 + (int)m_Pixels.pixX;
	int bpos4 = bpos3 + 1;

	if (tx == m_Pixels.pixX - 1)
	{
		bpos2 = bpos1;
		bpos4 = bpos3;
	}

	if (ty == m_Pixels.pixY - 1)
	{
		bpos3 = bpos1;
		if (tx == m_Pixels.pixX - 1)
			bpos4 = bpos1;
		else
			bpos4 = bpos2;
	}

	float rat2 = (float)(ImPix.Xpos - double(tx));
	float rat1 = 1.0f - rat2;
	float rat4 = (float)(ImPix.Ypos - double(ty));
	float rat3 = 1.0f - rat4;

	float p1p = ((float)m_GDAL.reddata[bpos1] * rat1) + ((float)m_GDAL.reddata[bpos2] * rat2);
	float p2p = ((float)m_GDAL.reddata[bpos3] * rat1) + ((float)m_GDAL.reddata[bpos4] * rat2);
	float red = (p1p * rat3) + (p2p * rat4);

	p1p = ((float)m_GDAL.greendata[bpos1] * rat1) + ((float)m_GDAL.greendata[bpos2] * rat2);
	p2p = ((float)m_GDAL.greendata[bpos3] * rat1) + ((float)m_GDAL.greendata[bpos4] * rat2);
	float green = (p1p * rat3) + (p2p * rat4);

	p1p = ((float)m_GDAL.bluedata[bpos1] * rat1) + ((float)m_GDAL.bluedata[bpos2] * rat2);
	p2p = ((float)m_GDAL.bluedata[bpos3] * rat1) + ((float)m_GDAL.bluedata[bpos4] * rat2);
	float blue = (p1p * rat3) + (p2p * rat4);

	red = round(red) < 255.0f ? round(red) : 255.0f;
	green = round(green) < 255.0f ? round(green) : 255.0f;
	blue = round(blue) < 255.0f ? round(blue) : 255.0f;

	RedPix = (unsigned char)red;
	GreenPix = (unsigned char)green;
	BluePix = (unsigned char)blue;
	return true;

}

bool osgEarth::CDBTile::CDB_Tile::Get_Material_Pixel(coord2d ImPix, unsigned char &MatPix)
{
	int tx = (int)ImPix.Xpos;
	int ty = (int)ImPix.Ypos;

	if ((tx < 0) || (tx > m_Pixels.pixX - 1) || (ty < 0) || (ty > m_Pixels.pixY - 1))
	{
		return false;
	}

	int bpos1 = (((int)round(ImPix.Ypos)) * m_Pixels.pixX) + (int)round(ImPix.Xpos);

	MatPix = m_GDAL.materialdata[bpos1];
	return true;
}

bool osgEarth::CDBTile::CDB_Tile::Get_Mask_Pixel(coord2d ImPix, unsigned char &MaskPix)
{
	int tx = (int)ImPix.Xpos;
	int ty = (int)ImPix.Ypos;

	if ((tx < 0) || (tx > m_Pixels.pixX - 1) || (ty < 0) || (ty > m_Pixels.pixY - 1))
	{
		return false;
	}

	int bpos1 = (((int)round(ImPix.Ypos)) * m_Pixels.pixX) + (int)round(ImPix.Xpos);

	MaskPix = m_GDAL.materialmaskdata[bpos1];

	return true;
}

bool osgEarth::CDBTile::CDB_Tile::Get_Elevation_Pixel(coord2d ImPix, float &ElevationPix)
{
	int tx = (int)ImPix.Xpos;
	int ty = (int)ImPix.Ypos;

	if ((tx < 0) || (tx > m_Pixels.pixX - 1) || (ty < 0) || (ty > m_Pixels.pixY - 1))
	{
		return false;
	}

	int bpos1 = (((int)ImPix.Ypos) * m_Pixels.pixX) + (int)ImPix.Xpos;
	int bpos2 = bpos1 + 1;
	int bpos3 = bpos1 + (int)m_Pixels.pixX;
	int bpos4 = bpos3 + 1;

	if (tx == m_Pixels.pixX - 1)
	{
		bpos2 = bpos1;
		bpos4 = bpos3;
	}

	if (ty == m_Pixels.pixY - 1)
	{
		bpos3 = bpos1;
		if (tx == m_Pixels.pixX - 1)
			bpos4 = bpos1;
		else
			bpos4 = bpos2;
	}

	float rat2 = (float)(ImPix.Xpos - double(tx));
	float rat1 = 1.0f - rat2;
	float rat4 = (float)(ImPix.Ypos - double(ty));
	float rat3 = 1.0f - rat4;

	float e1p = (m_GDAL.elevationdata[bpos1] * rat1) + (m_GDAL.elevationdata[bpos2] * rat2);
	float e2p = (m_GDAL.elevationdata[bpos3] * rat1) + (m_GDAL.elevationdata[bpos4] * rat2);
	float elevation = (e1p * rat3) + (e2p * rat4);

	ElevationPix = elevation;
	return true;
}

bool osgEarth::CDBTile::CDB_Tile::Get_Subordinate_Elevation_Pixel(coord2d ImPix, float &ElevationPix)
{
	int tx = (int)ImPix.Xpos;
	int ty = (int)ImPix.Ypos;

	if ((tx < 0) || (tx > m_Pixels.pixX - 1) || (ty < 0) || (ty > m_Pixels.pixY - 1))
	{
		return false;
	}

	int bpos1 = (((int)ImPix.Ypos) * m_Pixels.pixX) + (int)ImPix.Xpos;
	int bpos2 = bpos1 + 1;
	int bpos3 = bpos1 + (int)m_Pixels.pixX;
	int bpos4 = bpos3 + 1;

	if (tx == m_Pixels.pixX - 1)
	{
		bpos2 = bpos1;
		bpos4 = bpos3;
	}

	if (ty == m_Pixels.pixY - 1)
	{
		bpos3 = bpos1;
		if (tx == m_Pixels.pixX - 1)
			bpos4 = bpos1;
		else
			bpos4 = bpos2;
	}

	float rat2 = (float)(ImPix.Xpos - double(tx));
	float rat1 = 1.0f - rat2;
	float rat4 = (float)(ImPix.Ypos - double(ty));
	float rat3 = 1.0f - rat4;

	float e1p = (m_GDAL.subord_elevationdata[bpos1] * rat1) + (m_GDAL.subord_elevationdata[bpos2] * rat2);
	float e2p = (m_GDAL.subord_elevationdata[bpos3] * rat1) + (m_GDAL.subord_elevationdata[bpos4] * rat2);
	float elevation = (e1p * rat3) + (e2p * rat4);

	ElevationPix = elevation;
	return true;
}

double osgEarth::CDBTile::CDB_Tile::West(void)
{
	return m_TileExtent.West;
}

double osgEarth::CDBTile::CDB_Tile::East(void)
{
	return m_TileExtent.East;
}

double osgEarth::CDBTile::CDB_Tile::North(void)
{
	return m_TileExtent.North;
}

double osgEarth::CDBTile::CDB_Tile::South(void)
{
	return m_TileExtent.South;
}

void osgEarth::CDBTile::CDB_Tile::Build_From_Tiles(CDB_TilePV *Tiles, bool from_scratch)
{

	for each (CDB_TileP tile in *Tiles)
	{
		if (tile->Subordinate_Exists())
		{
			m_Subordinate_Tile = true;
			m_Subordinate_Exists = true;
			break;
		}
	}

	for each (CDB_TileP tile in *Tiles)
	{

		if (tile->Subordinate2_Exsits())
		{
			m_Subordinate2_Exists = true;
			m_Have_MaterialData = true;
			break;
		}
	}
	Allocate_Buffers();

	if (!from_scratch && m_FileExists)
	{
		Open_Tile();
		//Load the current tile Information
		Read();
	}
	else
	{
		Fill_Tile();
	}

	bool have_some_contribution = false;
	double XRes = (m_TileExtent.East - m_TileExtent.West) / (double)m_Pixels.pixX;
	double YRes = (m_TileExtent.North - m_TileExtent.South) / (double)m_Pixels.pixY;
	for each (CDB_TileP tile in *Tiles)
	{
		Image_Contrib ImageContrib = tile->Get_Contribution(m_TileExtent);
		if ((ImageContrib == Full) || (ImageContrib == Partial))
		{
			if (tile->Load_Tile())
			{
				have_some_contribution = true;
				int sy = (int)((m_TileExtent.North - tile->North()) / YRes);
				if (sy < 0)
					sy = 0;
				int ey = (int)((m_TileExtent.North - tile->South()) / YRes);
				if (ey > m_Pixels.pixY - 1)
					ey = m_Pixels.pixY - 1;
				int sx = (int)((tile->West() - m_TileExtent.West) / XRes);
				if (sx < 0)
					sx = 0;
				int ex = (int)((tile->East() - m_TileExtent.West) / XRes);
				if (ex > m_Pixels.pixX - 1)
					ex = m_Pixels.pixX - 1;

				double srowlon = m_TileExtent.West + ((double)sx * XRes);
				double srowlat = m_TileExtent.North - ((double)sy *  YRes);
				int buffloc = 0;
				coord2d clatlon;
				clatlon.Ypos = srowlat;
				bool proces_subordinate = m_Subordinate_Tile && tile->Subordinate_Exists();
				if (m_Subordinate2_Exists)
					if (tile->Has_Mask_Data())
						m_Have_MaterialMaskData = true;
				for (int iy = sy; iy <= ey; ++iy)
				{
					buffloc = (iy * m_Pixels.pixX) + sx;
					clatlon.Xpos = srowlon;
					for (int ix = sx; ix <= ex; ++ix)
					{
						coord2d impix = tile->LL2Pix(clatlon);
						if ((m_TileType == Imagery) || (m_TileType == ImageryCache))
						{
							unsigned char redpix, greenpix, bluepix, matpix, maskpix;
							if (!m_Subordinate_Exists && !m_Subordinate2_Exists)
							{
								if (tile->Get_Image_Pixel(impix, redpix, greenpix, bluepix))
								{
									m_GDAL.reddata[buffloc] = redpix;
									m_GDAL.greendata[buffloc] = greenpix;
									m_GDAL.bluedata[buffloc] = bluepix;
								}
							}
							else if (m_Subordinate_Exists)
							{
								if (tile->Get_Lightmap_Pixel(impix, redpix, greenpix, bluepix))
								{
									m_GDAL.lightmapdatar[buffloc] = redpix;
									m_GDAL.lightmapdatag[buffloc] = greenpix;
									m_GDAL.lightmapdatab[buffloc] = bluepix;
								}
							}
							else if (m_Subordinate2_Exists)
							{
								if (tile->Has_Material_Data())
								{
									if (tile->Get_Material_Pixel(impix, matpix))
									{
										m_GDAL.materialdata[buffloc] = matpix;
									}
								}
								if (tile->Has_Mask_Data())
								{
									if (tile->Get_Mask_Pixel(impix, maskpix))
									{
										m_GDAL.materialmaskdata[buffloc] = maskpix;
									}
								}
							}
						}
						else if ((m_TileType == Elevation) || (m_TileType == ElevationCache))
						{
							tile->Get_Elevation_Pixel(impix, m_GDAL.elevationdata[buffloc]);
							if(proces_subordinate)
								tile->Get_Subordinate_Elevation_Pixel(impix, m_GDAL.subord_elevationdata[buffloc]);
						}
						++buffloc;
						clatlon.Xpos += XRes;
					}
					clatlon.Ypos -= YRes;
				}
			}
			tile->Free_Resources();

		}
	}

	if (have_some_contribution)
	{
		m_Tile_Status = Loaded;
	}

}

bool osgEarth::CDBTile::CDB_Tile::Save(void)
{
	char **papszOptions = NULL;

	//Set the transformation Matrix
	m_GDAL.adfGeoTransform[0] = m_TileExtent.West;
	m_GDAL.adfGeoTransform[1] = (m_TileExtent.East - m_TileExtent.West) / (double)m_Pixels.pixX;
	m_GDAL.adfGeoTransform[2] = 0.0;
	m_GDAL.adfGeoTransform[3] = m_TileExtent.North;
	m_GDAL.adfGeoTransform[4] = 0.0;
	m_GDAL.adfGeoTransform[5] = ((m_TileExtent.North - m_TileExtent.South) / (double)m_Pixels.pixY) * -1.0;

	OGRSpatialReference *CDB_SRS = new OGRSpatialReference();
	CDB_SRS->SetWellKnownGeogCS("WGS84");
	CDB_SRS->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
	if (m_GDAL.poDataset)
	{
		Close_Dataset();
	}


	if (m_TileType == ImageryCache)
	{
		if (m_GDAL.poDataset == NULL)
		{
			//Get the Imagine Driver
			m_GDAL.poDriver = Gbl_TileDrivers.cdb_GTIFFDriver;
			GDALDataType dataType = GDT_Byte;
			if (!m_GDAL.poDriver)
			{
				delete CDB_SRS;
				return false;
			}
			//Create the file
			m_GDAL.poDataset = m_GDAL.poDriver->Create(m_FileName.c_str(), m_Pixels.pixX, m_Pixels.pixY, m_Pixels.bands, dataType, papszOptions);

			if (!m_GDAL.poDataset)
			{
				delete CDB_SRS;
				return false;
			}

			m_GDAL.poDataset->SetGeoTransform(m_GDAL.adfGeoTransform);
			char *projection = NULL;
			CDB_SRS->exportToWkt(&projection);
			m_GDAL.poDataset->SetProjection(projection);
			CPLFree(projection);
		}
		//Write the elevation data to the file
		if (!Write())
		{
			delete CDB_SRS;
			return false;
		}
	}
	else if (m_TileType == ElevationCache)
	{
		if (m_GDAL.poDataset == NULL)
		{
			//Get the Imagine Driver
			m_GDAL.poDriver = Gbl_TileDrivers.cdb_HFADriver;
			GDALDataType dataType = GDT_Float32;
			if (!m_GDAL.poDriver)
			{
				delete CDB_SRS;
				return false;
			}
			//Create the file
			m_GDAL.poDataset = m_GDAL.poDriver->Create(m_FileName.c_str(), m_Pixels.pixX, m_Pixels.pixY, m_Pixels.bands, dataType, papszOptions);

			if (!m_GDAL.poDataset)
			{
				delete CDB_SRS;
				return false;
			}
			m_GDAL.poDataset->SetGeoTransform(m_GDAL.adfGeoTransform);
			char *projection = NULL;
			CDB_SRS->exportToWkt(&projection);
			m_GDAL.poDataset->SetProjection(projection);
			if (m_Subordinate_Tile)
			{
				m_GDAL.soDataset = m_GDAL.poDriver->Create(m_SubordinateName.c_str(), m_Pixels.pixX, m_Pixels.pixY, m_Pixels.bands, dataType, papszOptions);

				if (!m_GDAL.soDataset)
				{
					delete CDB_SRS;
					return false;
				}
				m_GDAL.soDataset->SetGeoTransform(m_GDAL.adfGeoTransform);
				m_GDAL.poDataset->SetProjection(projection);
			}
			CPLFree(projection);
		}
		//Write the elevation data to the file
		if (!Write())
		{
			delete CDB_SRS;
			return false;
		}
	}

	delete CDB_SRS;
	return true;
}

bool osgEarth::CDBTile::CDB_Tile::Write(void)
{
	CPLErr gdal_err;

	if (m_TileType == ImageryCache)
	{
		GDALRasterBand * RedBand = m_GDAL.poDataset->GetRasterBand(1);
		GDALRasterBand * GreenBand = m_GDAL.poDataset->GetRasterBand(2);
		GDALRasterBand * BlueBand = m_GDAL.poDataset->GetRasterBand(3);

		gdal_err = RedBand->RasterIO(GF_Write, 0, 0, m_Pixels.pixX, m_Pixels.pixY, m_GDAL.reddata, m_Pixels.pixX, m_Pixels.pixY, GDT_Byte, 0, 0);
		if (gdal_err == CE_Failure)
		{
			return false;
		}

		gdal_err = GreenBand->RasterIO(GF_Write, 0, 0, m_Pixels.pixX, m_Pixels.pixY, m_GDAL.greendata, m_Pixels.pixX, m_Pixels.pixY, GDT_Byte, 0, 0);
		if (gdal_err == CE_Failure)
		{
			return false;
		}

		gdal_err = BlueBand->RasterIO(GF_Write, 0, 0, m_Pixels.pixX, m_Pixels.pixY, m_GDAL.bluedata, m_Pixels.pixX, m_Pixels.pixY, GDT_Byte, 0, 0);
		if (gdal_err == CE_Failure)
		{
			return false;
		}
	}
	else if (m_TileType == ElevationCache)
	{
		GDALRasterBand * ElevationBand = m_GDAL.poDataset->GetRasterBand(1);
		gdal_err = ElevationBand->RasterIO(GF_Write, 0, 0, m_Pixels.pixX, m_Pixels.pixY, m_GDAL.elevationdata, m_Pixels.pixX, m_Pixels.pixY, GDT_Float32, 0, 0);
		if (gdal_err == CE_Failure)
		{
			return false;
		}
		if (m_Subordinate_Tile)
		{
			GDALRasterBand * SoElevationBand = m_GDAL.soDataset->GetRasterBand(1);
			gdal_err = SoElevationBand->RasterIO(GF_Write, 0, 0, m_Pixels.pixX, m_Pixels.pixY, m_GDAL.subord_elevationdata, m_Pixels.pixX, m_Pixels.pixY, GDT_Float32, 0, 0);
			if (gdal_err == CE_Failure)
			{
				return false;
			}
		}
	}
	return true;
}

std::string osgEarth::CDBTile::CDB_Tile::Subordinate_Name(void)
{
	return m_SubordinateName;
}

std::string osgEarth::CDBTile::CDB_Tile::Subordinate2_Name(void)
{
	return m_SubordinateName2;
}

osg::Image* osgEarth::CDBTile::CDB_Tile::Image_From_Tile(void)
{
	if (m_Tile_Status == Loaded)
	{
		//allocate the osg image
		osg::ref_ptr<osg::Image> image = new osg::Image;
		std::string tilename = osgDB::getSimpleFileName(m_FileName);
		image->setFileName(tilename);
		GLenum pixelFormat = GL_RGBA;
		int Rnum = 1;
		int curRnum = 0;
		int ibufpos = 0;
		int dst_row = 0;
		image->allocateImage(m_Pixels.pixX, m_Pixels.pixY, Rnum, pixelFormat, GL_UNSIGNED_BYTE);
		memset(image->data(), 0, image->getImageSizeInBytes());
		if (!m_Subordinate_Exists && !m_Subordinate2_Exists)
		{
			ibufpos = 0;
			dst_row = 0;
			for (int iy = 0; iy < m_Pixels.pixY; ++iy)
			{
				int dst_col = 0;
				for (int ix = 0; ix < m_Pixels.pixX; ++ix)
				{
					//Populate the osg:image
					*(image->data(dst_col, dst_row, curRnum) + 0) = m_GDAL.reddata[ibufpos];
					*(image->data(dst_col, dst_row, curRnum) + 1) = m_GDAL.greendata[ibufpos];
					*(image->data(dst_col, dst_row, curRnum) + 2) = m_GDAL.bluedata[ibufpos];
					*(image->data(dst_col, dst_row, curRnum) + 3) = 255;
					++ibufpos;
					++dst_col;
				}
				++dst_row;
			}
		}
		else if (m_Subordinate_Exists)
		{
			ibufpos = 0;
			dst_row = 0;
			for (int iy = 0; iy < m_Pixels.pixY; ++iy)
			{
				int dst_col = 0;
				for (int ix = 0; ix < m_Pixels.pixX; ++ix)
				{
					//Populate the osg:image
					*(image->data(dst_col, dst_row, curRnum) + 0) = m_GDAL.lightmapdatar[ibufpos];
					*(image->data(dst_col, dst_row, curRnum) + 1) = m_GDAL.lightmapdatag[ibufpos];
					*(image->data(dst_col, dst_row, curRnum) + 2) = m_GDAL.lightmapdatab[ibufpos];
					*(image->data(dst_col, dst_row, curRnum) + 3) = 255;
					++ibufpos;
					++dst_col;
				}
				++dst_row;
			}
		}
		else if (m_Subordinate2_Exists)
		{
			ibufpos = 0;
			dst_row = 0;
			for (int iy = 0; iy < m_Pixels.pixY; ++iy)
			{
				int dst_col = 0;
				for (int ix = 0; ix < m_Pixels.pixX; ++ix)
				{
					//Populate the osg:image
					*(image->data(dst_col, dst_row, curRnum) + 0) = m_GDAL.materialdata[ibufpos];
					*(image->data(dst_col, dst_row, curRnum) + 1) = m_GDAL.materialdata[ibufpos];
					*(image->data(dst_col, dst_row, curRnum) + 2) = m_GDAL.materialdata[ibufpos];
					if(m_Have_MaterialMaskData)
						*(image->data(dst_col, dst_row, curRnum) + 3) = m_GDAL.materialmaskdata[ibufpos];
					else
						*(image->data(dst_col, dst_row, curRnum) + 3) = 255;
					++ibufpos;
					++dst_col;
				}
				++dst_row;
			}
		}
		image->flipVertical();
		return image.release();
	}
	else
		return NULL;
}

osg::HeightField* osgEarth::CDBTile::CDB_Tile::HeightField_From_Tile(void)
{
	if (m_Tile_Status == Loaded)
	{
		osg::ref_ptr<osg::HeightField> field = new osg::HeightField;
		std::string tilename = osgDB::getSimpleFileName(m_FileName);
		field->setName(tilename);

		field->allocate(m_Pixels.pixX, m_Pixels.pixY);
		//For now clear the data
		for (unsigned int i = 0; i < field->getHeightList().size(); ++i)
			field->getHeightList()[i] = NO_DATA_VALUE;

		int ibpos = 0;
		for (unsigned int r = 0; r < (unsigned)m_Pixels.pixY; r++)
		{
			unsigned inv_r = m_Pixels.pixY - r - 1;
			for (unsigned int c = 0; c < (unsigned)m_Pixels.pixX; c++)
			{
				//Re-enable this if data is suspect
				//Should already be filtered in CDB creation however
#if 0
				float h = elevation[ibpos];
				// Mark the value as nodata using the universal NO_DATA_VALUE marker.
				if (!isValidValue(h, band))
				{
					h = NO_DATA_VALUE;
				}
#endif
				if(m_Subordinate_Exists || m_Subordinate_Tile)
					field->setHeight(c, inv_r, (m_GDAL.elevationdata[ibpos] - m_GDAL.subord_elevationdata[ibpos]));
				else
					field->setHeight(c, inv_r, m_GDAL.elevationdata[ibpos]);
				++ibpos;
			}
		}
		return field.release();
	}
	else
		return NULL;
}

std::string osgEarth::CDBTile::CDB_Tile::Xml_Name(std::string Name)
{
	std::string retString = "";
	size_t pos = Name.find_last_of(".");
	if (pos != string::npos)
	{
		retString = Name.substr(0, pos) + ".xml";
	}
	return retString;
}

std::string osgEarth::CDBTile::CDB_Tile::Set_FileType(std::string Name, std::string type)
{
	std::string retString = "";
	size_t pos = Name.find_last_of(".");
	if (pos != string::npos)
	{
		retString = Name.substr(0, pos) + type;
	}
	return retString;

}

std::string osgEarth::CDBTile::CDB_Tile::Model_FullFileName(std::string &FACC_value, std::string &FSC_value, std::string &BaseFileName, int sel)
{
	std::string lod_str;
	std::string lo_str = m_lod_str;
	if (sel >= 0)
	{
		if (!m_ModelSet[sel].LodStr.empty())
		{
			lod_str = m_ModelSet[sel].LodStr;
			lo_str = "LC";
		}
		else
			lod_str = m_lod_str;
	}
	else
		lod_str = m_lod_str;
	std::stringstream modbuf;
	modbuf << m_cdbRootDir
		<< "\\Tiles"
		<< "\\" << m_lat_str
		<< "\\" << m_lon_str
		<< "\\300_GSModelGeometry"
		<< "\\" << lo_str
		<< "\\" << m_uref_str
		<< "\\" << m_lat_str << m_lon_str << "_D300_S001_T001_" << lod_str
		<< "_" << m_uref_str << "_" << m_rref_str << "_"
		<< FACC_value << "_" << FSC_value << "_"
		<< BaseFileName << ".flt";
	return modbuf.str();
}

std::string osgEarth::CDBTile::CDB_Tile::GeoTypical_FullFileName(std::string &BaseFileName)
{
	std::string facc = BaseFileName.substr(0, 5);
	std::string Facc1 = BaseFileName.substr(0, 1);
	std::string Facc2 = BaseFileName.substr(1, 1);
	std::string Fcode = BaseFileName.substr(2, 3);
	if (m_HaveDataDictionary)//pull the info from the xml file
	{
		CDB_Data_Dictionary* datDict = CDB_Data_Dictionary::GetInstance();
		
		if (datDict->SelectFACC(facc, Facc1, Facc2, Fcode))
		{
			Facc1 = facc.substr(0, 1) + "_" + Facc1;

			Facc2 = facc.substr(1, 1) + "_" + Facc2;

			Fcode = facc.substr(2) + "_" + Fcode;
		}
	}
	else //don't have it so do it the old way for now
	{
		//First Level subdirectory
		if (Facc1 == "A")
			Facc1 = "A_Culture";
		else if (Facc1 == "E")
			Facc1 = "E_Vegetation";
		else if (Facc1 == "B")
			Facc1 = "B_Hydrography";
		else if (Facc1 == "C")
			Facc1 = "C_Hypsography";
		else if (Facc1 == "D")
			Facc1 = "D_Physiography";
		else if (Facc1 == "F")
			Facc1 = "F_Demarcation";
		else if (Facc1 == "G")
			Facc1 = "G_Aeronautical_Information";
		else if (Facc1 == "I")
			Facc1 = "I_Cadastral";
		else if (Facc1 == "S")
			Facc1 = "S_Special_Use";
		else
			Facc1 = "Z_General";

		//Second Level Directory
		if (Facc2 == "C")
			Facc2 = "C_Woodland";
		else if (Facc2 == "D")
			Facc2 = "D_Power_Gen";
		else if (Facc2 == "E")
			Facc2 = "E_Fab_Industry";
		else if (Facc2 == "K")
			Facc2 = "K_Recreational";
		else if (Facc2 == "L")
			Facc2 = "L_Misc_Feature";
		else if (Facc2 == "T")
			Facc2 = "T_Comm";


		if (Facc1 == "A_Culture")
		{
			if (Fcode == "015")
				Fcode = "015_Building";
			else if (Fcode == "050")
				Fcode = "050_Display_Sign";
			else if (Fcode == "110")
				Fcode = "110_Light_Standard";
			else if (Fcode == "030")
				Fcode = "030_Power_Line";
			else if (Fcode == "040")
				Fcode = "040_Power_Pylon";
			else if (Fcode == "010")
			{
				if (Facc2 == "E_Fab_Industry")
					Fcode = "010_Assembly_Plant";
				else
					Fcode = "010_Power_Plant";
			}
			else if (Fcode == "240")
				Fcode = "240_Tower-NC";
			else if (Fcode == "241")
				Fcode = "241_Tower_General";
			else if (Fcode == "080")
				Fcode = "080_Comm_Tower";
			else if (Fcode == "020")
				Fcode = "020_Built-Up_Area";
		}
		else if (Facc1 == "E_Vegetation")
		{
			if (Fcode == "030")
				Fcode = "030_Trees";
		}
	}

	std::stringstream modbuf;
	if (!m_DataFromGlobal)
	{
		modbuf << m_cdbRootDir
			<< "\\GTModel\\500_GTModelGeometry"
			<< "\\" << Facc1
			<< "\\" << Facc2
			<< "\\" << Fcode
			<< "\\D500_S001_T001_" << BaseFileName;
	}
	else
	{
		modbuf << Facc1
			<< "/" << Facc2
			<< "/" << Fcode
			<< "/D500_S001_T001_" << BaseFileName;
	}
	return modbuf.str();
}

std::string osgEarth::CDBTile::CDB_Tile::Model_FileName(std::string &FACC_value, std::string &FSC_value, std::string &BaseFileName, int sel)
{
	std::stringstream modbuf;
	std::string lod_str;
	if (sel > 0)
	{
		if (!m_ModelSet[sel].LodStr.empty())
			lod_str = m_ModelSet[sel].LodStr;
		else
			lod_str = m_lod_str;
	}
	else
		lod_str = m_lod_str;
	modbuf << m_lat_str << m_lon_str << "_D300_S001_T001_" << lod_str
		<< "_" << m_uref_str << "_" << m_rref_str << "_"
		<< FACC_value << "_" << FSC_value << "_"
		<< BaseFileName << ".flt";
	return modbuf.str();
}

std::string osgEarth::CDBTile::CDB_Tile::Model_TextureDir(void)
{
	std::stringstream modbuf;
	modbuf << m_cdbRootDir
		<< "\\Tiles"
		<< "\\" << m_lat_str
		<< "\\" << m_lon_str
		<< "\\301_GSModelTexture"
		<< "\\" << m_lod_str
		<< "\\" << m_uref_str;
	return modbuf.str();
}

std::string osgEarth::CDBTile::CDB_Tile::Model_ZipDir(void)
{
	std::string retstr = "";
	if (m_TileType != GeoSpecificModel)
		return retstr;
	std::stringstream modbuf;
	modbuf << m_cdbRootDir
		<< "\\Tiles"
		<< "\\" << m_lat_str
		<< "\\" << m_lon_str
		<< "\\300_GSModelGeometry"
		<< "\\" << m_lod_str
		<< "\\" << m_uref_str;
	return modbuf.str();
}

bool osgEarth::CDBTile::CDB_Tile::validate_tile_name(std::string &filename)
{
#ifdef _WIN32
	DWORD ftyp = ::GetFileAttributes(filename.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
	{
		DWORD error = ::GetLastError();
		if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
		{
			return false;
		}
		else
			return false;
	}
	return true;
#else
	int ftyp = ::access(filename.c_str(), F_OK);
	if (ftyp == 0)
	{
		return  true;
	}
	else
	{
		return false;
	}
#endif
}

std::string osgEarth::CDBTile::CDB_Tile::Model_HeaderName(void)
{
	std::string retstr = "";
	if (m_TileType != GeoSpecificModel && m_TileType != GeoTypicalModel)
		return retstr;

	std::stringstream modbuf;
	modbuf << m_lat_str << m_lon_str << "_D300_S001_T001_" << m_lod_str
		<< "_" << m_uref_str << "_" << m_rref_str << "_";
	return modbuf.str();

}

std::string osgEarth::CDBTile::CDB_Tile::Model_KeyNameFromArchiveName(const std::string &ArchiveFileName, std::string &Header)
{
	std::string KeyName = "";
	std::string SearchStr;
	//Mask off the selectors
	int spos1 = Header.find("_S");
	if (spos1 != std::string::npos)
		SearchStr = Header.substr(0, spos1 + 2);
	else
		SearchStr = Header;

	int pos1 = ArchiveFileName.find(SearchStr);
	if (pos1 != std::string::npos)
	{
		int pos2 = pos1 + Header.length();
		if (pos2 < ArchiveFileName.length())
		{
			KeyName = ArchiveFileName.substr(pos2);
		}
	}
	return KeyName;

}


osgDB::Archive::FileNameList * osgEarth::CDBTile::CDB_Tile::Model_Archive_List(unsigned int pos)
{
	if (m_TileType != GeoSpecificModel)
		return NULL;
	return &m_ModelSet[pos].archiveFileList;
}

osgEarth::CDBTile::OGR_File::OGR_File() : m_FileName(""), m_Driver(""), m_oSRS(NULL), m_OutputIsShape(false), m_PODataset(NULL), m_FID(0), m_FileExists(false), m_InstLayer(NULL),
					   m_ClassLayer(NULL), m_GeoTyp_Proc(false), m_InputIsShape(false)
{
	m_GS_ClassMap.clear();
}

osgEarth::CDBTile::OGR_File::OGR_File(std::string FileName, std::string Driver) :m_FileName(FileName), m_Driver(Driver), m_oSRS(NULL), m_OutputIsShape(false), m_PODataset(NULL), m_FID(0),
															  m_InstLayer(NULL), m_ClassLayer(NULL), m_GeoTyp_Proc(false), m_InputIsShape(false)
{
	m_FileExists = osgEarth::CDBTile::CDB_Tile::validate_tile_name(m_FileName);
	m_GS_ClassMap.clear();
}

osgEarth::CDBTile::OGR_File::~OGR_File()
{
	if (m_PODataset)
	{
		GDALClose(m_PODataset);
		m_PODataset = NULL;
	}
	m_GS_ClassMap.clear();
}

bool osgEarth::CDBTile::OGR_File::SetName_and_Driver(std::string Name, std::string Driver)
{
	m_FileName = Name;
	m_Driver = Driver;
	m_FileExists = osgEarth::CDBTile::CDB_Tile::validate_tile_name(m_FileName);
	return m_FileExists;
}

osgEarth::CDBTile::OGR_File * osgEarth::CDBTile::OGR_File::GetInstance(void)
{
	return &Ogr_File_Instance;
}

bool osgEarth::CDBTile::OGR_File::Set_Inst_Layer(std::string LayerName)
{
	if (!m_PODataset)
		return false;
	m_InstLayer = m_PODataset->GetLayerByName(LayerName.c_str());

	if (m_InstLayer)
		return true;
	else
		return false;
}

bool osgEarth::CDBTile::OGR_File::Set_Class_Layer(std::string LayerName)
{
	if (!m_PODataset)
		return false;
	m_ClassLayer = m_PODataset->GetLayerByName(LayerName.c_str());

	if (m_ClassLayer)
		return true;
	else
		return false;
}

OGRSpatialReference * osgEarth::CDBTile::OGR_File::Inst_Spatial_Reference()
{
	return m_InstLayer->GetSpatialRef();
}

bool osgEarth::CDBTile::OGR_File::Get_Inst_Envelope(OGREnvelope &env)
{
	OGRErr status = m_InstLayer->GetExtent(&env);
	if (status == OGRERR_NONE)
		return true;
	else
		return false;

}

bool osgEarth::CDBTile::OGR_File::Load_Class(void)
{
	OGRFeatureDefn * poFDefn = m_ClassLayer->GetLayerDefn();
	int name_attr_index = Find_Field_Index(poFDefn, "MODL", OFTString);
	if (name_attr_index < 0)
	{
		return false;
	}

	int cnam_attr_index = Find_Field_Index(poFDefn, "CNAM", OFTString);
	if (cnam_attr_index < 0)
	{
		return false;
	}

	int facc_index = Find_Field_Index(poFDefn, "FACC", OFTString);
	if (facc_index < 0)
	{
		return false;
	}

	int fsc_index = Find_Field_Index(poFDefn, "FSC", OFTInteger);
	if (fsc_index < 0)
	{
		return false;
	}

	int bsr_index = Find_Field_Index(poFDefn, "BSR", OFTReal);
	if (bsr_index < 0)
	{
		return false;
	}

	int bbw_index = Find_Field_Index(poFDefn, "BBW", OFTReal);
	if (bbw_index < 0)
	{
		return false;
	}

	int bbl_index = Find_Field_Index(poFDefn, "BBL", OFTReal);
	if (bbl_index < 0)
	{
		return false;
	}

	int bbh_index = Find_Field_Index(poFDefn, "BBH", OFTReal);
	if (bbh_index < 0)
	{
		return false;
	}

	int ahgt_index = Find_Field_Index(poFDefn, "AHGT", OFTInteger);

	m_ClassLayer->ResetReading();
	OGRFeature* dbf_feature;
	while ((dbf_feature = m_ClassLayer->GetNextFeature()) != NULL)
	{
		CDB_Model_Runtime_Class nextEntry;
		std::string Key = nextEntry.set_class(dbf_feature, cnam_attr_index, name_attr_index, facc_index, fsc_index, bsr_index, bbw_index, bbl_index, bbh_index, ahgt_index);
		m_GS_ClassMap.insert(std::pair<std::string, CDB_Model_Runtime_Class>(Key, nextEntry));
		OGRFeature::DestroyFeature(dbf_feature);
	}
	return true;
}

void osgEarth::CDBTile::OGR_File::Set_GeoTyp_Process(bool value)
{
	m_GeoTyp_Proc = value;
}

bool osgEarth::CDBTile::OGR_File::Exists(void)
{
	return m_FileExists;
}

GDALDataset * osgEarth::CDBTile::OGR_File::Open_Output_File(std::string FileName, bool FileExists)
{
#if 0
	while (!m_OpenLock->Lock())
	{
		Sleep(1000);
	}
#endif

	if (m_Driver == "ESRI Shapefile")
		m_OutputIsShape = true;

	GDALDataset *poDS = NULL;
	GDALDriver * poDriver = GetGDALDriverManager()->GetDriverByName(m_Driver.c_str());

	if (poDriver != NULL)
	{
		if (!FileExists)
		{
			poDS = poDriver->Create(FileName.c_str(), 0, 0, 0, GDT_Unknown, NULL);
			if (poDS == NULL)
			{
				std::string Message = "Unable to create output file " + FileName;
				printf("%s\n", Message.c_str());
#if 0
				m_OpenLock->Unlock();
#endif
				return NULL;
			}
		}
		else
		{
			GDALOpenInfo poOpenInfo(FileName.c_str(), GA_Update | GDAL_OF_VECTOR);
			poDS = poDriver->pfnOpen(&poOpenInfo);
			if (poDS == NULL)
			{
				std::string Message = "Unable to open output file " + FileName;
				printf("%s\n", Message.c_str());
#if 0
				m_OpenLock->Unlock();
#endif
				return NULL;
			}
		}
		if (m_oSRS == NULL)
		{
			Set_oSRS();
		}
	}
	else
	{
		std::string Message = "Unable to obtain driver for " + m_Driver;
		printf("%s\n", Message.c_str());
#if 0
		m_OpenLock->Unlock();
#endif
		return NULL;
	}

#if 0
	m_OpenLock->Unlock();
#endif

	return poDS;
}

GDALDataset * osgEarth::CDBTile::OGR_File::Open_Input(std::string FileName)
{
#if 0
	while (!m_OpenLock->Lock())
	{
		Sleep(1000);
	}
#endif

	if (!FileName.empty())
		m_FileName = FileName;
	if (m_Driver == "ESRI Shapefile")
		m_InputIsShape = true;

	GDALDriver * poDriver = GetGDALDriverManager()->GetDriverByName(m_Driver.c_str());

	if (poDriver != NULL)
	{
		GDALOpenInfo poOpenInfo(m_FileName.c_str(), GDAL_OF_VECTOR);
		m_PODataset = poDriver->pfnOpen(&poOpenInfo);
		if (m_PODataset == NULL)
		{
			std::string Message = "Unable to open output file " + m_FileName;
			printf("%s\n", Message.c_str());
			return NULL;
		}

	}
	else
	{
		std::string Message = "Unable to obtain driver for " + m_Driver;
		printf("%s\n", Message.c_str());
#if 0
		m_OpenLock->Unlock();
#endif
		return NULL;
	}

#if 0
	m_OpenLock->Unlock();
#endif

	return m_PODataset;
}

void osgEarth::CDBTile::OGR_File::Set_oSRS(void)
{
	if (m_oSRS == NULL)
	{
		m_oSRS = new OGRSpatialReference();
		m_oSRS->SetWellKnownGeogCS("WGS84");
		m_oSRS->SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
	}
}

GDALDataset * osgEarth::CDBTile::OGR_File::Open_Output(void)
{
	m_PODataset = Open_Output_File(m_FileName, m_FileExists);
	return m_PODataset;
}

OGRLayer * osgEarth::CDBTile::OGR_File::Get_Or_Create_Layer(std::string LayerName, osgEarth::Feature * f)
{
	return Get_Or_Create_Layer(m_PODataset, LayerName, f);
}

OGRLayer * osgEarth::CDBTile::OGR_File::Get_Or_Create_Layer(GDALDataset *poDS, std::string LayerName, osgEarth::Feature * f)
{
	OGRLayer *poLayer = NULL;
	if (poDS->TestCapability(ODsCCreateLayer))
	{
		poLayer = poDS->GetLayerByName(LayerName.c_str());
		if (!poLayer)
		{
			OGRSpatialReference * LayerSRS = new OGRSpatialReference(*m_oSRS);
			osgEarth::Geometry * geo = f->getGeometry();
			osgEarth::Geometry::Type gtype = geo->getType();
			OGRwkbGeometryType OgrGtype = wkbPoint25D;
			if (gtype == osgEarth::Geometry::Type::TYPE_POINTSET)
				OgrGtype = wkbPoint25D;
			else if (gtype == osgEarth::Geometry::Type::TYPE_POLYGON)
				OgrGtype = wkbPolygon25D;
			else if (gtype == osgEarth::Geometry::Type::TYPE_LINESTRING)
				OgrGtype = wkbLineString25D;
			poLayer = poDS->CreateLayer(LayerName.c_str(), LayerSRS, OgrGtype, NULL);
			if (poLayer)
			{
				if (!Set_Layer_Fields(f, poLayer))
				{
					std::string Message = "Failure to set Layer Feilds in " + m_FileName;
					printf("%s\n", Message.c_str());
					return NULL;
				}
			}
		}
		else
		{
			if (!Check_Layer_Fields(f, poLayer))
			{
				std::string Message = "Failure to set Layer Feilds in " + m_FileName;
				printf("%s\n", Message.c_str());
				return NULL;
			}

		}
	}
	else
	{
		std::string Message = "Unable to create layers in file " + m_FileName;
		printf("%s\n", Message.c_str());
		return NULL;
	}
	return poLayer;
}

bool osgEarth::CDBTile::OGR_File::Set_Layer_Fields(osgEarth::Feature * f, OGRLayer * poLayer)
{
	osgEarth::AttributeTable t = f->getAttrs();
	for (osgEarth::AttributeTable::iterator ti = t.begin(); ti != t.end(); ++ti)
	{
		osgEarth::AttributeTable::ENTRY temp;
		temp = *ti;
		std::string name = temp.first;
		osgEarth::AttributeValue value = temp.second;
		osgEarth::AttributeType attrType = value.type;
		OGRFieldType ogrType;
		if (attrType == osgEarth::AttributeType::ATTRTYPE_BOOL)
		{
			ogrType = OFTInteger;
		}
		else if (attrType == osgEarth::AttributeType::ATTRTYPE_DOUBLE)
		{
			ogrType = OFTReal;
		}
		else if (attrType == osgEarth::AttributeType::ATTRTYPE_INT)
		{
			ogrType = OFTInteger;
		}
		else if (attrType == osgEarth::AttributeType::ATTRTYPE_STRING)
		{
			ogrType = OFTString;
		}
		else if (attrType == osgEarth::AttributeType::ATTRTYPE_UNSPECIFIED)
		{
			ogrType = OFTBinary;
		}

		OGRFieldDefn po_FieldDefn(name.c_str(), ogrType);
		if (poLayer->CreateField(&po_FieldDefn) != OGRERR_NONE)
		{
			std::stringstream	buf;
			buf << "Unable to create Feild " << name << " in " << poLayer->GetName();
			std::string Message = buf.str();
			printf("%s\n", Message.c_str());
			return false;
		}

	}
	return true;
}

bool osgEarth::CDBTile::OGR_File::Check_Layer_Fields(osgEarth::Feature * f, OGRLayer * poLayer)
{
	OGRFeatureDefn *LayerDef = poLayer->GetLayerDefn();
	int Defncnt = LayerDef->GetFieldCount();
	bool retval = true;
	osgEarth::AttributeTable t = f->getAttrs();
	for (osgEarth::AttributeTable::iterator ti = t.begin(); ti != t.end(); ++ti)
	{
		osgEarth::AttributeTable::ENTRY temp;
		temp = *ti;
		std::string name = temp.first;
		osgEarth::AttributeValue value = temp.second;
		osgEarth::AttributeType attrType = value.type;
		OGRFieldType ogrType;
		if (attrType == osgEarth::AttributeType::ATTRTYPE_BOOL)
		{
			ogrType = OFTInteger;
		}
		else if (attrType == osgEarth::AttributeType::ATTRTYPE_DOUBLE)
		{
			ogrType = OFTReal;
		}
		else if (attrType == osgEarth::AttributeType::ATTRTYPE_INT)
		{
			ogrType = OFTInteger;
		}
		else if (attrType == osgEarth::AttributeType::ATTRTYPE_STRING)
		{
			ogrType = OFTString;
		}
		else if (attrType == osgEarth::AttributeType::ATTRTYPE_UNSPECIFIED)
		{
			ogrType = OFTBinary;
		}
		
		bool have_attr = false;
		bool add_attr = false;
		bool bad_attr = false;
		for (int i = 0; i < Defncnt; ++i)
		{
			OGRFieldDefn *po_FieldDefn = LayerDef->GetFieldDefn(i);
			std::string nameref = po_FieldDefn->GetNameRef();
			if (nameref == name)
			{
				if (ogrType == po_FieldDefn->GetType())
				{
					have_attr = true;
				}
				else
					bad_attr = true;
				break;
			}
		}
		if (!have_attr && !bad_attr)
		{
			OGRFieldDefn npo_FieldDefn(name.c_str(), ogrType);
			if (poLayer->CreateField(&npo_FieldDefn) != OGRERR_NONE)
			{
				std::stringstream	buf;
				buf << "Unable to create Feild " << name << " in " << poLayer->GetName();
				std::string Message = buf.str();
				printf("%s\n", Message.c_str());
				return false;
			}
		}
		if (bad_attr)
			retval = false;
	}
	return retval;
}

bool osgEarth::CDBTile::OGR_File::Add_Feature_to_Layer(OGRLayer *oLayer, osgEarth::Feature * f)
{
	bool retval = true;
	OGRFeatureDefn *FeaDefn = oLayer->GetLayerDefn();
	OGRFeature * OFeature = OGRFeature::CreateFeature(FeaDefn);
	OFeature->SetFID(m_FID);
	++m_FID;
	for (int i = 0; i < FeaDefn->GetFieldCount(); ++i)
	{
		OGRFieldDefn * Fdefn = FeaDefn->GetFieldDefn(i);
		OGRFieldType ogrType = Fdefn->GetType();
		std::string FieldName = Fdefn->GetNameRef();
		if (ogrType == OFTInteger)
		{
			int intval = f->getInt(FieldName);
			OFeature->SetField(i, intval);
		}
		else if (ogrType == OFTReal)
		{
			double doubleval = f->getDouble(FieldName);
			OFeature->SetField(i, doubleval);
		}
		else if (ogrType == OFTString)
		{
			std::string stringval = f->getString(FieldName);
			OFeature->SetField(i, stringval.c_str());
		}

	}

	osgEarth::Geometry * geo = f->getGeometry();
	osgEarth::Geometry::Type gtype = geo->getType();
	OGRwkbGeometryType OgrGtype = wkbPoint25D;
	if (gtype == osgEarth::Geometry::Type::TYPE_POINTSET)
		OgrGtype = wkbPoint25D;
	else if (gtype == osgEarth::Geometry::Type::TYPE_POLYGON)
		OgrGtype = wkbPolygon25D;
	else if (gtype == osgEarth::Geometry::Type::TYPE_LINESTRING)
		OgrGtype = wkbLineString25D;
	if (OgrGtype == wkbPoint25D)
	{
		OGRPoint poPoint;
		osg::Vec3d vec = geo->at(0);
		poPoint.setX(vec.x());
		poPoint.setY(vec.y());
		poPoint.setZ(vec.z());
		OFeature->SetGeometry(&poPoint);
	}
	else if (OgrGtype == wkbPolygon25D)
	{
		OGRPolygon poPolygon;
		OGRLinearRing poRing;
		for (int ii = 0; ii < geo->size(); ++ii)
		{
			OGRPoint poPoint;
			osg::Vec3d vec = geo->at(ii);
			poPoint.setX(vec.x());
			poPoint.setY(vec.y());
			poPoint.setZ(vec.z());
			poRing.addPoint(&poPoint);
		}
		poPolygon.addRing(&poRing);
		OFeature->SetGeometry(&poPolygon);
	}
	else if (OgrGtype == wkbLineString25D)
	{
		OGRLineString oLineString;
		for (int ii = 0; ii < geo->size(); ++ii)
		{
			OGRPoint poPoint;
			osg::Vec3d vec = geo->at(ii);
			poPoint.setX(vec.x());
			poPoint.setY(vec.y());
			poPoint.setZ(vec.z());
			oLineString.addPoint(&poPoint);
		}
		OFeature->SetGeometry(&oLineString);
	}
	OGRErr oErr = oLayer->CreateFeature(OFeature);
	if (oErr != OGRERR_NONE)
		retval = false;
	OGRFeature::DestroyFeature(OFeature);
	return retval;
}

bool osgEarth::CDBTile::OGR_File::Close_File(void)
{
	bool retval = false;
	if (m_PODataset)
	{
		GDALClose(m_PODataset);
		m_PODataset = NULL;
		m_FID = 0;
		m_FileName = "";
		m_FileExists = false;
		if (m_oSRS)
		{
			delete m_oSRS;
			m_oSRS = NULL;
		}
		retval = true;
	}
	return retval;
}

int osgEarth::CDBTile::OGR_File::Find_Field_Index(OGRFeatureDefn *poFDefn, std::string fieldname, OGRFieldType Type)
{
	int dbfieldcnt = poFDefn->GetFieldCount();
	for (int dbffieldIdx = 0; dbffieldIdx < dbfieldcnt; ++dbffieldIdx)
	{
		OGRFieldDefn *po_FieldDefn = poFDefn->GetFieldDefn(dbffieldIdx);
		std::string thisname = po_FieldDefn->GetNameRef();
		if (thisname.compare(fieldname) == 0)
		{
			if (po_FieldDefn->GetType() == Type)
				return dbffieldIdx;
		}
	}
	return -1;
}

OGRFeature * osgEarth::CDBTile::OGR_File::Next_Valid_Feature(std::string &ModelKeyName, std::string &FullModelName )
{
	bool valid = false;
	bool done = false;
	OGRFeature *f = NULL;

	while (!valid && !done)
	{
		f = m_InstLayer->GetNextFeature();
		if (!f)
		{
			done = true;
			break;
		}
		valid = true;

		std::string cnam = f->GetFieldAsString("CNAM");
		if (!cnam.empty())
		{
			CDB_Model_RuntimeMap::iterator mi = m_GS_ClassMap.find(cnam);
			if (mi == m_GS_ClassMap.end())
				valid = false;
			else
			{
				CDB_Model_Runtime_Class CurFeatureClass = m_GS_ClassMap[cnam];
				if (!m_GeoTyp_Proc)
				{
					ModelKeyName = cnam;
					FullModelName = CurFeatureClass.Model_Base_Name;
				}
				else
				{
					valid = false;
				}
			}
		}
		else
			valid = false;
	}
	return f;
}

osgEarth::CDBTile::CDB_Data_Dictionary::CDB_Data_Dictionary() :  m_dataDictDoc(NULL), m_dataDictData(NULL), m_IsInitialized(false)
{

}

bool osgEarth::CDBTile::CDB_Data_Dictionary::Init_Feature_Data_Dictionary(std::string CDB_Root_Dir)
{
	if (!m_IsInitialized)
	{
		std::string xmlFileName = CDB_Root_Dir + "\\Metadata\\Feature_Data_Dictionary.xml";
		m_dataDictDoc = new osgEarth::XmlDocument();
		m_dataDictData = m_dataDictDoc->load(xmlFileName);
		if (m_dataDictData)
		{
			bool hasCategories;
			hasCategories = Get_Model_Base_Catagory_List(m_BaseCategories);
			m_IsInitialized = hasCategories;
		}
	}	
	return m_IsInitialized;
}

bool osgEarth::CDBTile::CDB_Data_Dictionary::Get_Model_Base_Catagory_List(std::vector<CDB_Model_Code_Struct> &cats)
{
	osgEarth::XmlElement * mainNode = m_dataDictData->getSubElement("Feature_Data_Dictionary");
	for (osgEarth::XmlNodeList::const_iterator i = mainNode->getChildren().begin(); i != mainNode->getChildren().end(); i++)
	{
		CDB_Model_Code_Struct codes;
		osgEarth::XmlElement* e = (osgEarth::XmlElement*)i->get();
		osgEarth::XmlElement* eLabel = (osgEarth::XmlElement*)e->findElement("Label");
		codes.code = e->getAttr("code");
		codes.label = eLabel->getText();
		Get_Model_Sub_Catagory_List(e, codes);
		cats.push_back(codes);
	}
	return true;
}

bool osgEarth::CDBTile::CDB_Data_Dictionary::Get_Model_Sub_Catagory_List(osgEarth::XmlElement* catElement, CDB_Model_Code_Struct &subCats)
{
	for (osgEarth::XmlNodeList::const_iterator i = catElement->getChildren().begin(); i != catElement->getChildren().end(); i++)
	{

		osgEarth::XmlElement* e = (osgEarth::XmlElement*)i->get();
		if (e->getName() != "label")
		{
			CDB_Model_Code_Struct subCat;
			osgEarth::XmlElement* eLabel = (osgEarth::XmlElement*)e->findElement("Label");
			subCat.code = e->getAttr("code");
			subCat.label = eLabel->getText();
			Get_Model_Sub_Code_List(e, subCat);
			subCats.subCodes.push_back(subCat);
		}
	}
	return true;
}

bool osgEarth::CDBTile::CDB_Data_Dictionary::Get_Model_Sub_Code_List(osgEarth::XmlElement* subCodeElement, CDB_Model_Code_Struct &subCodes)
{
	for (osgEarth::XmlNodeList::const_iterator i = subCodeElement->getChildren().begin(); i != subCodeElement->getChildren().end(); i++)
	{

		osgEarth::XmlElement* e = (osgEarth::XmlElement*)i->get();
		if (e->getName() != "label")
		{
			CDB_Model_Code_Struct subCode;
			osgEarth::XmlElement* eLabel = (osgEarth::XmlElement*)e->findElement("Label");
			subCode.code = e->getAttr("code");
			subCode.label = eLabel->getText();
			subCodes.subCodes.push_back(subCode);
		}
	}
	return true;
}

osgEarth::CDBTile::CDB_Data_Dictionary * osgEarth::CDBTile::CDB_Data_Dictionary::GetInstance(void)
{
	return &CDB_Data_Dictionary_Instance;
}

osgEarth::CDBTile::CDB_Data_Dictionary::~CDB_Data_Dictionary()
{
	ClearMaps();
}

bool osgEarth::CDBTile::CDB_Data_Dictionary::SelectFACC(std::string FACC, std::string &CategoryLabel, std::string &SubCodeLabel, std::string &FeatureTypeLabel)
{
	std::string category = FACC.substr(0, 1);
	std::string subCode = FACC.substr(1, 1);
	std::string featureType = FACC.substr(2, 3);

	for each(CDB_Model_Code_Struct base in m_BaseCategories)
	{
		if (base.code == category)
		{
			CategoryLabel = base.label;
			for each(CDB_Model_Code_Struct subCodeObject in base.subCodes)
			{
				if (subCodeObject.code == subCode)
				{
					SubCodeLabel = subCodeObject.label;
					for each(CDB_Model_Code_Struct featTypeObject in subCodeObject.subCodes)
					{
						if (featTypeObject.code == featureType)
						{
							FeatureTypeLabel = featTypeObject.label;
							return true;
						}
					}

				}

			}

		}
	}
	return false;
}


void osgEarth::CDBTile::CDB_Data_Dictionary::ClearMaps()
{
	for each(CDB_Model_Code_Struct base in m_BaseCategories)
	{
		for each(CDB_Model_Code_Struct subCodeObject in base.subCodes)
		{
			subCodeObject.subCodes.clear();
		}
		base.subCodes.clear();
	}
	m_BaseCategories.clear();
}

