# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

set(MODULE GPUTrackingCUDA)

if(DEFINED CUDA_COMPUTETARGET)
  set(TMP_TARGET "(Compute Target ${CUDA_COMPUTETARGET})")
endif()
message(STATUS "Building GPUTracking with CUDA support ${TMP_TARGET}")

set(SRCS GPUReconstructionCUDA.cu)
set(HDRS GPUReconstructionCUDA.h GPUReconstructionCUDAInternals.h GPUReconstructionCUDADef.h GPUReconstructionCUDAIncludes.h)

# -------------------------------- Prepare RTC -------------------------------------------------------
if(NOT ALIGPU_BUILD_TYPE STREQUAL "ALIROOT")
  set(CURTC_DEFINES "-D$<JOIN:$<TARGET_PROPERTY:O2::GPUTracking,COMPILE_DEFINITIONS>,$<SEMICOLON>-D>"
                    -DGPUCA_GPUCODE_GENRTC -DGPUCA_GPULIBRARY=CUDA
  )
  set(CURTC_INCLUDES "-I$<JOIN:$<TARGET_PROPERTY:O2::GPUTracking,INCLUDE_DIRECTORIES>,$<SEMICOLON>-I>"
                    -I${CMAKE_SOURCE_DIR}/Detectors/Base/src
                    -I${CMAKE_SOURCE_DIR}/Detectors/TRD/base/src
  )
  if(ALIGPU_BUILD_TYPE STREQUAL "O2")
    set(CURTC_INCLUDES ${CURTC_INCLUDES} "-I$<JOIN:$<TARGET_PROPERTY:O2::ITStrackingCUDA,INCLUDE_DIRECTORIES>,$<SEMICOLON>-I>")
  endif()
  #set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -keep")

  set(CUDARTC_FLAGS "${CMAKE_CUDA_FLAGS} ${CMAKE_CUDA_FLAGS_${CMAKE_BUILD_TYPE}} -std=c++${CMAKE_CUDA_STANDARD}")
  if(CUDA_COMPUTETARGET)
      set(CUDARTC_FLAGS "${CUDARTC_FLAGS} -gencode arch=compute_${CUDA_COMPUTETARGET},code=sm_${CUDA_COMPUTETARGET}")
      set(RTC_CUDA_ARCH "${CUDA_COMPUTETARGET}0")
  else()
      set(RTC_CUDA_ARCH "750")
  endif()
  separate_arguments(CUDARTC_FLAGS)
  
  # convenience variables
  if(ALIGPU_BUILD_TYPE STREQUAL "Standalone")
    get_filename_component(GPUDIR ${CMAKE_SOURCE_DIR}/../ ABSOLUTE)
  else()
    set(GPUDIR ${CMAKE_SOURCE_DIR}/GPU/GPUTracking)
  endif()
  set(CURTC_SRC ${GPUDIR}/Base/cuda/GPUReconstructionCUDArtc.cu)
  set(CURTC_BIN ${CMAKE_CURRENT_BINARY_DIR}/GPUReconstructionCUDArtc)

  # cmake-format: off
  add_custom_command(
      OUTPUT ${CURTC_BIN}.src
      COMMAND cat ${GPUDIR}/Base/cuda/GPUReconstructionCUDAIncludes.h > ${CURTC_BIN}.src
      COMMAND ${CMAKE_CXX_COMPILER} ${CURTC_DEFINES} ${CURTC_INCLUDES} -std=c++${CMAKE_CUDA_STANDARD} -D__CUDA_ARCH__=${RTC_CUDA_ARCH} -D__CUDACC__ -x c++ -E ${CURTC_SRC} >> ${CURTC_BIN}.src
      MAIN_DEPENDENCY ${CURTC_SRC}
      IMPLICIT_DEPENDS CXX ${CURTC_SRC}
      COMMAND_EXPAND_LISTS
  )
  
  add_custom_command(
      OUTPUT ${CURTC_BIN}.src.S
      COMMAND cat ${GPUDIR}/Standalone/makefiles/include.S | sed "s/FILENAMEMOD/_curtc_GPUReconstructionCUDArtc_cu_src/g" | sed "s,FILENAMENORMAL,${CURTC_BIN}.src,g" > ${CURTC_BIN}.src.S
      MAIN_DEPENDENCY ${GPUDIR}/Standalone/makefiles/include.S
  )

  add_custom_command(
      OUTPUT ${CURTC_BIN}.command
      COMMAND echo -n "${CMAKE_CUDA_COMPILER} ${CUDARTC_FLAGS} ${CURTC_DEFINES}" > ${CURTC_BIN}.command
      COMMAND_EXPAND_LISTS
      VERBATIM
  )
  
  add_custom_command(
      OUTPUT ${CURTC_BIN}.command.S
      COMMAND cat ${GPUDIR}/Standalone/makefiles/include.S | sed "s/FILENAMEMOD/_curtc_GPUReconstructionCUDArtc_cu_command/g" | sed "s,FILENAMENORMAL,${CURTC_BIN}.command,g" > ${CURTC_BIN}.command.S
      MAIN_DEPENDENCY ${GPUDIR}/Standalone/makefiles/include.S
      DEPENDS ${CURTC_BIN}.command
  )
  # cmake-format: on
  
  # make cmake compile the assembler file, add proper dependency on included
  # binary code
  set_source_files_properties(
    ${CURTC_BIN}.src.S
    PROPERTIES
    LANGUAGE
    CXX
    OBJECT_DEPENDS
    "${CURTC_BIN}.src;${GPUDIR}/Standalone/makefiles/include.S")
    
  set_source_files_properties(
    ${CURTC_BIN}.command.S
    PROPERTIES
    LANGUAGE
    CXX
  )
  
  set(SRCS ${SRCS} ${CURTC_BIN}.src.S ${CURTC_BIN}.command.S)
