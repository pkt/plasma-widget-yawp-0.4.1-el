/***************************************************************************
 *   Copyright (C) 2007-2008 by Shawn Starr <shawn.starr@rogers.com>       *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

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

//--- LOCAL ---
#include "../config.h"
#include "ion_google.h"
#include "logger/streamlogger.h"

//--- QT4 ---
#include <QHash>
#include <QMutex>
#include <QMutexLocker>

//--- KDE4 ---
#include <KDateTime>
#include <KSystemTimeZones>
#include <kdeversion.h>

const QString GoogleIon::IonName("google");
const QString GoogleIon::ActionValidate("validate");
const QString GoogleIon::ActionWeather("weather");

/*	Google does not support to search for locations.
 * 	Therefore when user searchs for a city, we will request the weather information
 * 	from Google for the user given location and return one city (the user entered) or invalid message.
 *
 *	Due to this matter we can use one function/methode to parse the xml and pass a function
 * 	pointer that is handling the specific part we want:
 * 		+ reading location only
 * 		+ reading weather information
 * 
 * 	This is the signature of the function pointer to read the specific part of the xml.
 */
typedef void (*PtfDataReader)(QXmlStreamReader & xml, void * data);


struct XmlServiceData
{
	QXmlStreamReader  xmlReader; 
	QString           sPlace;
	QString           sSource;
	
	//-- we need the following information, only for the lookup job ---
	QString           sCityTag;
	QString           sPostalTag;
};

struct XmlForecastDay
{
	QString     sDayName;
	QString     sIconName;
	QString     sCondition;
	QString     sHighTemp;
	QString     sLowTemp;
};

struct XmlWeatherData
{
	int         iTempSystem;
	int         iSpeedSystem;
	
	QString     sTimestamp;
	
	QString     sCurrentTempF;
	QString     sCurrentTempC;
	QString     sCurrentIconName;
	QString     sCurrentCondition;
	QString     sWind;
	QString     sHumidity;
	
	QList<XmlForecastDay *> vForecasts;
};

struct GoogleIon::Private
{
	QMap<QString, ConditionIcons>       vConditionList;
	QHash<KJob *, XmlServiceData *>     vJobData;
	QHash<QString, KJob *>              vActiveJobs;
	
#if KDE_IS_VERSION(4,4,80)
	QStringList                         sourcesToReset;
#endif
	
	QString createLocationString( const QString & sCityTag, const QString & sPostalTag ) const;
	
	inline QString stringConverter( const QString & sValue ) const
	{
		return (sValue.isEmpty() || sValue.compare("NA")==0)? "N/A" : sValue;
	}
	
	bool readWeatherData( QXmlStreamReader & xml, PtfDataReader ptfReader, void * data ) const;
	
	static void readLocation( QXmlStreamReader & xml, void * data );
	static void readWeather( QXmlStreamReader & xml, void * data );

	static void readForecastInformations( QXmlStreamReader & xml, XmlWeatherData & weather );
	static void readCurrentConditions( QXmlStreamReader & xml, XmlWeatherData & weather );
	static void readForecastConditions( QXmlStreamReader & xml, XmlWeatherData & weather );
	
	static QString getNodeValue( const QXmlStreamReader & xml );
};

