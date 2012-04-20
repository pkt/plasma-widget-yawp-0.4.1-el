include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_BINARY_DIR}/logger
	)

set(yawp_SRCS
	configdialog/yawpconfig.cpp
	configdialog/dlgaddcity.cpp
	configdialog/dlgtimezone.cpp

	../logger/streamlogger.cpp

	painter/abstractpainter.cpp
	painter/desktoppainter.cpp
	painter/panelpainter.cpp

	countrymap.cpp
	ionlistmodel.cpp
	pageanimator.cpp
	paneldesktopinterface.cpp
	statemachine.cpp
	utils.cpp
	weatherdataprocessor.cpp
	weatherservice.cpp
	yawp.cpp
	yawpday.cpp
	yawpdefines.cpp
	)

kde4_add_ui_files(yawp_SRCS
	ui/yawplocationspage.ui
	ui/yawpsettingspage.ui
	ui/yawppanelpage.ui
	ui/yawpthemepage.ui
	ui/dlgaddcity.ui
	ui/dlgtimezone.ui
	)

set(plasma_include_libs
	${QT_QTXML_LIBRARY}
	${KDE4_KDEUI_LIBS}
	${KDE4_KIO_LIBS}
	${KDE4_SOLID_LIBS}
	${PLASMA_LIBS}
	${KDE4WORKSPACE_WEATHERION_LIBRARY}
	${KDE4_KUNITCONVERSION_LIBS}
	)

## Add Qt4 resource to binary
set(YAWP_RCCS yawpresource.qrc )
qt4_add_resources(YAWP_RCCS_SRCS	${YAWP_RCCS})

kde4_add_plugin(plasma_applet_yawp		${yawp_SRCS} ${YAWP_RCCS_SRCS})
target_link_libraries(plasma_applet_yawp	${plasma_include_libs})

configure_file(plasma-applet-yawp.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/plasma-applet-yawp.desktop)

install(TARGETS        plasma_applet_yawp
        DESTINATION    ${PLUGIN_INSTALL_DIR}
        )
install(FILES           ${CMAKE_CURRENT_BINARY_DIR}/plasma-applet-yawp.desktop
        DESTINATION     ${SERVICES_INSTALL_DIR}
        PERMISSIONS     OWNER_READ GROUP_READ WORLD_READ)
install(FILES           yawp_theme15.svg
        DESTINATION     ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/
        )


## This is needed for the CPack package generator.
#
INSTALL(TARGETS               plasma_applet_yawp
        LIBRARY DESTINATION   ${YAWP_PACK_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION   ${YAWP_PACK_INSTALL_LIB_DIR}
        RUNTIME DESTINATION   ${YAWP_PACK_INSTALL_LIB_DIR}
        COMPONENT Libraries
        )

INSTALL(FILES         ${CMAKE_CURRENT_BINARY_DIR}/plasma-applet-yawp.desktop
        DESTINATION   ${YAWP_PACK_SERVICES_DIR}
        COMPONENT     Libraries
        )

INSTALL(FILES         yawp_theme15.svg
        DESTINATION   ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/
        COMPONENT     ApplicationData
        )
