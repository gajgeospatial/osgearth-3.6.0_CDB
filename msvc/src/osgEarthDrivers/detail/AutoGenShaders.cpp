/** CMake Template File - compiled into AutoGenShaders.cpp */
#include <osgEarthDrivers/detail/DetailShaders>

using namespace osgEarth::Detail;

Shaders::Shaders()
{
    VertexView = "Detail.vert.view.glsl";
    _sources[VertexView] = 
R"(#pragma vp_entryPoint oe_detail_vertexView
#pragma vp_location   vertex_view
uniform float oe_detail_lod;    // uniform of base LOD
uniform float oe_detail_maxRange;
uniform float oe_detail_attenDist;
vec4 oe_layer_tilec;        // stage global - tile coordinates
out vec2 detailCoords;      // output to fragment stage
out float detailIntensity;  // output to fragment stage.
                
// Terrain SDK function
vec2 oe_terrain_scaleCoordsToRefLOD(in vec2 tc, in float refLOD);
               
void oe_detail_vertexView(inout vec4 VertexView)
{
  detailCoords = oe_terrain_scaleCoordsToRefLOD(oe_layer_tilec.st, int(oe_detail_lod));
  detailIntensity = clamp((oe_detail_maxRange - (-VertexView.z))/oe_detail_attenDist, 0.0, 1.0);
}
)";

    Fragment = "Detail.frag.glsl";
    _sources[Fragment] = 
R"(#pragma vp_entryPoint oe_detail_fragment
#pragma vp_location   fragment_coloring
#pragma vp_order      1
                
uniform sampler2D oe_detail_tex; // uniform of detail texture
uniform float oe_detail_alpha;   // The detail textures alpha.
in vec2 detailCoords;            // input from vertex stage
in float detailIntensity;        // The intensity of the detail effect.
                
void oe_detail_fragment(inout vec4 color)
{
    vec4 texel = texture(oe_detail_tex, detailCoords);
    color.rgb = mix(color.rgb, texel.rgb, oe_detail_alpha * detailIntensity);
}                
)";
};
