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

#include "weatherenginetest.h"
#include "yawpday.h"
#include "logger/streamlogger.h"

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KMessageBox>
#include <Plasma/DataEngineManager>

static const QString bbcukmet_weather_Charlestown("bbcukmet|weather|Charlestown|http://feeds.bbc.co.uk/weather/feeds/obs/world/4177.xml");
static const QString bbcukmet_weather_Berlin("bbcukmet|weather|Berlin - Brandenburg International|http://feeds.bbc.co.uk/weather/feeds/obs/world/4094.xml");

WeatherEngineTest::WeatherEngineTest( QObject * parent )
	: QObject(parent)
{
}

WeatherEngineTest::~WeatherEngineTest()
{
}

bool
WeatherEngineTest::init()
{
	m_pWeatherEngine = Plasma::DataEngineManager::self()->loadEngine("weather");
	if( m_pWeatherEngine->isValid() )
	{
		dDebug() << m_pWeatherEngine->query("ions");
		m_pWeatherEngine->connectSource("bbcukmet|validate|Berlin", this);
//		m_pWeatherEngine->connectSource(QLatin1String("noaa|weather|Charlestown/Newcast, AG"), this);
//	m_pWeatherEngine->connectSource("bbcukmet|weather|Charlestown|http://feeds.bbc.co.uk/weather/feeds/obs/world/4177.xml", this);
	}
	return m_pWeatherEngine->isValid();

}

void
WeatherEngineTest::dataUpdated(const QString & sAction, const Plasma::DataEngine::Data & data)
{
	// Check if we validate or not
	QString location = sAction;
//	location[0] = location[0].toUpper();
	dDebug() << "dataUpdated for" << location;
	QStringList actionList = location.split("|");
	
	if( actionList.at(1).compare("validate") == 0 &&
	    data.value("validate").toBool() && data["validate"].type() == QVariant::String )
	{
		dDebug() << "update list of places for searchrequest:";
		updatePlaces(location, data["validate"]);
		m_pWeatherEngine->disconnectSource(sAction, this);
	}
	else if( actionList.at(1).compare("weather") == 0 )
	{
		dDebug() << sAction << "data = " << data;


		if( sAction == bbcukmet_weather_Charlestown )
			m_pWeatherEngine->connectSource( bbcukmet_weather_Berlin, this);
		else if( sAction == bbcukmet_weather_Berlin )
			dDebug() << "query data for " << bbcukmet_weather_Charlestown
			         << m_pWeatherEngine->query( bbcukmet_weather_Charlestown );
	}
	else
		dDebug() << data;
}

void
WeatherEngineTest::updatePlaces( const QString & location, const QVariant & data )
{
	Q_UNUSED(location)

	dDebug() << "searchrequest for " << location << ": " << data.toString();
	QStringList vTokens = data.toString().split("|");
	if( vTokens.at(1).compare("valid") == 0 )
	{
		dDebug() << vTokens;
		if( vTokens.at(2).compare("single") == 0 ||
		    vTokens.at(2).compare("multiple") == 0 )
		{
			int iPos = 3;
			QString sLocation, sExtra;

			//--- go through all places and extract all informations ---
			while( iPos < vTokens.count() && vTokens.at(iPos).compare("place") == 0 )
			{
				sLocation = vTokens.at(iPos+1);
				sExtra.clear();

				//--- go through all attributes that belongs to the current place ---
				iPos+=2;
				while( iPos+1 < vTokens.count() && vTokens.at(iPos).compare("place") != 0 )
				{
					if( vTokens.at(iPos).compare("extra") == 0 )
						sExtra = vTokens.at(iPos+1);
					iPos+=2;
				}

				if( !sLocation.isEmpty() )
					dDebug() << sLocation << sExtra;
			}
		}
		else
			dDebug() << vTokens;
	}
	else if( vTokens.at(1).compare("timeout") == 0 )
	{
//		c.weatherEngine->disconnectSource(location, this);
		KMessageBox::error(0, i18n("The applet was not able to contact the server, please try again later"));
//		kDebug() << "*** DANGLING SOURCES AFTER ***"  << c.weatherEngine->sources();

		dDebug() << vTokens;
		return;
	}
	else
	{
//		c.weatherEngine->disconnectSource(location, this);
		KMessageBox::error(0, i18n("The place '%1' is not valid. The data source is not able to find this place.", vTokens.at(3)), i18n("Invalid Place"));
//		kDebug() << "*** DANGLING SOURCES AFTER ***"  << c.weatherEngine->sources();
		dDebug() << vTokens;
		return;
	}
}

bool
WeatherEngineTest::startTransferJob()
{
	KUrl url( "http://ruan.accu-weather.com/widget/ruan/weather-data.asp?location=EUR|DE|GM014|DRESDEN" );
	KIO::TransferJob * pJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );

	if( pJob )
	{
		/*NOTE This seams like a bug ? Doing a workarround*/
		qRegisterMetaType<KIO::filesize_t>("KIO::filesize_t");
		qRegisterMetaType<KIO::MetaData>("KIO::MetaData");
		
		KIO::TransferJob * pJob = KIO::get( url, KIO::Reload, KIO::HideProgressInfo );
		connect(pJob, SIGNAL(data(KIO::Job*, const QByteArray&)), this, SLOT(slotData(KIO::Job *, const QByteArray &)));
		connect(pJob, SIGNAL(result(KJob *)), this, SLOT(slotResult(KJob*)));
		pJob->start();
	}
	return (pJob ? true : false);
}

void
WeatherEngineTest::slotData(KIO::Job * job, const QByteArray & data)
{
	dDebug() << data;
}

void
WeatherEngineTest::slotResult( KJob * job )
{
	dDebug() << "TransferJob has been finished...";
	job->deleteLater();
}

CityWeather
getCityInfo()
{
	CityWeather cityInfo;
	cityInfo.setCity("New York");
	cityInfo.setProvider("bbcukmet");
	return cityInfo;
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
		ki18n("Copyright (c) 2009 Ulf Kreissig") );
	aboutData.addAuthor(
		ki18n("Ulf Kreissig"),
		ki18n("Main developer"),
		"udev@gmx.net");

	KCmdLineArgs::init( argc, argv, &aboutData );
	KApplication app;

	WeatherEngineTest engine;
	if( !engine.init() )
	{
		dDebug() << "WeatherEngineTest::init failed!";
		return -1;
	}

/*	if( !engine.startTransferJob() )
	{
		dDebug() << "WeatherEngineTest::startTransferJob() failed!";
		return -1;
	}
*/

//	CityWeather cityInfo = getCityInfo();
//	dDebug() << cityInfo.city() << cityInfo.provider();	
//	return 0;
	return app.exec();
}
