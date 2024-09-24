/** CMake Template File - compiled into AutoGenShaders.cpp */
#include <osgEarthDrivers/bumpmap/BumpMapShaders>

using namespace osgEarth::BumpMap;

Shaders::Shaders()
{
    VertexView = "BumpMap.vert.view.glsl";
    _sources[VertexView] = 
R"(#pragma vp_entryPoint oe_bumpmap_vertexView
#pragma vp_location   vertex_view
#pragma vp_order      0.5
#pragma import_defines(OE_IS_DEPTH_CAMERA, OE_IS_PICK_CAMERA)
#if defined(OE_IS_DEPTH_CAMERA) || defined(OE_IS_PICK_CAMERA)
void oe_bumpmap_vertexView(inout vec4 vertexView) { } 
#else
uniform float oe_bumpmap_scale;
uniform float oe_bumpmap_baseLOD;
out vec4 oe_layer_tilec;
out vec3 vp_Normal;
out vec2 oe_bumpmap_coords;
out float oe_bumpmap_range;
flat out mat3 oe_bumpmap_normalMatrix;
vec4 oe_tile_key;
vec2 oe_bumpmap_scaleCoords(in vec2 coords, in float targetLOD)
{
    float dL = oe_tile_key.z - targetLOD;
    float factor = exp2(dL);
    float invFactor = 1.0/factor;
    vec2 scale = vec2(invFactor);
    vec2 result = coords * scale;
    // For upsampling we need to calculate an offset as well
    float upSampleToggle = factor >= 1.0 ? 1.0 : 0.0;
    {
        vec2 a = floor(oe_tile_key.xy * invFactor);
        vec2 b = a * factor;
        vec2 c = (a+1.0) * factor;
        vec2 offset = (oe_tile_key.xy-b)/(c-b);
        result += upSampleToggle * offset;
    }
    return result;
}
void oe_bumpmap_vertexView(inout vec4 vertexView)
{
    oe_bumpmap_range = -vertexView.z;
    // quantize the scale factor
    float iscale = float(int(oe_bumpmap_scale));
    // scale sampling coordinates to a target LOD.
    oe_bumpmap_coords = oe_bumpmap_scaleCoords(oe_layer_tilec.st, floor(oe_bumpmap_baseLOD)) * iscale;
    // propagate normal matrix to fragment stage
    oe_bumpmap_normalMatrix = gl_NormalMatrix;
}
#endif
)";

    FragmentSimple = "BumpMap.frag.simple.glsl";
    _sources[FragmentSimple] = 
R"(#pragma vp_entryPoint oe_bumpmap_fragment
#pragma vp_location   fragment_coloring
#pragma vp_order      0.3
#pragma include BumpMap.frag.common.glsl
#pragma import_defines(OE_IS_DEPTH_CAMERA)
#pragma import_defines(OE_IS_PICK_CAMERA)
#if defined(OE_IS_DEPTH_CAMERA) || defined(OE_IS_PICK_CAMERA)
//nop
void oe_bumpmap_fragment(inout vec4 color) { }
#else
in vec3 vp_Normal;
in vec2 oe_bumpmap_coords;
flat in mat3 oe_bumpmap_normalMatrix;
in vec3 oe_UpVectorView;
uniform sampler2D oe_bumpmap_tex;
uniform float oe_bumpmap_intensity;
uniform float oe_bumpmap_slopeFactor;
uniform int oe_bumpmap_octaves;
void oe_bumpmap_fragment(inout vec4 color)
{
    if (oe_bumpmap_octaves > 0)
    {
        // sample the bump map
        vec3 bump = oe_bumpmap_normalMatrix * normalize(texture(oe_bumpmap_tex, oe_bumpmap_coords).xyz * 2.0 - 1.0);
        // calculate slope from normal:
        float slope = clamp((1.0 - dot(oe_UpVectorView, vp_Normal)) * oe_bumpmap_slopeFactor, 0.0, 1.0);
        // permute the normal with the bump.
        vp_Normal = normalize(vp_Normal + bump * oe_bumpmap_intensity * slope);
    }
}
#endif)";

    FragmentProgressive = "BumpMap.frag.progressive.glsl";
    _sources[FragmentProgressive] = 
