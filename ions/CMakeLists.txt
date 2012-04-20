if ( ${KDE_VERSION} VERSION_GREATER "4.2.9" )
	set( ACCUWEATHER_DESKTOP kde43/ion-accuweather.desktop)
	set( GOOGLE_DESKTOP kde43/ion-google.desktop)
	set( WUNDERGROUND_DESKTOP kde43/ion-wunderground.desktop)
else ( ${KDE_VERSION} VERSION_GREATER "4.2.9" )
	set( ACCUWEATHER_DESKTOP ion-accuweather.desktop)
	set( GOOGLE_DESKTOP ion-google.desktop)
	set( WUNDERGROUND_DESKTOP ion-wunderground.desktop)
endif ( ${KDE_VERSION} VERSION_GREATER "4.2.9" )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_BINARY_DIR}/logger
	)

set(weather_ion_libs
	${QT_QTXML_LIBRARY}
	${KDE4_KDEUI_LIBS}
	${KDE4_KIO_LIBS}
	${PLASMA_LIBS}
	${KDE4WORKSPACE_WEATHERION_LIBRARY}
	${KDE4_KUNITCONVERSION_LIBS}
	)

message(STATUS "yaWP sevice install dir: " ${YAWP_PACK_SERVICES_DIR})
message(STATUS "yaWP plugin install dir: " ${YAWP_PACK_INSTALL_LIB_DIR})
message(STATUS "yaWP application install dir: " ${YAWP_PACK_APPS_DIR})


set(ion_accuweather_SRCS
	ion_accuweather.cpp
	../logger/streamlogger.cpp
	)

kde4_add_plugin       (ion_accuweather           ${ion_accuweather_SRCS})
target_link_libraries (ion_accuweather           ${weather_ion_libs})
install(  TARGETS      ion_accuweather           DESTINATION    ${PLUGIN_INSTALL_DIR})
install(  FILES        ${ACCUWEATHER_DESKTOP}    DESTINATION    ${SERVICES_INSTALL_DIR})

set(ion_google_SRCS
	ion_google.cpp
	../logger/streamlogger.cpp
	)

kde4_add_plugin       (ion_google            ${ion_google_SRCS})
target_link_libraries (ion_google            ${weather_ion_libs})
install(  TARGETS      ion_google            DESTINATION    ${PLUGIN_INSTALL_DIR})
install(  FILES        ${GOOGLE_DESKTOP}     DESTINATION    ${SERVICES_INSTALL_DIR})


set(ion_wunderground_SRCS
	ion_wunderground.cpp
	../logger/streamlogger.cpp
	)

kde4_add_plugin       (ion_wunderground            ${ion_wunderground_SRCS})
target_link_libraries (ion_wunderground            ${weather_ion_libs})
install(  TARGETS      ion_wunderground            DESTINATION    ${PLUGIN_INSTALL_DIR})
install(  FILES        ${WUNDERGROUND_DESKTOP}     DESTINATION    ${SERVICES_INSTALL_DIR})


## This is needed by CPack package generator.
#
INSTALL(TARGETS       ion_accuweather ion_google ion_wunderground
	LIBRARY       DESTINATION "${YAWP_PACK_INSTALL_LIB_DIR}"
	ARCHIVE       DESTINATION "${YAWP_PACK_INSTALL_LIB_DIR}"
	RUNTIME       DESTINATION "${YAWP_PACK_INSTALL_LIB_DIR}"
	COMPONENT     Libraries
	)

INSTALL(FILES         ${ACCUWEATHER_DESKTOP} ${GOOGLE_DESKTOP} ${WUNDERGROUND_DESKTOP}
	DESTINATION   ${YAWP_PACK_SERVICES_DIR}
	COMPONENT     Libraries
	)
