PROJECT(plasma-applet-yawp)
cmake_minimum_required(VERSION 2.6)

#if you don't want the full compiler output, remove the following line
SET(CMAKE_VERBOSE_MAKEFILE ON)

#add definitions, compiler switches, etc.
#ADD_DEFINITIONS(-Wall -O2)

#  CMAKE_BUILD_TYPE:
#  A variable which controls the type of build when using a single-configuration generator like the Makefile generator.
#  CMake will create by default the following variables when using a single-configuration generator:
#
#    * None (CMAKE_C_FLAGS or CMAKE_CXX_FLAGS used)
#    * Debug (CMAKE_C_FLAGS_DEBUG or CMAKE_CXX_FLAGS_DEBUG)
#    * Release (CMAKE_C_FLAGS_RELEASE or CMAKE_CXX_FLAGS_RELEASE)
#    * RelWithDebInfo (CMAKE_C_FLAGS_RELWITHDEBINFO or CMAKE_CXX_FLAGS_RELWITHDEBINFO
#    * MinSizeRel (CMAKE_C_FLAGS_MINSIZEREL or CMAKE_CXX_FLAGS_MINSIZEREL) 


## Find the required Libaries
##
FIND_PACKAGE(KDE4 REQUIRED)
INCLUDE(KDE4Defaults)
INCLUDE(MacroLibrary)

find_package(KDE4Workspace QUIET)

## Need a better way to find cmake scripts at arb locations
if ( NOT ${KDE4Workspace_FOUND} )
  message(STATUS "COULD NOT FIND KDE4Workspace TRYING /usr/lib64/cmake/KDE4Workspace-4.3.1")
  SET (KDE4Workspace_DIR /usr/lib64/cmake/KDE4Workspace-4.3.1)
  find_package(KDE4Workspace REQUIRED)
endif ( NOT ${KDE4Workspace_FOUND} )

if ( ${KDE_VERSION} VERSION_LESS "4.1.90" )
	find_package(Plasma REQUIRED)
else ( ${KDE_VERSION} VERSION_LESS "4.1.90" )
	set( PLASMA_LIBS ${KDE4_PLASMA_LIBS} )
endif ( ${KDE_VERSION} VERSION_LESS "4.1.90" )

if ( ${KDE_VERSION} VERSION_GREATER "4.2.9" )
	set( KDE4WORKSPACE_WEATHERION_LIBRARY weather_ion)
endif ( ${KDE_VERSION} VERSION_GREATER "4.2.9" )

INCLUDE_DIRECTORIES(
	${QT_INCLUDES}
	${KDE4_INCLUDE_DIR}
	${KDE4_INCLUDES}
	${KDE4WORKSPACE_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	)

ADD_DEFINITIONS(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

## enable or disable the src/logger/streamlogger.cpp log-system
## when disabling the logsystem, no debug output occurs
IF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo" )
	## Loglevel needs to be valid, when streamlogger_definitions.h will be build.
	IF( NOT DEBUG_LOGLEVEL )
		SET(DEBUG_LOGLEVEL "Debug")
	ENDIF( NOT DEBUG_LOGLEVEL )

	## enable logging and create header that contains the settings (loglevel and logfile)
	ADD_DEFINITIONS( -DENABLE_DSTREAMLOGGER )
	CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/logger/streamlogger_definitions.h.in ${CMAKE_BINARY_DIR}/logger/streamlogger_definitions.h)
ENDIF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo" )

# Choose the type of build.  Example: SET(CMAKE_BUILD_TYPE Debug) 
MESSAGE( STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE} )

# the compiler flags for compiling C sources 
MESSAGE( STATUS "CMAKE_C_FLAGS: " ${CMAKE_C_FLAGS} )

# If set, runtime paths are not added when using shared libraries. Default it is set to OFF
MESSAGE( STATUS "CMAKE_SKIP_RPATH: " ${CMAKE_SKIP_RPATH} )
MESSAGE( STATUS "DEBUG_LOGLEVEL: " ${DEBUG_LOGLEVEL} )
MESSAGE( STATUS "DEBUG_LOGFILE: " ${DEBUG_LOGFILE} )

############################################################
##
##    YAWP specific settings
##

##	DEFINE THE PACKAGE VERSION AND NAME
SET(MAJOR_VERSION 0)
SET(MINOR_VERSION 4)
SET(PATCH_VERSION 1)
SET(YAWP_NAME     "yaWP")

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
macro_display_feature_log()

### Define some variables we need in subdires applet and ions
STRING(REGEX REPLACE "/usr/" ""  YAWP_PACK_INSTALL_LIB_DIR "${PLUGIN_INSTALL_DIR}")
STRING(REGEX REPLACE "/usr/" ""  YAWP_PACK_SERVICES_DIR    "${SERVICES_INSTALL_DIR}")
STRING(REGEX REPLACE "/usr/" ""  YAWP_PACK_APPS_DIR        "${DATA_INSTALL_DIR}")


add_subdirectory(applet)
add_subdirectory(ions)
add_subdirectory(po)
IF( "${BUILD_UNITTESTS}" STREQUAL "YES" )
	## This will create a static lib. We can link a program against a static
	## but we can not link a plugin against a static lib. At least i don not know how to do it,
	## So, if you know how to do it, please let me know.
	add_subdirectory(logger)
	add_subdirectory(unittest)
ENDIF( "${BUILD_UNITTESTS}" STREQUAL "YES" )

