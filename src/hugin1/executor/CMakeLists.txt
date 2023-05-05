ADD_EXECUTABLE(hugin_executor hugin_executor.cpp)

TARGET_LINK_LIBRARIES(hugin_executor huginbase huginbasewx ${wxWidgets_LIBRARIES}  ${common_libs})
INSTALL(TARGETS hugin_executor DESTINATION ${BINDIR})

FILE(GLOB EXECUTOR_FILES_OUTPUT user_defined_output/*.executor)
INSTALL(FILES ${EXECUTOR_FILES_OUTPUT} DESTINATION ${HUGINDATADIR}/data/output)

FILE(GLOB EXECUTOR_FILES_ASSISTANT user_defined_assistant/*.assistant)
INSTALL(FILES ${EXECUTOR_FILES_ASSISTANT} DESTINATION ${HUGINDATADIR}/data/assistant)
