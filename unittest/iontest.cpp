/*************************************************************************\
*   Copyright (C) 2009 by Ulf Kreißig                                     *
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

//--- LOCAL ---
#include "../config.h"
#include "iontest.h"
#include "logger/streamlogger.h"
#include "ions/units.h"

//--- QT4 ---
#include <QTimer>

//--- KDE4 ---
#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KMessageBox>
#include <KUnitConversion/Value>
/*#include <plasma/weather/weatherutils.h> */

IonTester::IonTester( QObject * parent )
	: QObject(parent),
	  m_ion( this, QVariantList() )
{
      m_ion.init();
}
IonTester::~IonTester()
{
}
void
IonTester::connectSource( const QString & source )
{
	m_ion.updateIonSource( source );
	m_ion.connectSource( source, this );
}
void
IonTester::dataUpdated(const QString & sourceName, const Plasma::DataEngine::Data & data)
{
	dDebug() << sourceName << data;
}

void
IonTester::quit()
{
	connectSource( QString("envcan|validate|Park") );
	QTimer::singleShot(30*1000, qApp, SLOT(quit()));
}

void
IonTester::reset()
{
	m_ion.reset();
}


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
		ki18n("Copyright (c) 2009 Ulf Kreißig") );
	aboutData.addAuthor(
		ki18n("Ulf Kreißig"),
		ki18n("Main developer"),
		"udev@gmx.net");

	KCmdLineArgs::init( argc, argv, &aboutData );
	KApplication app;
	

	KUnitConversion::Value v( 61.0, KUnitConversion::Fahrenheit);
	dTracing() << "Convert 61 F to Celcius:" << v.convertTo(KUnitConversion::Celsius).number();



	IonTester tester;

/*	tester.connectSource( "accuweather|weather|Berlin, Germany(Berlin)|EUR.DE.GM003.BERLIN" );
	tester.reset();
	tester.connectSource( "accuweather|validate|Dresden" );
*/	tester.connectSource( "accuweather|weather|Milwaukee, NC|27854" );

//	tester.connectSource( "google|weather|Berlin" );
//	tester.reset();
//	tester.connectSource("google|validate|springfield, ma");
//	tester.connectSource("google|weather|Springfield, MA");
//	tester.connectSource("google|weather|Hartford, CT");
//	dDebug() << WeatherUtils::convert( 5.0, WeatherUtils::Miles, WeatherUtils::Kilometers );

	QTimer::singleShot(30*1000, &app, SLOT(quit()));
	return app.exec();
}
