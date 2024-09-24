# configureshaders.cmake.in

set(source_dir      "../../../src/osgEarth")
set(bin_dir         "./")
set(glsl_files      "CascadeDraping.glsl;Chonk.glsl;Chonk.Culling.glsl;DepthOffset.glsl;Draping.glsl;DrawInstancedAttribute.glsl;GPUClamping.glsl;GPUClamping.lib.glsl;HexTiling.glsl;Instancing.glsl;LineDrawable.glsl;MetadataNode.glsl;WireLines.glsl;PhongLighting.glsl;PointDrawable.glsl;Text.glsl;Text_legacy.glsl;ContourMap.glsl;GeodeticGraticule.glsl;LogDepthBuffer.glsl;LogDepthBuffer.VertOnly.glsl;ShadowCaster.glsl;SimpleOceanLayer.glsl;RTTPicker.glsl;WindLayer.CS.glsl;PBR.glsl")
set(template_file   "Shaders.cpp.in")
set(output_cpp_file "./AutoGenShaders.cpp")

# modify the contents for inlining; replace input with output (var: file)
# i.e., the file name (in the form ) gets replaced with the
# actual contents of the named file and then processed a bit.
foreach(file ${glsl_files})

    # read the file into 'contents':
    file(READ ${source_dir}/${file} contents)

    # compress whitespace.
    # "contents" must be quoted, otherwise semicolons get dropped for some reason.
    string(REGEX REPLACE "\n\n+" "\n" tempString "${contents}")
    
    set(${file} "\nR\"(${tempString})\"")

endforeach(file)

# send the processed glsl_files to the next template to create the
# shader source file.
configure_file(
	${source_dir}/${template_file}
	${output_cpp_file}
	@ONLY )
