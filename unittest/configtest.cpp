/*************************************************************************\
*   Copyright (C) 2009 by Ulf Kreissig                                    *
*   udev@gmx.net                                                          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
\*************************************************************************/

#include "../config.h"

#include "configdialog/dlgaddcity.h"
#include "ionlistmodel.h"
#include "yawpdefines.h"
#include "logger/streamlogger.h"

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>

#include <Plasma/DataEngine>
#include <Plasma/DataEngineManager>

int
main(int argc, char *argv[])
{
	KAboutData aboutData(
		"weatherservicetest",		// appname
		0,                   		// catalogName
		ki18n("WeatherServiceTest"),	// programName
		YAWP_VERSION_STRING,        	// version
		ki18n("Simple Unit Test Application"),
		KAboutData::License_GPL,
		ki18n("Copyright (c) 2009 Ulf Kreissig") );
	aboutData.addAuthor(
		ki18n("Ulf Kreissig"),
		ki18n("Main developer"),
		"udev@gmx.net");

	KCmdLineArgs::init( argc, argv, &aboutData );
	KApplication app;

	Plasma::DataEngine * pDataEngine = Plasma::DataEngineManager::self()->loadEngine("weather");
	Yawp::Storage storage;
	storage.setEngine( pDataEngine );
/*	dDebug() << pDataEngine->query("ions");
	return 0;
*/
	DlgAddCity dlg( &storage );
	dlg.open();
	
	return app.exec();
}
