# configureshaders.cmake.in

set(source_dir      "../../../../src/osgEarthDrivers/bumpmap")
set(bin_dir         "./")
set(glsl_files      "BumpMap.vert.view.glsl;BumpMap.frag.simple.glsl;BumpMap.frag.progressive.glsl;BumpMap.frag.common.glsl")
set(template_file   "BumpMapShaders.cpp.in")
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
