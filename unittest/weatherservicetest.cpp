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

//#ifdef HAVE_CONFIG_H
#include "../config.h"
//#endif

#include <stdio.h>
#include <stdlib.h>

#include "weatherservice.h"
#include "countrymap.h"
#include "yawpdefines.h"
#include "logger/streamlogger.h"

#include <QByteArray>
#include <QFile>
#include <QTextStream>

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KStandardDirs>
#include <KLocale>

#include <Solid/Networking>

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


///////////////////////////////////////////////////////
/*	foreach(QString cc, KGlobal::locale()->allCountriesList())
	{
		dDebug() << "key =" << cc
		         << "   name =" << KGlobal::locale()->countryCodeToName(cc);
	}
	dDebug() << KGlobal::locale()->dateFormat();
	dDebug() << (KGlobal::locale()->measureSystem() == KLocale::Metric ? "Metric" : "Imperial");

	QDateTime date = QDateTime::currentDateTime().addDays(-2);
	dDebug() << KGlobal::locale()->formatDateTime( date, KLocale::ShortDate, true );
	dDebug() << KGlobal::locale()->formatDateTime( date, KLocale::LongDate, true );
	dDebug() << KGlobal::locale()->formatDateTime( date, KLocale::FancyShortDate, true );
	dDebug() << KGlobal::locale()->formatDateTime( date, KLocale::FancyLongDate, true );
*/

//	QPixmap pix = CountryMap::instance()->getIconForCountryCode(QLatin1String("de"));
//	pix.save("/tmp/de.png");

	Yawp::Storage storage;
	WeatherServiceModel model(&storage);
	dDebug() << "model contains " << model.rowCount() << "cities";

	CityWeather city;
	city.setCity( "New York" );
	city.setProvider( "noaa" );
	model.addCity( city );

	city.setCity( "Milwaukee, WI");
	city.setProvider( "bbcukmet" );
	model.addCity( city );

	model.moveCity(  1,0 );

	dDebug() << "model contains " << model.rowCount() << "cities";
	for( int i = 0; i < model.rowCount(); ++i )
	{
		const CityWeather * pCity = model.getCityInfo(i);
		dDebug() << pCity->city() << pCity->provider() << pCity->extraData();
	}

	WeatherServiceModel model2(&storage);
	model2.copyCities( model );

	dDebug() << "model contains " << model2.rowCount() << "cities";
	for( int i = 0; i < model2.rowCount(); ++i )
	{
		const CityWeather * pCity = model2.getCityInfo(i);
		dDebug() << pCity->city() << pCity->provider() << pCity->extraData();
	}

	return 0;
}