GoogleIon::GoogleIon( QObject * parent, const QVariantList & args )
	: IonInterface(parent, args),
	  d( new Private )
{
	Q_UNUSED(args);
	
	dInfo() << "GoogleIon" << YAWP_VERSION_STRING << "compiled at" << __DATE__ << __TIME__ << "for KDE" << KDE_VERSION_STRING;

#if defined(MIN_POLL_INTERVAL)
	setMinimumPollingInterval(MIN_POLL_INTERVAL);
#endif 

	d->vConditionList["chance_of_fog"]          = Mist;
	d->vConditionList["chance_of_rain"]         = LightRain;
	d->vConditionList["chance_of_showers"]      = ChanceShowersDay;
	d->vConditionList["chance_of_snow"]         = ChanceSnowDay;
	d->vConditionList["chance_of_snow_showers"] = Mist;
	d->vConditionList["chance_of_storm"]        = Thunderstorm;
	d->vConditionList["chance_of_thunderstorm"] = ChanceThunderstormDay;
	d->vConditionList["clear"]                  = ClearDay;
	d->vConditionList["cloudy"]                 = PartlyCloudyDay;
	d->vConditionList["flurries"]               = Flurries;
	d->vConditionList["fog"]                    = Mist;
	d->vConditionList["haze"]                   = Haze;
	d->vConditionList["hot"]                    = ClearDay;
	d->vConditionList["icy"]                    = Snow;
	d->vConditionList["mist"]                   = Mist;
	d->vConditionList["mostly_cloudy"]          = Overcast;
	d->vConditionList["mostly_sunny"]           = FewCloudsDay;
	d->vConditionList["partly_cloudy"]          = PartlyCloudyDay;
	d->vConditionList["partly_sunny"]           = PartlyCloudyDay;
	d->vConditionList["rain"]                   = Rain;
	d->vConditionList["sandstorm"]              = Thunderstorm;
	d->vConditionList["snow-showers"]           = RainSnow;
	d->vConditionList["showers"]                = Showers;
	d->vConditionList["snow"]                   = Snow;
	d->vConditionList["storm"]                  = Thunderstorm;
	d->vConditionList["sunny"]                  = ClearDay;
	d->vConditionList["thunderstorm"]           = Thunderstorm;
	d->vConditionList["windy"]                  = ClearDay;
}

GoogleIon::~GoogleIon()
{
	cleanup();
	delete d;
}

void
GoogleIon::init()
{
	setInitialized(true);
}

bool
GoogleIon::updateIonSource( const QString & source )
{
	// We expect the applet to send the source in the following tokenization:
	// ionname|validate|place_name - Triggers validation of place
	// ionname|weather|place_name|extra - Triggers receiving weather of place

	QStringList vTokens = source.split('|');

	if( vTokens.size() < 3 )
	{
#if KDE_IS_VERSION(4,4,80)
		setData(source, "validate", QString("%1|malformed").arg(IonName)); 
#else
		setData(source, "validate", QString("%1|timeout").arg(IonName));
#endif
		return true;
	}

	QString sPlace( vTokens.at(2).simplified() );
	
	if( d->vActiveJobs.contains( QString("%1|%2").arg(sPlace).arg(vTokens.at(1))) )
		return true;
	else if( vTokens.at(1) == ActionValidate )
	{
		//--- Look for places to match ---
		findPlace( sPlace, source );
		return true;
	}
	else if( vTokens.at(1) == ActionWeather )
	{
		//-- Request the weather data for a specific location ---
		getWeatherData( sPlace, source );

		//---- just for debug ---
/*		QFile file("/mnt/shared/kdevelop/plasmoids/yawp.svn/google_test/google_weather_augsburg_germany.xml");
		if( file.open(QIODevice::ReadOnly|QIODevice::Text) )
		{
			struct XmlServiceData * pXmlData = new XmlServiceData;
			pXmlData->sPlace = vTokens.at(2);
			pXmlData->sSource = source;
			
			XmlWeatherData * pWeather = new XmlWeatherData;
			
			pXmlData->xmlReader.addData( file.readAll() );
			readWeatherData( pXmlData->xmlReader, *pWeather );
			updateWeatherSource( *pWeather, pXmlData->sSource, pXmlData->sPlace );
			
			delete pXmlData;
			delete pWeather;
		}
		else
			dWarning() << "could not open file.";
*/		//--- end of debug ---

		return true;
	}
	else
	{
#if KDE_IS_VERSION(4,4,80)
		setData(source, "validate", QString("%1|malformed").arg(IonName)); 
#else
		setData(source, "validate", QString("%1|timeout").arg(IonName));
#endif
	}
	return false;
}

void
GoogleIon::reset()
{
	cleanup();
  
	/**  Triggered when we get initial setup data for ions that provide a list of places */
#if KDE_IS_VERSION(4,5,2) || KDE_IS_VERSION(4,5,60)
	d->sourcesToReset = sources();
	updateAllSources();
#elif KDE_IS_VERSION(4,4,80)
	d->sourcesToReset = sources();
	updateAllSources();
	emit(resetCompleted(this, true));
#elif KDE_IS_VERSION(4,2,85) || KDE_IS_VERSION(4,3,80)
	emit(resetCompleted(this, true));
#endif
}

void
GoogleIon::cleanup()
{
	dStartFunct();
  
	QHash<KJob *, XmlServiceData *>::iterator it;
	for( it = d->vJobData.begin(); it != d->vJobData.end(); ++it )
	{
		it.key()->kill( KJob::Quietly );
		delete it.value();
	}
	d->vJobData.clear();
	d->vActiveJobs.clear();
	
	dEndFunct();
}



