include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	)

set(streamlogger_libs
	${QT_QTCORE_LIBRARY}
	)

add_library(streamlogger STATIC streamlogger.cpp)
target_link_libraries(streamlogger ${streamlogger_libs})
