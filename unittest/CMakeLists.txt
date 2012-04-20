include_directories(
#	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}/../applet/
	${CMAKE_CURRENT_SOURCE_DIR}/../ions/
	${CMAKE_CURRENT_BINARY_DIR}/../
	)

set(unittests_include_libs
	${QT_QTXML_LIBRARY}
	${KDE4_KDEUI_LIBS}
	${KDE4_KIO_LIBS}
	${KDE4_SOLID_LIBS}
	${PLASMA_LIBS}
	${KDE4WORKSPACE_WEATHERION_LIBRARY}
	${KDE4_KUNITCONVERSION_LIBS}
	streamlogger
	)

## Add Qt4 resource to binary
set(YAWP_RCCS ../applet/yawpresource.qrc )
qt4_add_resources(YAWP_RCCS_SRCS	${YAWP_RCCS})


#### Streamlogger Test ####
set(streamloggertest_SRCS
	streamloggertest.cpp
	)

kde4_add_executable(streamloggertest		${streamloggertest_SRCS})
target_link_libraries(streamloggertest		${unittests_include_libs})


#### WeatherService Test ####
set(weatherservicetest_SRCS
	weatherservicetest.cpp
	../applet/configdialog/dlgtimezone.cpp
	../applet/countrymap.cpp
	../applet/ionlistmodel.cpp
	../applet/weatherdataprocessor.cpp
	../applet/weatherservice.cpp
	../applet/yawpday.cpp
	../applet/yawpdefines.cpp
	../applet/utils.cpp
	)

kde4_add_ui_files( weatherservicetest_SRCS
	../applet/ui/dlgaddcity.ui
	../applet/ui/dlgtimezone.ui
	)

kde4_add_executable(weatherservicetest		${weatherservicetest_SRCS} ${YAWP_RCCS_SRCS})
target_link_libraries(weatherservicetest	${unittests_include_libs})


#### ConfigDialog Test ####
set( configtest_SRCS
	configtest.cpp
	../applet/countrymap.cpp
	../applet/ionlistmodel.cpp
	../applet/utils.cpp
	../applet/yawpday.cpp
	../applet/yawpdefines.cpp
#	../applet/weatherdataprocessor.cpp
#	../applet/weatherservice.cpp
#	../applet/configdialog/dlgaddcity.cpp
#	../applet/configdialog/yawpconfig.cpp
	../applet/configdialog/dlgaddcity.cpp
	../applet/configdialog/dlgtimezone.cpp
	)

kde4_add_ui_files( configtest_SRCS
#	../applet/ui/yawplocationspage.ui
#	../applet/ui/yawpsettingspage.ui
#	../applet/ui/yawppanelpage.ui
#	../applet/ui/yawpthemepage.ui

	../applet/ui/dlgaddcity.ui
	../applet/ui/dlgtimezone.ui
	)

kde4_add_executable(configtest			${configtest_SRCS} ${YAWP_RCCS_SRCS})
target_link_libraries(configtest		${unittests_include_libs})


#### ION WeatherEngine Test ####
set( weatherenginetest_SRCS
	weatherenginetest.cpp
	../applet/countrymap.cpp
	../applet/ionlistmodel.cpp
	../applet/utils.cpp
	../applet/yawpday.cpp
	../applet/yawpdefines.cpp
	)

kde4_add_executable(weatherenginetest		${weatherenginetest_SRCS} ${YAWP_RCCS_SRCS})
target_link_libraries(weatherenginetest		${unittests_include_libs})


#### ION Test ####
set( iontest_SRCS
	iontest.cpp
#	../applet/countrymap.cpp
#	../applet/ionlistmodel.cpp
#	../applet/utils.cpp
#	../applet/yawpday.cpp
#	../applet/yawpdefines.cpp
#	../ions/ion_accuweather.cpp
#	../ions/ion_google.cpp
	../ions/ion_accuweather.cpp
	)

kde4_add_executable(iontest		${iontest_SRCS} ${YAWP_RCCS_SRCS})
target_link_libraries(iontest	${unittests_include_libs})


#### PLASMOID Test ####
set( plasmoidtest_SRCS
	plasmoidtest_main.cpp
	plasmoidtest_fullview.cpp
	../applet/configdialog/yawpconfig.cpp
	../applet/configdialog/dlgaddcity.cpp
	../applet/configdialog/dlgtimezone.cpp

	../applet/painter/abstractpainter.cpp
	../applet/painter/desktoppainter.cpp
	../applet/painter/panelpainter.cpp

	../applet/countrymap.cpp
	../applet/ionlistmodel.cpp
	../applet/pageanimator.cpp
	../applet/paneldesktopinterface.cpp
	../applet/statemachine.cpp
	../applet/utils.cpp
	../applet/weatherdataprocessor.cpp
	../applet/weatherservice.cpp
	../applet/yawp.cpp
	../applet/yawpday.cpp
	../applet/yawpdefines.cpp
	)

kde4_add_ui_files(plasmoidtest_SRCS
	../applet/ui/yawplocationspage.ui
	../applet/ui/yawpsettingspage.ui
	../applet/ui/yawppanelpage.ui
	../applet/ui/yawpthemepage.ui
	../applet/ui/dlgaddcity.ui
	../applet/ui/dlgtimezone.ui
	)

kde4_add_executable(plasmoidtest		${plasmoidtest_SRCS} ${YAWP_RCCS_SRCS})
target_link_libraries(plasmoidtest		${unittests_include_libs})