/*   Since google does not supports search requests, we are requesting the weather information of
 *   the place the user is looking for, and extract the city.
 *   To avoid to download the entire xml, we are parsing the data, as soon as we get them
 *   and stop the job, when we got the city-tag.
 */
void
GoogleIon::findPlace( const QString & place, const QString & source )
{
	QUrl url("http://www.google.com/ig/api");
	url.addEncodedQueryItem("weather", QUrl::toPercentEncoding(place));
	KIO::TransferJob * pJob = KIO::get( url, KIO::Reload, KIO::HideProgressInfo );
	if( pJob )
	{
		/*   we are using the name to descide wheather we have to look in d->vSearchJobs
		 *   or in d->vObservationJobs to find this certain job.
		 */
		pJob->setObjectName( ActionValidate );
		pJob->addMetaData("cookies", "none"); // Disable displaying cookies

		struct XmlServiceData * pData = new XmlServiceData;
		pData->sPlace = place;
		pData->sSource = source;

		d->vJobData.insert( pJob, pData );
		d->vActiveJobs.insert( QString("%1|%2").arg(place).arg(ActionValidate), pJob );

		connect( pJob, SIGNAL(data(KIO::Job *, const QByteArray &)), this,
			SLOT(slotDataArrived(KIO::Job *, const QByteArray &)) );
		connect( pJob, SIGNAL(result(KJob *)), this, SLOT(setup_slotJobFinished(KJob *)));
	}
}
  
void
GoogleIon::setup_slotJobFinished( KJob * job )
{
	if( !d->vJobData.contains(job) )
		return;
	dStartFunct();
	struct XmlServiceData * pXmlData = d->vJobData[job];

	if( job->error() != 0 )
	{
		setData( pXmlData->sSource, ActionValidate, QString("%1|timeout").arg(IonName) );
		disconnectSource( pXmlData->sSource, this );
		dWarning() << job->errorString();
	}
	else
	{
		//--- extract location information from weather data ---
		d->readWeatherData(pXmlData->xmlReader, &GoogleIon::Private::readLocation, pXmlData);
		
		//--- create response data ---
		if( !pXmlData->sCityTag.isEmpty() )
			setData( pXmlData->sSource, ActionValidate, QString("%1|valid|single|place|%2")
				.arg(IonName).arg(d->createLocationString(pXmlData->sCityTag, pXmlData->sPostalTag)) );
		else
			setData( pXmlData->sSource, ActionValidate, QString("%1|invalid|single|%2")
				.arg(IonName).arg(pXmlData->sPlace) );
	}
	d->vJobData.remove( job );
	d->vActiveJobs.remove( QString("%1|%2").arg(pXmlData->sPlace).arg(ActionValidate) );
	job->deleteLater();
	delete pXmlData;

	dDebug() << "Running Search/Observation Jobs: " << d->vJobData.count();
	dEndFunct();
}

void
GoogleIon::getWeatherData( const QString & place, const QString & source )
{
	QUrl url("http://www.google.com/ig/api");
	url.addEncodedQueryItem("weather", QUrl::toPercentEncoding(place));
	KIO::TransferJob * pJob = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
	if( pJob )
	{
		/*   we are using the name to descide wheather we have to look in d->vSearchJobs
		 *   or in d->vObservationJobs to find this certain job.
		 */
		pJob->setObjectName( ActionWeather );
		pJob->addMetaData("cookies", "none"); // Disable displaying cookies

		struct XmlServiceData * pXmlData = new XmlServiceData;
		pXmlData->sPlace = place;
		pXmlData->sSource = source;

		d->vJobData.insert( pJob, pXmlData );
		d->vActiveJobs.insert( QString("%1|%2").arg(place).arg(ActionWeather), pJob );

		connect( pJob, SIGNAL(data(KIO::Job *, const QByteArray &)), this,
			SLOT(slotDataArrived(KIO::Job *, const QByteArray &)) );
		connect( pJob, SIGNAL(result(KJob *)), this, SLOT(slotJobFinished(KJob *)));
	}
}

void
GoogleIon::slotDataArrived( KIO::Job * job, const QByteArray & data )
{
	if( data.isEmpty() || !d->vJobData.contains(job) )
		return;
	d->vJobData[job]->xmlReader.addData( data );
}

