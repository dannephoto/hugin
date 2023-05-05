
include_directories( ${CMAKE_SOURCE_DIR}/src/hugin1 )

INCLUDE(${wxWidgets_USE_FILE})

# do we need to define _UNICODE on windows?
IF(WIN32)
  ADD_DEFINITIONS(-D_UNICODE -DUNICODE)
ENDIF(WIN32)

# populate correct dpi aware manifest from wxWidgets
IF(WIN32)
  IF(CMAKE_SIZEOF_VOID_P EQUAL 8) 
    # 64 bit build
    FIND_FILE(WINDOWS_DPI_MANIFEST 
      NAMES amd64_dpi_aware_pmv2.manifest amd64-dpi-aware-pmv2.manifest wx_dpi_aware_pmv2.manifest 
      PATHS "${wxWidgets_ROOT_DIR}/include/wx/msw/"
    )
  ELSE()
    # 32 bit build
    FIND_FILE(WINDOWS_DPI_MANIFEST 
      NAMES wx_dpi_aware_pmv2.manifest wx-dpi-aware-pmv2.manifest
      PATHS "${wxWidgets_ROOT_DIR}/include/wx/msw/"
    )
  ENDIF()
  IF(NOT WINDOWS_DPI_MANIFEST)
    MESSAGE(FATAL_ERROR "Manifest in wx directory not found. Is wxWidgets not new enough or corrupted?")
  ENDIF()
ENDIF()

add_subdirectory(base_wx)
add_subdirectory(stitch_project)
add_subdirectory(icpfind)
add_subdirectory(executor)
add_subdirectory(hugin)
add_subdirectory(ptbatcher)
add_subdirectory(calibrate_lens)
