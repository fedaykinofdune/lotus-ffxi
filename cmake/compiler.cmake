# taken from sapphire 
# https://github.com/SapphireServer/Sapphire/blob/master/cmake/compiler.cmake

if( NOT MSVC )
  set( CMAKE_CXX_STANDARD 20 )
# set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a " )
  add_definitions( -DGLM_ENABLE_EXPERIMENTAL=1 )
else()

  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
  add_definitions( -DNOMINMAX )
      
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc" )
  message( STATUS "Enabling Build with Multiple Processes.." )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4834" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest" )

  #set( CMAKE_CXX_STANDARD 20 )
  set( CMAKE_CXX_STANDARD_REQUIRED ON )
  set( CMAKE_CXX_EXTENSIONS ON )

  if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    # disabling SAFESEH
    message( STATUS "Disabling Safe Exception Handlers.." )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO" )

    # edit and continue
    message( STATUS "Enabling Edit and Continue.." )
    add_definitions( /Zi )

    # incremental linking
    message( STATUS "Enabling Incremental Linking.." )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL" )
    add_link_options(/DEBUG:FASTLINK)
  endif()
endif()
