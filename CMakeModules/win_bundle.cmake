IF(WIN32)

  # copy installer files
#  CONFIGURE_FILE(platforms/windows/msi/WixFragmentRegistry.wxs ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/WixFragmentRegistry.wxs COPYONLY)
#  CONFIGURE_FILE(platforms/windows/msi/hugin.warsetup ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/hugin.warsetup )
  # bug: CONFIGURE_FILE destroys the bitmaps.
#  CONFIGURE_FILE(platforms/windows/msi/top_banner.bmp ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/top_banner.bmp COPYONLY)
#  CONFIGURE_FILE(platforms/windows/msi/big_banner.bmp ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/big_banner.bmp COPYONLY)

  # install hugin readme, license etc.
  INSTALL(FILES AUTHORS COPYING.txt 
          DESTINATION doc/hugin)

  # find the path to enblend and panotools build directories
  # and copy required binaries into hugin installation folder
  FIND_PATH(PANO13_EXE_DIR PTmender.exe 
            ${SOURCE_BASE_DIR}/Deploy/bin
            ${PANO13_INCLUDE_DIR}/pano13/tools
            ${PANO13_INCLUDE_DIR}/pano13/tools/Release
            "${PANO13_INCLUDE_DIR}/pano13/tools/Release CMD/win32"
            ${SOURCE_BASE_DIR}/libpano/tools
            "${SOURCE_BASE_DIR}/libpano/pano13/tools/Release CMD/Win32"
            ${SOURCE_BASE_DIR}/libpano/tools/Release          
            ${SOURCE_BASE_DIR}/libpano13/bin
            DOC "Location of pano13 executables"
            NO_DEFAULT_PATH)
  FILE(GLOB PANO13_EXECUTABLES ${PANO13_EXE_DIR}/PT*.exe ${PANO13_EXE_DIR}/panoinfo.exe ${PANO13_EXE_DIR}/*.dll)
  INSTALL(FILES ${PANO13_EXECUTABLES} DESTINATION ${BINDIR})

  # TODO: install documentation for panotools?
  FIND_PATH(PANO13_DOC_DIR Optimize.txt 
            ${PANO13_INCLUDE_DIR}/../share/pano13/doc
            DOC "Location of pano13 documentation"
            NO_DEFAULT_PATH)
  INSTALL(FILES ${PANO13_DOC_DIR}/AUTHORS
          ${PANO13_DOC_DIR}/COPYING
          ${PANO13_DOC_DIR}/README
          ${PANO13_DOC_DIR}/Optimize.txt
          ${PANO13_DOC_DIR}/PTblender.readme
          ${PANO13_DOC_DIR}/PTmender.readme
          ${PANO13_DOC_DIR}/stitch.txt
          DESTINATION doc/panotools)

  # install enblend/enfuse files

  FILE(GLOB ENBLEND_EXECUTABLES ${ENBLEND_DIR}/bin/*.exe ${ENBLEND_DIR}/bin/*.dll ${ENBLEND_DIR}/*.exe)
  FILE(GLOB ENBLEND_DOC_FILES ${ENBLEND_DIR}/doc/*.pdf)
  INSTALL(FILES ${ENBLEND_EXECUTABLES} DESTINATION ${BINDIR})
  INSTALL(FILES ${ENBLEND_DOC_FILES} DESTINATION doc/enblend)

  # install exiftool, for version >=12.88
  IF(CMAKE_SIZEOF_VOID_P EQUAL 8) 
    # 64 bit version
    FILE(GLOB EXIFTOOL_PATHS LIST_DIRECTORIES true ${SOURCE_BASE_DIR}/exiftool/exiftool-*_64)
  ELSE()
    # 32 bit version
    FILE(GLOB EXIFTOOL_PATHS LIST_DIRECTORIES true ${SOURCE_BASE_DIR}/exiftool/exiftool-*_32)
  ENDIF()
  # check that we found at least one folder
  LIST(LENGTH EXIFTOOL_PATHS EXIFTOOL_COUNT)
  IF(EXIFTOOL_COUNT<1)
    MESSAGE(ERROR "Exiftool folder not found")
  ENDIF()
  # file list is sorted, so take last directory has it the highest number
  LIST(POP_BACK EXIFTOOL_PATHS EXIFTOOL_PATH)
  MESSAGE(STATUS "Exiftool path ${EXIFTOOL_PATH}")
  # check that correctly renamed executable is in path
  IF(NOT EXISTS "${EXIFTOOL_PATH}/exiftool.exe")
    MESSAGE(FATAL_ERROR "exiftool.exe not in exiftool path ${EXIFTOOL_PATH} found. Did you forget to rename exiftool(-k).exe to exiftool.exe?")
  ENDIF()
  INSTALL(
    DIRECTORY "${EXIFTOOL_PATH}/"
    DESTINATION ${BINDIR}
  )

  # now install all necessary DLL
  IF(HUGIN_SHARED)
    SET(DLL_SEARCH_PATH ${SOURCE_BASE_DIR}/Deploy/bin)
    IF(VCPKG_TOOLCHAIN)
      LIST(APPEND DLL_SEARCH_PATH ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin)
    ENDIF()
    FIND_FILE(TIFF_DLL
      NAMES libtiff.dll tiff.dll
      PATHS ${DLL_SEARCH_PATH} 
            ${SOURCE_BASE_DIR}/libtiff/bin
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(LZMA_DLL
      NAMES liblzma.dll lzma.dll
      PATHS ${DLL_SEARCH_PATH}
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(JPEG_DLL
      NAMES jpeg.dll libjpeg.dll jpeg62.dll
      PATHS ${DLL_SEARCH_PATH}
            ${SOURCE_BASE_DIR}/jpeg-9a/lib 
            ${SOURCE_BASE_DIR}/jpeg-9a/x64/Release
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(PNG_DLL
      NAMES libpng16.dll libpng15.dll libpng14.dll 
      PATHS ${DLL_SEARCH_PATH}
            ${SOURCE_BASE_DIR}/libpng/bin 
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(ZLIB_DLL
      NAMES zlib1.dll zlib.dll libz.dll libzlib.dll
      PATHS ${DLL_SEARCH_PATH}
            ${SOURCE_BASE_DIR}/zlib 
            ${SOURCE_BASE_DIR}/zlib/bin 
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_PATH(OPENEXR_BIN_DIR
            NAMES OpenEXR-3_2.dll OpenEXR-3_1.dll OpenEXR-3_0.dll
            PATHS ${DLL_SEARCH_PATH}
            DOC "Location of OpenEXR3 libraries"
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_DEFAULT_PATH
    )
    IF(OPENEXR_BIN_DIR)
      # DLL for OpenEXR 3.x
      FILE(GLOB OPENEXR_DLL 
        ${OPENEXR_BIN_DIR}/OpenEXR*.dll ${OPENEXR_BIN_DIR}/IlmThread*.dll ${OPENEXR_BIN_DIR}/IEx*.dll
        ${OPENEXR_BIN_DIR}/Imath*.dll ${OPENEXR_BIN_DIR}/deflate.dll
      )
    ELSE()
      # DLL for OpenEXR 2.x
      UNSET(OPENEXR_BIND_DIR)
      FIND_PATH(OPENEXR_BIN_DIR 
            NAMES Half-2_5.dll Half-2_4.dll Half-2_3.dll Half.dll libHalf.dll
            PATHS ${DLL_SEARCH_PATH}
                  ${SOURCE_BASE_DIR}/Deploy/lib 
                  ${SOURCE_BASE_DIR}/Deploy/lib/Release 
                  ${SOURCE_BASE_DIR}/Deploy/bin/Release 
            DOC "Location of OpenEXR libraries"
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_DEFAULT_PATH
      )
      IF(NOT OPENEXR_BIN_DIR)
        MESSAGE(FATAL_ERROR "OpenEXR dlls not found in search path")
      ENDIF()
      FILE(GLOB OPENEXR_DLL 
        ${OPENEXR_BIN_DIR}/Half*.dll ${OPENEXR_BIN_DIR}/IlmImf*.dll ${OPENEXR_BIN_DIR}/IEx*.dll
        ${OPENEXR_BIN_DIR}/IMath*.dll ${OPENEXR_BIN_DIR}/IlmThread*.dll)
    ENDIF()
    FIND_FILE(VIGRA_DLL
       NAMES vigraimpex.dll
       PATHS ${DLL_SEARCH_PATH}
             ${SOURCE_BASE_DIR}/vigra/bin 
       NO_SYSTEM_ENVIRONMENT_PATH
    )
    IF(NOT HAVE_STD_FILESYSTEM)
      FILE(GLOB BOOST_SYSTEM_DLL ${Boost_LIBRARY_DIRS}/*boost_system*.dll)
      FILE(GLOB BOOST_FILESYSTEM_DLL ${Boost_LIBRARY_DIRS}/*boost_filesystem*.dll)
      LIST(APPEND BOOST_DLLs ${BOOST_SYSTEM_DLL} ${BOOST_FILESYSTEM_DLL})
    ENDIF()
    FIND_FILE(EXIV2_DLL 
      NAMES exiv2.dll libexiv2.dll
      PATHS ${DLL_SEARCH_PATH}
            ${SOURCE_BASE_DIR}/exiv2/bin 
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(LIBEXPAT_DLL 
      NAMES libexpat.dll expat.dll
      PATHS ${DLL_SEARCH_PATH}
        ${SOURCE_BASE_DIR}/expat/bin 
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(LIBICONV_DLL 
      NAMES libiconv.dll iconv-2.dll
      PATHS ${DLL_SEARCH_PATH}
        ${SOURCE_BASE_DIR}/expat/bin 
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(LIBBROTLI_DLL 
      NAMES brotlidec.dll
      PATHS ${DLL_SEARCH_PATH}
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(LIBBROTLI_COMMON_DLL
      NAMES brotlicommon.dll
      PATHS ${DLL_SEARCH_PATH}
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    IF(BUILD_WITH_EPOXY)
      FIND_FILE(EPOXY_DLL
        NAMES epoxy-0.dll epoxy.dll
        PATHS ${DLL_SEARCH_PATH} ${SOURCE_BASE_DIR}/libepoxy/bin 
        NO_SYSTEM_ENVIRONMENT_PATH
      )
    ELSE()
      FIND_FILE(GLEW_DLL
        NAMES glew32.dll
        PATHS ${DLL_SEARCH_PATH}
              ${SOURCE_BASE_DIR}/glew/bin 
        NO_SYSTEM_ENVIRONMENT_PATH
      )
    ENDIF()

    FIND_FILE(LCMS2_DLL
      NAMES lcms2.dll liblcms2.dll liblcms2-2.dll lcms.dll
      PATHS ${DLL_SEARCH_PATH}
            ${LCMS2_ROOT_DIR}/bin 
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    # hand tuned dll, so that only necesarry dll are install and not all wxWidgets DLL to save space
    IF(MSVC)
      SET(WXSUFFIX vc)
    ELSE()
      IF(MINGW)
        SET(WXSUFFIX gcc)
      ELSE()
        MESSAGE(FATAL_ERROR "Unknown target for Win32 wxWidgets DLLs")
      ENDIF()
    ENDIF()
    FIND_PATH(
      WXWIDGETS_DLL_PATH
      NAME 
        wxbase330u_${WXSUFFIX}_custom.dll
        wxbase321u_${WXSUFFIX}_custom.dll
        wxbase320u_${WXSUFFIX}_custom.dll
        wxbase317u_${WXSUFFIX}_custom.dll
        wxbase316u_${WXSUFFIX}_custom.dll
        wxbase315u_${WXSUFFIX}_custom.dll
        wxbase314u_${WXSUFFIX}_custom.dll
        wxbase313u_${WXSUFFIX}_custom.dll
        wxbase312u_${WXSUFFIX}_custom.dll
        wxbase311u_${WXSUFFIX}_custom.dll
        wxbase310u_${WXSUFFIX}_custom.dll
        wxbase305u_${WXSUFFIX}_custom.dll
        wxbase304u_${WXSUFFIX}_custom.dll
        wxbase303u_${WXSUFFIX}_custom.dll
        wxbase302u_${WXSUFFIX}_custom.dll
        wxbase301u_${WXSUFFIX}_custom.dll
        wxbase300u_${WXSUFFIX}_custom.dll
      PATHS
        ${wxWidgets_LIB_DIR}
        ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    IF(NOT WXWIDGETS_DLL_PATH AND CMAKE_SIZEOF_VOID_P EQUAL 8) 
      # for 64 bit build check also variant with x64
      SET(WXSUFFIX "${WXSUFFIX}_x64")
      FIND_PATH(
        WXWIDGETS_DLL_PATH
        NAME 
          wxbase330u_${WXSUFFIX}_custom.dll
          wxbase321u_${WXSUFFIX}_custom.dll
          wxbase320u_${WXSUFFIX}_custom.dll
          wxbase317u_${WXSUFFIX}_custom.dll
          wxbase316u_${WXSUFFIX}_custom.dll
          wxbase315u_${WXSUFFIX}_custom.dll
          wxbase314u_${WXSUFFIX}_custom.dll
          wxbase313u_${WXSUFFIX}_custom.dll
          wxbase312u_${WXSUFFIX}_custom.dll
          wxbase311u_${WXSUFFIX}_custom.dll
          wxbase310u_${WXSUFFIX}_custom.dll
          wxbase305u_${WXSUFFIX}_custom.dll
          wxbase304u_${WXSUFFIX}_custom.dll
          wxbase303u_${WXSUFFIX}_custom.dll
          wxbase302u_${WXSUFFIX}_custom.dll
          wxbase301u_${WXSUFFIX}_custom.dll
          wxbase300u_${WXSUFFIX}_custom.dll
        PATHS
          ${wxWidgets_LIB_DIR}
          ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin
        NO_SYSTEM_ENVIRONMENT_PATH
      )
    ENDIF()
    MESSAGE(STATUS "wxWidgets DLL path: ${WXWIDGETS_DLL_PATH}")
    # first variant is for development versions with 3 numbers, second variant for stable versions with 2 numbers
    FILE(GLOB WXWIDGETS_DLL
      ${WXWIDGETS_DLL_PATH}/wxbase[2-3][0-9][0-9]u_${WXSUFFIX}*.dll     ${WXWIDGETS_DLL_PATH}/wxbase[2-3][0-9]u_${WXSUFFIX}*.dll
      ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9][0-9]u_core_${WXSUFFIX}*.dll ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9]u_core_${WXSUFFIX}*.dll
      ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9][0-9]u_xrc_${WXSUFFIX}*.dll  ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9]u_xrc_${WXSUFFIX}*.dll
      ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9][0-9]u_adv_${WXSUFFIX}*.dll  ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9]u_adv_${WXSUFFIX}*.dll
      ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9][0-9]u_gl_${WXSUFFIX}*.dll   ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9]u_gl_${WXSUFFIX}*.dll
      ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9][0-9]u_html_${WXSUFFIX}*.dll ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9]u_html_${WXSUFFIX}*.dll
      ${WXWIDGETS_DLL_PATH}/wxbase[2-3][0-9][0-9]u_xml_${WXSUFFIX}*.dll ${WXWIDGETS_DLL_PATH}/wxbase[2-3][0-9]u_xml_${WXSUFFIX}*.dll
      ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9][0-9]u_aui_${WXSUFFIX}*.dll  ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9]u_aui_${WXSUFFIX}*.dll
      ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9][0-9]u_qa_${WXSUFFIX}*.dll   ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9]u_qa_${WXSUFFIX}*.dll
      ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9][0-9]u_propgrid_${WXSUFFIX}*.dll   ${WXWIDGETS_DLL_PATH}/wxmsw[2-3][0-9]u_propgrid_${WXSUFFIX}*.dll
    )
    # some checking in ensure all is found okay
    list(LENGTH WXWIDGETS_DLL COUNT_WXWIDGETS_DLL)
    IF(NOT ${COUNT_WXWIDGETS_DLL} EQUAL 10)
      MESSAGE(FATAL_ERROR "Not all necessary wxWidgets dlls could be found.")
    ENDIF()

    INSTALL(FILES ${TIFF_DLL} ${JPEG_DLL} ${PNG_DLL} ${ZLIB_DLL} ${OPENEXR_DLL} ${VIGRA_DLL}
        ${BOOST_DLLs} ${EXIV2_DLL} ${LCMS2_DLL}
        ${WXWIDGETS_DLL}
        DESTINATION ${BINDIR}
    )
    IF(LIBEXPAT_DLL)
      INSTALL(FILES ${LIBEXPAT_DLL}  DESTINATION ${BINDIR})
    ENDIF()
    IF(LZMA_DLL)
      INSTALL(FILES ${LZMA_DLL} DESTINATION ${BINDIR})
    ENDIF()
    IF(LIBICONV_DLL)
      INSTALL(FILES ${LIBICONV_DLL} DESTINATION ${BINDIR})
    ENDIF()
    IF(LIBBROTLI_DLL)
        INSTALL(FILES ${LIBBROTLI_DLL} ${LIBBROTLI_COMMON_DLL} DESTINATION ${BINDIR})
    ENDIF()
    IF(BUILD_WITH_EPOXY)
      #install epoxy dll
      INSTALL(FILES ${EPOXY_DLL} DESTINATION ${BINDIR})
    ELSE()
      #install glew dll
      INSTALL(FILES ${GLEW_DLL} DESTINATION ${BINDIR})
    ENDIF()

    FIND_FILE(SQLITE3_DLL 
        NAMES sqlite3.dll libsqlite3.dll 
        PATHS ${DLL_SEARCH_PATH}
          ${SOURCE_BASE_DIR}/sqlite3 
        NO_SYSTEM_ENVIRONMENT_PATH
    )
    INSTALL(FILES ${SQLITE3_DLL} DESTINATION ${BINDIR})

    IF(HAVE_FFTW)
      FIND_FILE(FFTW3_DLL 
        NAMES libfftw-3.3.dll fftw3.dll
        PATHS ${DLL_SEARCH_PATH}
              ${SOURCE_BASE_DIR}/fftw-3.3.4/fftw-3.3-libs/x64/Release
              ${SOURCE_BASE_DIR}/fftw-3.3.4/fftw-3.3-libs/x64 
              ${SOURCE_BASE_DIR}/fftw-3.3.4/fftw-3.3-libs/
              ${SOURCE_BASE_DIR}/fftw-3.3.3/fftw-3.3-libs/x64 
              ${SOURCE_BASE_DIR}/fftw-3.3.3/fftw-3.3-libs/
          NO_SYSTEM_ENVIRONMENT_PATH)
      INSTALL(FILES ${FFTW3_DLL} DESTINATION ${BINDIR})
    ENDIF()

    IF(FLANN_FOUND)
      FIND_FILE(FLANN_DLL 
        NAMES flann_cpp.dll
        PATHS ${DLL_SEARCH_PATH}
          NO_SYSTEM_ENVIRONMENT_PATH)
      INSTALL(FILES ${FLANN_DLL} DESTINATION ${BINDIR})
    ENDIF()

    IF(LAPACK_FOUND)
      FIND_FILE(BLAS_DLL
        NAMES openblas.dll
        PATHS ${DLL_SEARCH_PATH}
          NO_SYSTEM_ENVIRONMENT_PATH
      )
      INSTALL(FILES ${BLAS_DLL} DESTINATION ${BINDIR})
    ENDIF()

  ENDIF()
ENDIF(WIN32)

