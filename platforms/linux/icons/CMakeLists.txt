# install icon files in different sizes
INSTALL(FILES hugin_16.png DESTINATION ${DATADIR}/icons/hicolor/16x16/apps RENAME hugin.png)
INSTALL(FILES hugin_32.png DESTINATION ${DATADIR}/icons/hicolor/32x32/apps RENAME hugin.png)
INSTALL(FILES hugin_48.png DESTINATION ${DATADIR}/icons/hicolor/48x48/apps RENAME hugin.png)
INSTALL(FILES hugin_128.png DESTINATION ${DATADIR}/icons/hicolor/128x128/apps RENAME hugin.png)
INSTALL(FILES hugin_256.png DESTINATION ${DATADIR}/icons/hicolor/256x256/apps RENAME hugin.png)

INSTALL(FILES ptbatcher_16.png DESTINATION ${DATADIR}/icons/hicolor/16x16/apps RENAME ptbatcher.png)
INSTALL(FILES ptbatcher_32.png DESTINATION ${DATADIR}/icons/hicolor/32x32/apps RENAME ptbatcher.png)
INSTALL(FILES ptbatcher_48.png DESTINATION ${DATADIR}/icons/hicolor/48x48/apps RENAME ptbatcher.png)
INSTALL(FILES ptbatcher_128.png DESTINATION ${DATADIR}/icons/hicolor/128x128/apps RENAME ptbatcher.png)
INSTALL(FILES ptbatcher_256.png DESTINATION ${DATADIR}/icons/hicolor/256x256/apps RENAME ptbatcher.png)

#finally also svg version
INSTALL(FILES hugin.svg ptbatcher.svg DESTINATION ${DATADIR}/icons/hicolor/scalable/apps)