############################################################
##
##    Package Generator
##
IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
  MESSAGE(STATUS "CMakeRoot: ${CMAKE_ROOT}/Modules/CPack.cmake")
  INCLUDE(InstallRequiredSystemLibraries)

  SET(CPACK_SET_DESTDIR "on")
  SET(CPACK_PACKAGING_INSTALL_PREFIX "/tmp")
  SET(CPACK_GENERATOR "DEB;RPM;STGZ;TBZ2;TGZ;TZ;ZIP;SOURCE_TGZ;SOURCE_TZ;SOURCE_ZIP;")
  SET(CPACK_SOURCE_IGNORE_FILES 
      "/build/"
      "/.svn/"
      ".kdev4$"
      "~$"
      )

  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY	"Yet Another Weather Plasmoid")

  SET(CPACK_PACKAGE_VENDOR		"Plasma Factory")
  SET(CPACK_PACKAGE_CONTACT		"Plasma Factory <plasmafactory@jcell.co.za>, Ruan Strydom <ruans@kr8.co.za>, Ulf Kreißig <udev@gmx.net>, Marián Kyral <mkyral@email.cz>, Ezequiel R. Aguerre <ezeaguerre@gmail.com>, Pierpaolo Vittorini  <pierpaolo.vittorini@gmail.com>")
#	SET(CPACK_RESOURCE_FILE_LICENSE		${CMAKE_CURRENT_SOURCE_DIR}//COPYING)
  SET(CPACK_PACKAGE_VERSION_MAJOR	${MAJOR_VERSION})
  SET(CPACK_PACKAGE_VERSION_MINOR	${MINOR_VERSION})	# to create a debian confirm file-version-string
  STRING(REGEX REPLACE "_" "-" CPACK_PACKAGE_VERSION_PATCH ${PATCH_VERSION})
  SET(CPACK_PACKAGE_FILE_NAME		${CMAKE_PROJECT_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}_${CMAKE_SYSTEM_PROCESSOR})
  SET(CPACK_SOURCE_PACKAGE_FILE_NAME	${CMAKE_PROJECT_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}_source)

  ##	LIST EXECUTABLES
  SET(CPACK_STRIP_FILES "lib//plasma_applet_yawp.so;lib//ion_accuweather.so;lib//ion_google.so;lib/ion_wunderground.so")
  SET(CPACK_COMPONENTS_ALL Libraries ApplicationData)

  ##	DEBIAN SPECIFIC SETTINGS
  ##
  SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION	"A weather display plasma widget for KDE 4\n Yet Another Weather Plasmoid that shows the current weather conditions\n and up to 4 days forecast depending on the chosen weather engine (ion).\n This package also contains two ions (google and accuweather).")
  SET(CPACK_DEBIAN_PACKAGE_PRIORITY	"optional")
  SET(CPACK_DEBIAN_PACKAGE_SECTION	"kde")
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS	"kdebase-runtime (>= 4:4.2.1), kdelibs5 (>= 4:4.2.1), libc6 (>= 2.1.3), libgcc1 (>= 1:4.1.1), libplasma3, libqt4-dbus (>= 4.5.0~+rc1), libqt4-network (>= 4.5.0~+rc1), libqt4-svg (>= 4.5.0~+rc1), libqt4-xml (>= 4.5.0~+rc1), libqtcore4 (>= 4.5.0~+rc1), libqtgui4 (>= 4.5.0~+rc1), libstdc++6 (>= 4.2.1)")

  FIND_PROGRAM(DPKG_CMD dpkg)
  IF(DPKG_CMD)
    EXECUTE_PROCESS(COMMAND "${DPKG_CMD}" --print-architecture
      OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET(CPACK_DEBIAN_PACKAGE_NAME	${CMAKE_PROJECT_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
  ENDIF(DPKG_CMD)

  ##	RPM SPECIFIC SETTINGS
  ##

  INCLUDE(CPack)
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")


## On debian systems we install or add the following files to the package
## /usr/share/doc/plasma-applet-yawp
##   copyright
##   README.gz
##   changelog.Debian.gz
##   TODO
IF(EXISTS "/etc/debian_version")
  MESSAGE(STATUS "Debian System detected...")
  IF(NOT EXISTS "/bin/gzip")
    MESSAGE(FATAL_ERROR "Program gzip is missing on your system.")
  ENDIF(NOT EXISTS "/bin/gzip")

  ADD_CUSTOM_TARGET(debian_package
                    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/README.gz
                            ${CMAKE_CURRENT_BINARY_DIR}/changelog.Debian.gz)

  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/README.gz
                     COMMAND /bin/cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/README ${CMAKE_CURRENT_BINARY_DIR}/README
                     COMMAND /bin/gzip ARGS -9 ${CMAKE_CURRENT_BINARY_DIR}/README
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/README)

  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/changelog.Debian.gz
                     COMMAND /bin/cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG ${CMAKE_CURRENT_BINARY_DIR}/changelog.Debian
                     COMMAND /bin/gzip ARGS -9 ${CMAKE_CURRENT_BINARY_DIR}/changelog.Debian
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG)

  INSTALL(FILES         ${CMAKE_CURRENT_SOURCE_DIR}/COPYRIGHT
          DESTINATION   /usr/share/doc/${CMAKE_PROJECT_NAME}
          RENAME        copyright
          COMPONENT     ApplicationData
         )
  INSTALL(FILES         ${CMAKE_CURRENT_BINARY_DIR}/README.gz
                        ${CMAKE_CURRENT_BINARY_DIR}/changelog.Debian.gz
                        TODO
          DESTINATION   /usr/share/doc/${CMAKE_PROJECT_NAME}
          COMPONENT     ApplicationData
         )

  ## this might be dirty, but the target debian_package needs to be called in order to create some files for a debian package
  ## and this was the only idea i had...
  ## if anybody has a better idea, how to generate and add the files in /usr/share/doc/plasma-widget-yawp/
  ## to the debian package, please let me know
  ADD_DEPENDENCIES(plasma_applet_yawp debian_package)
ENDIF(EXISTS "/etc/debian_version")
