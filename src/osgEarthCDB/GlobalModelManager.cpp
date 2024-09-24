#include "GlobalModelManager"
#include <thread>

osgEarth::cdbModel::GlobalModelManager ReplaceCurrentModel_Instance;

osgEarth::cdbModel::GlobalModelManager::GlobalModelManager()
{
}


osgEarth::cdbModel::GlobalModelManager::~GlobalModelManager()
{
}

osgEarth::cdbModel::GlobalModelManager * osgEarth::cdbModel::GlobalModelManager::getInstance(void)
{
	return &ReplaceCurrentModel_Instance;
}

void osgEarth::cdbModel::GlobalModelManager::Add_To_Replacment_Stack(ModelReplacmentdataPV Replacements)
{
	m_StackMutex.lock();
	m_ReplaceNodes.push(Replacements);
	m_StackMutex.unlock();
}

bool osgEarth::cdbModel::GlobalModelManager::Have_Replacement_Data(void)
{
	if (m_ReplaceNodes.size() > 0)
		return true;
	else
		return false;
}

osgEarth::cdbModel::ModelReplacmentdataPV osgEarth::cdbModel::GlobalModelManager::Test_Next_Replacement(void)
{
	return m_ReplaceNodes.top();
}

osgEarth::cdbModel::ModelReplacmentdataPV osgEarth::cdbModel::GlobalModelManager::Get_Next_Replacement_From_Stack(void)
{
	m_StackMutex.lock();
	ModelReplacmentdataPV Replacements = m_ReplaceNodes.top();
	m_ReplaceNodes.pop();
	m_StackMutex.unlock();
	return Replacements;
}

void osgEarth::cdbModel::GlobalModelManager::Add_To_Retired_Map(std::string NodeName, std::string XformName, osg::ref_ptr<osg::Node> oldNode)
{
	RetiredModelData MD(XformName, oldNode);
	m_RetiredGSModels.insert(std::pair<std::string, RetiredModelData>(NodeName,MD));
}

bool osgEarth::cdbModel::GlobalModelManager::Have_RetiredModel(std::string ModelName)
{
	bool status = false;
	m_RetiredMutex.lock();

	RetiredModelMap::iterator mi = m_RetiredGSModels.find(ModelName);
	if (mi == m_RetiredGSModels.end())
		status = false;
	else
		status = true;

	m_RetiredMutex.unlock();

	return status;

}

bool osgEarth::cdbModel::GlobalModelManager::ModelNotLoading(std::string Uri)
{
	bool rstatus = false;
	m_LoadStatusMutex.lock();

	std::thread::id mythreadId = std::this_thread::get_id();
	std::map<std::string, TileLoadStatus>::iterator id = m_LoadStatusMap.find(Uri);
	if (id != m_LoadStatusMap.end())
	{
		if((id->second == Loading) || (id->second == Loaded))
		{ 
			OSG_WARN << "Uri " << Uri << " Is Already Loading " << std::endl;
			rstatus = false;
		}
		else
		{
			OSG_WARN << "Uri " << Uri << " Is Set to Reload " << std::endl;
			rstatus = true;
		}
	}
	else
	{
		OSG_WARN << "Uri " << Uri << " Is Set to Load " << std::endl;
		m_LoadStatusMap.insert(std::pair<std::string, TileLoadStatus>(Uri, Loading));
		rstatus = true;
	}

	m_LoadStatusMutex.unlock();

	return rstatus;

}
