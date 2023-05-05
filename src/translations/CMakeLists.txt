# This file is part of hugin, a GUI for the panorama tools suite
# Licence details can be found in the file COPYING.txt.
#
# Copyright (c) 2013 Kornel Benko, <Kornel.Benko@berlin.de>
#

find_package(MSGFMT QUIET REQUIRED)

# simply use all po files and hugin.pot from the source directory
FILE(GLOB PO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.po")

ADD_TRANSLATIONS("${CMAKE_CURRENT_SOURCE_DIR}/hugin.pot" ${PO_FILES})

IF(APPLE)
  add_custom_command(
    TARGET translations
    POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/mac/CopyTranslations.sh ${CMAKE_CURRENT_BINARY_DIR})
ENDIF()
