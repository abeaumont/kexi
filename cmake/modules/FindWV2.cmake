# - Try to find the WV2 library
# Once done this will define
#
#  WV2_FOUND - system has the WV2 library
#  WV2_INCLUDE_DIR - the WV2 include directory
#  WV2_LIBRARIES - The libraries needed to use WV2

FIND_PROGRAM(WV2CONFIG_EXECUTABLE NAMES wv2-config )

# if wv2-config has been found
IF (WV2CONFIG_EXECUTABLE)
  EXEC_PROGRAM(${WV2CONFIG_EXECUTABLE} ARGS --libs RETURN_VALUE _return_VALUE OUTPUT_VARIABLE WV2_LIBRARIES)

  EXEC_PROGRAM(${WV2CONFIG_EXECUTABLE} ARGS --cflags RETURN_VALUE _return_VALUE OUTPUT_VARIABLE WV2_INCLUDE_DIR)

  EXEC_PROGRAM(${WV2CONFIG_EXECUTABLE} ARGS --version RETURN_VALUE _return_VALUE OUTPUT_VARIABLE WV2_VERSION)

  IF (WV2_LIBRARIES AND WV2_INCLUDE_DIR AND (WV2_VERSION STREQUAL "0.3.1"))
    SET(WV2_FOUND TRUE)
  ENDIF (WV2_LIBRARIES AND WV2_INCLUDE_DIR)

  # ensure that they are cached
  set(WV2_INCLUDE_DIR ${WV2_INCLUDE_DIR} CACHE INTERNAL "The wv2 include path")
  set(WV2_LIBRARIES ${WV2_LIBRARIES} CACHE INTERNAL "The libraries needed to use libraries")

ENDIF (WV2CONFIG_EXECUTABLE)

if (WV2_FOUND)
  if (NOT WV2_FIND_QUIETLY)
    message(STATUS "Found wv2: ${WV2_LIBRARIES}")
  endif (NOT WV2_FIND_QUIETLY)
else (WV2_FOUND)
  if (WV2_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find WV2 (0.3.1 = currenly unreleased trunk)")
  endif (WV2_FIND_REQUIRED)
endif (WV2_FOUND)

MACRO_LOG_FEATURE(WV2_FOUND "wv2 libraries" "Library that interprets MS Office files. You need this for reading word/powerpoint files" "http://sf.net/projects/wvware/")