R"(#pragma vp_entryPoint oe_bumpmap_fragment
#pragma vp_location   fragment_coloring
#pragma vp_order      0.3
#pragma include BumpMap.frag.common.glsl
#pragma import_defines(OE_IS_DEPTH_CAMERA)
#pragma import_defines(OE_IS_PICK_CAMERA)
#if defined(OE_IS_DEPTH_CAMERA) || defined(OE_IS_PICK_CAMERA)
//nop
void oe_bumpmap_fragment(inout vec4 color) { }
#else
uniform sampler2D oe_bumpmap_tex;
uniform float     oe_bumpmap_intensity;
uniform int       oe_bumpmap_octaves;
uniform float     oe_bumpmap_maxRange;
uniform float     oe_bumpmap_slopeFactor;
// framework/stage global
in vec3 vp_Normal;
in vec3 oe_UpVectorView;
// from BumpMap.model.vert.glsl
in vec2 oe_bumpmap_coords;
flat in mat3 oe_bumpmap_normalMatrix;
// from BumpMap.view.vert.glsl
in float oe_bumpmap_range;
// Entry point for progressive blended bump maps
void oe_bumpmap_fragment(inout vec4 color)
{
    if (oe_bumpmap_octaves > 0)
    {
        // sample the bump map
        const float amplitudeDecay = 1.0; // no decay.
        float maxLOD = float(oe_bumpmap_octaves) + 1.0;
        // starter vector:
        vec3 bump = vec3(0.0);
        float scale = 1.0;
        float amplitude = 1.0;
        float limit = oe_bumpmap_range;
        float range = oe_bumpmap_maxRange;
        float lastRange = oe_bumpmap_maxRange;
        for (float lod = 1.0; lod < maxLOD; lod += 1.0, scale *= 2.0, amplitude *= amplitudeDecay)
        {
            float fadeIn = 1.0;
            if (range <= limit && limit < oe_bumpmap_maxRange)
                fadeIn = clamp((lastRange - limit) / (lastRange - range), 0.0, 1.0);
            bump += (texture(oe_bumpmap_tex, oe_bumpmap_coords * scale).xyz * 2.0 - 1.0) * amplitude * fadeIn;
            if (range <= limit)
                break;
            lastRange = range;
            range = oe_bumpmap_maxRange / exp(lod);
        }
        // finally, transform into view space and normalize the vector.
        bump = normalize(oe_bumpmap_normalMatrix * bump);
        // calculate slope from normal:
        float slope = mix(1.0, 1.0 - dot(oe_UpVectorView, vp_Normal), oe_bumpmap_slopeFactor);
        // permute the normal with the bump.
        vp_Normal = normalize(vp_Normal + bump * oe_bumpmap_intensity * slope);
    }
}
#endif)";

    FragmentCommon = "BumpMap.frag.common.glsl";
    _sources[FragmentCommon] = 
R"(#pragma vp_define OE_USE_NORMAL_MAP
#ifdef OE_USE_NORMAL_MAP
// normal map version:
uniform sampler2D oe_tile_normalTex;
in vec2 oe_normalMapCoords;
float oe_bumpmap_getSlope()
{
    vec4 encodedNormal = texture(oe_nmap_normalTex, oe_normalMapCoords);
    vec3 normalTangent = normalize(encodedNormal.xyz*2.0-1.0);
    return clamp((1.0-normalTangent.z)/0.8, 0.0, 1.0);
}
#else // OE_USE_NORMAL_MAP
// non- normal map version:
in float oe_bumpmap_slope;
float oe_bumpmap_getSlope()
{
    return oe_bumpmap_slope;
}
#endif // OE_USE_NORMAL_MAP
)";
};