void
GoogleIon::slotJobFinished( KJob * job )
{
	if( !d->vJobData.contains(job) )
		return;
	dStartFunct();
	struct XmlServiceData * pXmlData = d->vJobData[job];

	if( job->error() != 0 )
	{
		dWarning() << "XML error: " << job->errorString() << pXmlData->sSource << pXmlData->sSource;
	}
	else
	{
		XmlWeatherData * pData = new XmlWeatherData;
		pData->iTempSystem = FAHRENHEIT;
		pData->iSpeedSystem = MPH;
		if( d->readWeatherData( pXmlData->xmlReader, &GoogleIon::Private::readWeather, pData ) )
			updateWeatherSource( *pData, pXmlData->sSource, pXmlData->sPlace );
		qDeleteAll(pData->vForecasts.begin(), pData->vForecasts.end());
		delete pData;
	}
	d->vJobData.remove( job );
	d->vActiveJobs.remove( QString("%1|%2").arg(pXmlData->sPlace).arg(ActionWeather) );
	job->deleteLater();
 	delete pXmlData;

	dDebug() << "Running Search/Observation Jobs: " << d->vJobData.count();
	dEndFunct();
}

bool
GoogleIon::Private::readWeatherData(QXmlStreamReader & xml, PtfDataReader ptfReader, void * data) const
{
	if( ptfReader == NULL )
	  return false;

	dStartFunct();
	
	short iState = 0;
	bool bReturn = true;
	while( !xml.atEnd() )
	{
		xml.readNext();
		
		if( xml.isStartElement() )
		{
			if( iState == 0 && xml.name().compare("xml_api_reply") == 0 )
			{
				if( xml.attributes().value("version").compare("1") == 0 )
					iState = 1;
				else
				{
					bReturn = false;
					break;	//-- invalid version !?
				}
			}
			else if( iState == 1 && xml.name().compare("weather") == 0 )
			{
				iState = 2;
			}
			else if( iState == 2 )
			{
				if( xml.name().compare("problem_cause") == 0 )
					bReturn = false;
				else
					ptfReader(xml, data);
				
				// now we are done reading the xml document
				break;
			}
		}
		else if( xml.isEndElement() && iState > 0 )
		{
			if( iState == 2 && xml.name().compare("weather") == 0 )
				iState = 1;
			else if( iState == 1 && xml.name().compare("xml_api_reply") == 0 )
				iState = 0;
		}
	}
	if(xml.hasError())
		dWarning() << xml.errorString();
	
	dEndFunct();
	return bReturn;
}

void
GoogleIon::Private::readLocation( QXmlStreamReader & xml, void * data )
{
	if( data == NULL )
		return;
	
	dStartFunct();
	
	XmlServiceData * pService = (XmlServiceData*)data;
	int iState = ( xml.name().compare("forecast_information") == 0 ) ? 1 : 0;
	
	while( !xml.atEnd() )
	{
		xml.readNext();
		
		if( xml.isStartElement() )
		{
			if( iState == 0 && xml.name().compare("forecast_information") == 0 )
				iState = 1;
			else if( iState == 1 )
			{
				if( xml.name().compare("city") == 0 )
					pService->sCityTag = getNodeValue(xml);
				else if( xml.name().compare("postal_code") == 0 )
					pService->sPostalTag = getNodeValue(xml);
			}
		}
		else if( xml.isEndElement() )
		{
			if( iState == 1 && xml.name().compare("forecast_information") == 0 )
				break;
		}
	}
	
	dEndFunct();
}

void
GoogleIon::Private::readWeather( QXmlStreamReader & xml, void * data )
{
	if( data == NULL )
		return;
	
	dStartFunct();
	
	XmlWeatherData * pWeather = (XmlWeatherData*)data;
	bool bReadNext = false;
	
	while( !xml.atEnd() )
	{
		if( bReadNext )
			xml.readNext();
		else
  			bReadNext = true;

		if( xml.isStartElement() )
		{
			if( xml.name().compare("forecast_information") == 0 )
				readForecastInformations( xml,* pWeather );
			else if( xml.name().compare("current_conditions") == 0 )
				readCurrentConditions( xml, *pWeather );
			else if( xml.name().compare("forecast_conditions") == 0 )
				readForecastConditions( xml, *pWeather );
		}
	}
	dEndFunct();
}

