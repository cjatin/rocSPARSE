# ########################################################################
# Copyright (C) 2018-2023 Advanced Micro Devices, Inc. All rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# ########################################################################

# This option only works for make/nmake and the ninja generators, but no reason it shouldn't be on all the time
# This tells cmake to create a compile_commands.json file that can be used with clang tooling or vim
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Print verbose compiler flags
if(BUILD_VERBOSE)
  include(../cmake/Verbose.cmake)
endif()

# MEMSTAT ?
if(BUILD_MEMSTAT)
  add_compile_options(-DROCSPARSE_WITH_MEMSTAT)
endif()


# Configure a header file to pass the rocSPARSE version
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/rocsparse-version.h.in"
               "${PROJECT_BINARY_DIR}/include/rocsparse/rocsparse-version.h"
)

# Public rocSPARSE headers
set(rocsparse_headers_public
  include/rocsparse-auxiliary.h
  include/rocsparse-functions.h
  include/rocsparse-types.h
  include/rocsparse-complex-types.h
  include/rocsparse.h
  ${PROJECT_BINARY_DIR}/include/rocsparse/rocsparse-version.h
)

source_group("Header Files\\Public" FILES ${rocsparse_headers_public})

# Include sources
include(src/CMakeLists.txt)

if (NOT WIN32)
# Set Fortran module output directory
set(CMAKE_Fortran_MODULE_DIRECTORY ${PROJECT_BINARY_DIR}/include/rocsparse)

# Create rocSPARSE Fortran module
add_library(rocsparse_fortran OBJECT ${rocsparse_fortran_source})

# Target compile options
target_compile_options(rocsparse_fortran PRIVATE -std=f2003 -ffree-form -cpp)
endif()

# Create rocSPARSE library
add_library(rocsparse ${rocsparse_source} ${rocsparse_headers_public})
add_library(roc::rocsparse ALIAS rocsparse)

# Target compile options
target_compile_options(rocsparse PRIVATE -Wno-unused-command-line-argument -Wall)

# Target include directories
target_include_directories(rocsparse
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
                                   $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/library/include>
                           PUBLIC  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/rocsparse>
                                   $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
                                   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Target link libraries
target_link_libraries(rocsparse PRIVATE roc::rocprim hip::device)
# Target properties
rocm_set_soversion(rocsparse ${rocsparse_SOVERSION})
set_target_properties(rocsparse PROPERTIES CXX_VISIBILITY_PRESET "hidden" VISIBILITY_INLINES_HIDDEN ON)
set_target_properties(rocsparse PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging")
set_target_properties(rocsparse PROPERTIES DEBUG_POSTFIX "-d")

if (WIN32 AND BUILD_CLIENTS)
  add_custom_command( TARGET rocsparse POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/staging/$<TARGET_FILE_NAME:rocsparse> ${PROJECT_BINARY_DIR}/clients/staging/$<TARGET_FILE_NAME:rocsparse> )
  if( ${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_custom_command( TARGET rocsparse POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/staging/rocsparse.pdb ${PROJECT_BINARY_DIR}/clients/staging/rocsparse.pdb )
  endif()
endif()

# Generate export header
include(GenerateExportHeader)
generate_export_header(rocsparse EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/rocsparse/rocsparse-export.h)


if (BUILD_FILE_REORG_BACKWARD_COMPATIBILITY AND NOT WIN32)
  rocm_wrap_header_file(
    rocsparse-version.h rocsparse-export.h
    GUARDS SYMLINK WRAPPER
    WRAPPER_LOCATIONS ${CMAKE_INSTALL_INCLUDEDIR} rocsparse/${CMAKE_INSTALL_INCLUDEDIR}
  )
endif( )



# Force installation of .f module file
rocm_install(FILES "${CMAKE_SOURCE_DIR}/library/src/rocsparse.f90"
              "${CMAKE_SOURCE_DIR}/library/src/rocsparse_enums.f90"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rocsparse"
)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/library/include ${PROJECT_BINARY_DIR}/include/rocsparse)
# Install targets
rocm_install_targets(TARGETS rocsparse
                     INCLUDE
                       ${CMAKE_BINARY_DIR}/include
)

if(BUILD_FILE_REORG_BACKWARD_COMPATIBILITY AND NOT WIN32)
  rocm_install(
    DIRECTORY
       "${PROJECT_BINARY_DIR}/rocsparse"
        DESTINATION "." )
  if ( NOT WIN32 )

    #Create SymLink for Fortran Object Module for backward compatibility
    rocm_install(
      CODE "
        set(PREFIX \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX})
	set(INPUT_FILES \${PREFIX}/include/rocsparse/rocsparse.f90 \${PREFIX}/include/rocsparse/rocsparse_enums.f90)
        set(SYMLINK_LOCATIONS \${PREFIX}/rocsparse/include \${PREFIX}/include)
	foreach(INPUT_FILE IN LISTS INPUT_FILES)
          foreach(LOCATION IN LISTS SYMLINK_LOCATIONS)
            file(MAKE_DIRECTORY \${LOCATION})
            execute_process(COMMAND ln -sfr \${INPUT_FILE} \${LOCATION})
            message(STATUS \"Created symlink in \${LOCATION} to \${INPUT_FILE}.\")
          endforeach()
        endforeach()
        "
    )
  endif() #NOT WIN32

  message( STATUS "Backward Compatible Sym Link Created for include directories" )
endif()

# Export targets
rocm_export_targets(TARGETS roc::rocsparse
                    DEPENDS PACKAGE hip
                    NAMESPACE roc::
)