endif()
# -------------------------------- End RTC -------------------------------------------------------

if(ALIGPU_BUILD_TYPE STREQUAL "O2")
  o2_add_library(
    ${MODULE}
    SOURCES ${SRCS}
    PRIVATE_INCLUDE_DIRECTORIES
      ${CMAKE_SOURCE_DIR}/Detectors/Base/src
      ${CMAKE_SOURCE_DIR}/Detectors/TRD/base/src
    PUBLIC_LINK_LIBRARIES O2::GPUTracking O2::ITStrackingCUDA
    TARGETVARNAME targetName)

  target_compile_definitions(
    ${targetName} PUBLIC GPUCA_GPULIBRARY=CUDA
    $<TARGET_PROPERTY:O2::GPUTracking,COMPILE_DEFINITIONS>)

  set_target_properties(${targetName} PROPERTIES LINKER_LANGUAGE CXX)

  install(FILES ${HDRS} DESTINATION include/GPU)
endif()

if(ALIGPU_BUILD_TYPE STREQUAL "ALIROOT")
  add_definitions(-DGPUCA_GPULIBRARY=CUDA)

  # Generate the dictionary
  get_directory_property(incdirs INCLUDE_DIRECTORIES)
  generate_dictionary("Ali${MODULE}" "" "GPUReconstructionCUDA.h"
                      "${incdirs} .")

  # Generate the ROOT map Dependecies
  generate_rootmap("Ali${MODULE}" "" "")

  # Add a library to the project using the specified source files
  add_library_tested(Ali${MODULE} SHARED ${SRCS} G__Ali${MODULE}.cxx)
  # CUDA run-time and driver
  target_link_libraries(Ali${MODULE} PUBLIC AliGPUTracking)

  # Additional compilation flags
  set_target_properties(Ali${MODULE} PROPERTIES COMPILE_FLAGS "")

  # Installation
  install(TARGETS Ali${MODULE} ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

  install(FILES ${HDRS} DESTINATION include)

  set(targetName Ali${MODULE})
endif()

if(ALIGPU_BUILD_TYPE STREQUAL "Standalone")
  add_definitions(-DGPUCA_GPULIBRARY=CUDA)
  add_library(${MODULE} SHARED ${SRCS})
  target_link_libraries(${MODULE} PUBLIC GPUTracking)
  set(targetName ${MODULE})
  install(TARGETS GPUTrackingCUDA)
endif()

# Since
target_link_libraries(${targetName} PRIVATE cuda cudart nvrtc)
