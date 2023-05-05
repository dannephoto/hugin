# Try to find the expoxy libraries, setting these defines:
#  EPOXY_FOUND - system has epoxy
#  EPOXY_INCLUDE_DIR - epoxy include directory
#  EPOXY_LIBRARIES - Libraries needed to use epoxy

FIND_PATH(EPOXY_INCLUDE_DIR 
  NAMES epoxy/gl.h 
  PATHS ${SOURCE_BASE_DIR}/libepoxy/include
)
FIND_LIBRARY(EPOXY_LIBRARIES 
  NAMES libepoxy epoxy
  PATHS ${SOURCE_BASE_DIR}/libepoxy/lib
)

IF (EPOXY_INCLUDE_DIR AND EPOXY_LIBRARIES)
   SET(EPOXY_FOUND TRUE)
ENDIF()

IF (EPOXY_FOUND)
   IF (NOT EPOXY_FIND_QUIETLY)
      MESSAGE(STATUS "Found Epoxy: ${EPOXY_LIBRARIES}")
   ENDIF()
ELSE()
   IF(EPOXY_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find epoxy, install it with your package manager.")
   ENDIF()
ENDIF()