void
GoogleIon::updateWeatherSource( const XmlWeatherData & data, const QString & sSource, const QString & sLocation )
{
	dStartFunct() << sSource;
	removeAllData( sSource );	// clear the old values
	setData(sSource, Data());	// start the update timer

	setData(sSource, "Credit", i18n("Supported by Google Weather Service") );
	setData(sSource, "Place", sLocation);

	setData(sSource, "Temperature Unit", QString::number(data.iTempSystem) );
	setData(sSource, "Wind Speed Unit",  QString::number(data.iSpeedSystem) );

	KDateTime observationDate = KDateTime::fromString( data.sTimestamp, "%Y-%m-%d %H:%M:%S %z", KSystemTimeZones::timeZones(), true);
	if( observationDate.isValid() )
		setData(sSource, "Observation Period", observationDate.toString("%d.%m.%Y @ %H:%M %z"));
	
	if( !data.sWind.isEmpty() && data.sWind.compare("N/A") != 0 )
	{
		int iPos = data.sWind.indexOf(" ", 6 );
		setData(sSource, "Wind Direction", data.sWind.mid(6, iPos-6 ));
		iPos+=4;
		int iSpeedPos = data.sWind.indexOf(" ", iPos);
		setData(sSource, "Wind Speed", data.sWind.mid(iPos, iSpeedPos-iPos));
	}
	setData(sSource, "Current Conditions", d->stringConverter(data.sCurrentCondition));
	setData(sSource, "Condition Icon", getIconName(data.sCurrentIconName));
	setData(sSource, "Humidity", d->stringConverter(data.sHumidity));

	if( data.iTempSystem == FAHRENHEIT )
		setData(sSource, "Temperature", d->stringConverter(data.sCurrentTempF));
	else
		setData(sSource, "Temperature", d->stringConverter(data.sCurrentTempC));

	if( data.vForecasts.count() > 0 )
	{
		QList<XmlForecastDay *>::const_iterator itDay = data.vForecasts.constBegin();
		short iDayIndex(0);
		for( ; itDay != data.vForecasts.constEnd(); ++itDay, ++iDayIndex )
		{
			const XmlForecastDay * pDay = *itDay;
			setData(sSource,
				QString("Short Forecast Day %1").arg(iDayIndex),
				QString("%1|%2|%3|%4|%5|N/A")
					.arg(pDay->sDayName)
					.arg(getIconName(pDay->sIconName))
					.arg(d->stringConverter(pDay->sCondition))
					.arg(d->stringConverter(pDay->sHighTemp))
					.arg(d->stringConverter(pDay->sLowTemp)));
		}
		setData(sSource, "Total Weather Days", QString::number(data.vForecasts.count()));
	}
	dEndFunct();
}

void
GoogleIon::Private::readForecastInformations( QXmlStreamReader & xml, XmlWeatherData & data )
{
//	dStartFunct();
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isEndElement() && xml.name().compare("forecast_information") == 0 )
			break;
		else if( xml.isStartElement() )
		{
//			dDebug() << xml.name().toString();
			if( xml.name().compare("unit_system") == 0 )
			{
				QString sUnitString = getNodeValue(xml);

				if( sUnitString.compare("US") == 0 )
				{
					data.iSpeedSystem = MPH;
					data.iTempSystem  = FAHRENHEIT;
				}
				else
				{
					data.iSpeedSystem = MPH;
					data.iTempSystem = CELSIUS;
				}
			}
			else if( xml.name().compare("current_date_time") == 0 )
				data.sTimestamp = getNodeValue(xml);
		}
	}
	if( xml.error() != 0 )
		dWarning() << xml.errorString();
//	dEndFunct();
}

void
GoogleIon::Private::readCurrentConditions( QXmlStreamReader & xml, XmlWeatherData & data )
{
//	dStartFunct();
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isEndElement() && xml.name().compare("current_conditions") == 0 )
			break;
		else if( xml.isStartElement() )
		{
			if( xml.name().compare("condition") == 0 )
				data.sCurrentCondition = getNodeValue(xml);
			else if( xml.name().compare("temp_f") == 0 )
				data.sCurrentTempF = getNodeValue(xml);
			else if( xml.name().compare("temp_c") == 0 )
				data.sCurrentTempC = getNodeValue(xml);
			else if( xml.name().compare("humidity") == 0 )
			{
				QString sHumidity = getNodeValue(xml);
				int iPos = sHumidity.indexOf(" ")+1;
				if( iPos > 0 )
					data.sHumidity = sHumidity.right(sHumidity.length()-iPos);
			}
			else if( xml.name().compare("icon") == 0 )
				data.sCurrentIconName = getNodeValue(xml);
			else if( xml.name().compare("wind_condition") == 0 )
				data.sWind = getNodeValue(xml);
		}
	}
	if( xml.error() != 0 )
		dWarning() << xml.errorString();
