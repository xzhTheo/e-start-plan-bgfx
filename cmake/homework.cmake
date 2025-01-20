set( HOMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/homework")

# 在此处添加文件
set( HOMEWORK_SOURCES
	${HOMEWORK_DIR}/homework.cpp
	${HOMEWORK_DIR}/bgfx_logo.h
	${HOMEWORK_DIR}/camera.cpp
	${HOMEWORK_DIR}/camera.h
	${HOMEWORK_DIR}/shader/varying.def.sc
	${HOMEWORK_DIR}/shader/uniforms.sh

	${HOMEWORK_DIR}/shader/vs_plane.sc
	${HOMEWORK_DIR}/shader/fs_plane.sc
	${HOMEWORK_DIR}/shader/vs_skybox.sc
	${HOMEWORK_DIR}/shader/fs_skybox.sc
	${HOMEWORK_DIR}/shader/vs_LUT.sc
	${HOMEWORK_DIR}/shader/fs_LUT.sc
	${HOMEWORK_DIR}/shader/vs_pbr_ibl_stone.sc
	${HOMEWORK_DIR}/shader/fs_pbr_ibl_stone.sc
	${HOMEWORK_DIR}/shader/vs_shadowmap.sc
	${HOMEWORK_DIR}/shader/fs_shadowmap.sc
 )


add_executable( homework ${HOMEWORK_SOURCES} )
#SET(CMAKE_EXE_LINKER_FLAGS     "${CMAKE_EXE_LINKER_FLAGS} /level='requireAdministrator' /uiAccess='false'")

target_link_libraries( homework example-common )
target_include_directories( homework PRIVATE ${HOMEWORK_DIR} )

set(SHADER_VS_VERSION_GLSL "150")
set(SHADER_VS_VERSION_HLSL "_3_0")

execute_process(COMMAND ${CMAKE_COMMAND} -E
  copy_directory ${HOMEWORK_DIR}/shader/glsl ${CMAKE_CURRENT_BINARY_DIR}/shaders/glsl
  )

file(GLOB SHADERS "${HOMEWORK_DIR}/shader/*.sc")
foreach(SHADER ${SHADERS})
	string(REGEX REPLACE "[.]sc$" ".bin" SHADER_OUTPUT ${SHADER})
	string(REGEX REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/homework/shader" "${CMAKE_CURRENT_SOURCE_DIR}/homework/shader/glsl" SHADER_OUTPUT ${SHADER_OUTPUT})
	if(SHADER MATCHES "vs.*$")
		set(SHADER_TYPE "v")
	elseif(SHADER MATCHES "fs.*$")
		set(SHADER_TYPE "f")
	else()
		continue()
	endif()
	add_custom_command(TARGET homework
			PRE_BUILD COMMAND ${HOMEWORK_DIR}/../tools/shaderc.exe
			-i ${HOMEWORK_DIR}/../bgfx/examples/common -i ${HOMEWORK_DIR}/../bgfx/src          #include
			-f ${SHADER} -o ${SHADER_OUTPUT}  
			-p ${SHADER_VS_VERSION_GLSL}                                                 #input and output
			--type ${SHADER_TYPE} --platform windows                                           #shader type
			#-p ${SHADER_VS_VERSION} #-p ${SHADER_PS_VERSION}                                     #shader version
	)

endforeach()

execute_process(COMMAND ${CMAKE_COMMAND} -E
  copy_directory ${HOMEWORK_DIR}/shader/glsl ${CMAKE_CURRENT_BINARY_DIR}/shaders/glsl
  )



  



  execute_process(COMMAND ${CMAKE_COMMAND} -E
  copy_directory ${HOMEWORK_DIR}/shader/dx11 ${CMAKE_CURRENT_BINARY_DIR}/shaders/dx11
  )

file(GLOB SHADERS "${HOMEWORK_DIR}/shader/*.sc")
foreach(SHADER ${SHADERS})
	string(REGEX REPLACE "[.]sc$" ".bin" SHADER_OUTPUT ${SHADER})
	string(REGEX REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/homework/shader" "${CMAKE_CURRENT_SOURCE_DIR}/homework/shader/dx11" SHADER_OUTPUT ${SHADER_OUTPUT})
	if(SHADER MATCHES "vs.*$")
		set(SHADER_TYPE "v")
		set(SHADER_VS_VERSION_HLSL "vs_5_0")
	elseif(SHADER MATCHES "fs.*$")
		set(SHADER_TYPE "f")
		set(SHADER_VS_VERSION_HLSL "ps_5_0")
	else()
		continue()
	endif()
	add_custom_command(TARGET homework
			PRE_BUILD COMMAND ${HOMEWORK_DIR}/../tools/shaderc.exe
			-i ${HOMEWORK_DIR}/../bgfx/examples/common -i ${HOMEWORK_DIR}/../bgfx/src          #include
			-f ${SHADER} -o ${SHADER_OUTPUT}  
			-p ${SHADER_VS_VERSION_HLSL}                                                 #input and output
			--type ${SHADER_TYPE} --platform windows                                           #shader type
			#-p ${SHADER_VS_VERSION} #-p ${SHADER_PS_VERSION}                                     #shader version
	)
	
endforeach()

execute_process(COMMAND ${CMAKE_COMMAND} -E
  copy_directory ${HOMEWORK_DIR}/shader/dx11 ${CMAKE_CURRENT_BINARY_DIR}/shaders/dx11
  )

# Special Visual Studio Flags
if( MSVC )
	target_compile_definitions( homework PRIVATE "_CRT_SECURE_NO_WARNINGS" )
endif()