//	dEndFunct();
}

void
GoogleIon::Private::readForecastConditions( QXmlStreamReader & xml, XmlWeatherData & data )
{
//	dStartFunct();
	XmlForecastDay * pDay = new XmlForecastDay;
	data.vForecasts.append(pDay);
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isEndElement() && xml.name().compare("forecast_conditions") == 0 )
			break;
		else if( xml.isStartElement() )
		{
			if( xml.name().compare("day_of_week") == 0 )
				pDay->sDayName = getNodeValue(xml);
			else if( xml.name().compare("low") == 0 )
				pDay->sLowTemp = getNodeValue(xml);
			else if( xml.name().compare("high") == 0 )
				pDay->sHighTemp = getNodeValue(xml);
			else if( xml.name().compare("icon") == 0 )
				pDay->sIconName = getNodeValue(xml);
			else if( xml.name().compare("condition") == 0 )
				pDay->sCondition = getNodeValue(xml);
		}
	}
	if( xml.error() != 0 )
		dWarning() << xml.errorString();
//	dEndFunct();
}

QString
GoogleIon::Private::createLocationString( const QString & sCityTag, const QString & sPostalTag ) const
{
	//--- When both strings match just return one of them ---
	if( sCityTag.compare(sPostalTag, Qt::CaseInsensitive) == 0 )
		return sCityTag;

	QStringList vTokens(sPostalTag.split(QChar(','), QString::SkipEmptyParts));
	QString sCity;
	QString sCountry;
	QString sDistrict;

	//--- INTERPRET THE POSTAL TAG ---
	//
	if( vTokens.count() == 2 )	// try to parse the format 'CITY, COUNTRY' or 'CITY, COUNTRY(DISTRICT)'
	{
		sCountry = vTokens.at(1).simplified();
		int iStart = sCountry.indexOf(QChar('('));
		if( iStart >= 0 )
		{
			int iEnd = sCountry.lastIndexOf(QChar(')'));
			if( iEnd > 0 && iStart < iEnd )
			{
				sDistrict = sCountry.mid(iStart+1, iEnd-iStart-1).simplified();
				sCountry  = sCountry.remove(iStart, iEnd-iStart+1).simplified();
			}
		}
	}
	else if( vTokens.count() >= 3 )	// interpret the string as 'CITY, COUNTRY'
	{
		sDistrict = vTokens.at(1).simplified();
		sCountry = vTokens.at(2).simplified();
	}
	
	//--- PARSE THE CITY TAG, WHEN WE HAVE THE COUNTRY ONLY ---
	//
	if( sDistrict.isEmpty() && sCountry.isEmpty())
		return sCityTag;
	else
	{
		vTokens = sCityTag.split(QChar(','), QString::SkipEmptyParts);
		if( vTokens.count() >= 2 )
		{
			sDistrict = vTokens.at(1).simplified();
			sCity = vTokens.at(0).simplified();
		}
	}
	
	if( !sDistrict.isEmpty() )
		return QString("%1, %2(%3)").arg(sCity).arg(sCountry).arg(sDistrict);
	return QString("%1, %2").arg(sCity).arg(sCountry);
}

QString
GoogleIon::getIconName( const QString & sNodeValue )
{
	int iPos = sNodeValue.lastIndexOf("/");
	if( iPos <= 0 )
		return QLatin1String("weather-none-available");
	iPos+=1;
	return getWeatherIcon( d->vConditionList, sNodeValue.mid(iPos, sNodeValue.length()-iPos-4) );
}

QString
GoogleIon::Private::getNodeValue( const QXmlStreamReader & xml )
{
	return xml.attributes().value("data").toString();
}

#if KDE_VERSION_MINOR >= 3
	 K_EXPORT_PLASMA_DATAENGINE(google, GoogleIon);
#else
	 K_EXPORT_PLASMA_ION(google, GoogleIon);
#endif
