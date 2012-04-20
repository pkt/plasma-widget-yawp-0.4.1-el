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
*   Copyright (C) 2009 by  Ulf Kreißig                                    *
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
#include "ion_wunderground.h"
#include "logger/streamlogger.h"

//--- QT4 ---
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>

//--- KDE4 ---
#include <KLocale>
#include <KDateTime>
#include <KSystemTimeZones>
#include <kdeversion.h>

const QString WundergroundIon::IonName("wunderground");
const QString WundergroundIon::ActionValidate("validate");
const QString WundergroundIon::ActionWeather("weather");
const QString WundergroundIon::GeoLookupXML("http://api.wunderground.com/auto/wui/geo/GeoLookupXML");

const QString WundergroundIon::XmlDataCurrentObservation("current_observation");
const QString WundergroundIon::XmlDataForecast("forecast");
//const QString WundergroundIon::XmlDataAlerts("alerts");

enum WeatherStationType
{
	InvalidStation = 0,
	AirportStation,
	PersonalStation
};

struct XmlLookupResult
{
	// this provider might return recursive results, so we have to put the cities for a search result
	// to a map to return them in a sorted way to the gui.
	QMap<QString, QString>  vCities;
	QHash<QString, QString> vLocationExtras; // we will bypass some information to be compatible to the location-format

	KLocale::MeasureSystem measureSystem;
	short iActiveChildJobs;	// city search might return more than one xml, that we have to parse
};

struct XmlServiceData
{
	QXmlStreamReader   xmlReader;
	QString            sLocation;
	QString            sSource;
	QString            sServiceId;
	
	KIO::TransferJob * pJob;
};

struct XmlForecastDay
{
	QString sDayName;           // equals one of the following strings: Today, Tonight, Mon, Tue, Wed, Thu, Fri, Sat, Sun
	QString sDescription;       // weather-description
	QString sIcon;
	QString sHighTemp;
	QString sLowTemp;
};

struct XmlWeatherData
{
	KLocale::MeasureSystem measureSystem;
	QString sForecastUrl;
	QString sObservationTime;   // date and time when the service did the last updated on the weather-data
	QUrl imageUrl;
	
	QString sSource;
	QString sLocation;
	
	QString sCurrentIcon;
	QString sDayCondition;
	QString sNightCondition;
	QString sHumidity;
	QString sWindDirection;
	QString sWindSpeed;
	QString sWindGust;
	
	QString sCurrentTemp;
	QString sPressure;
	QString sVisibility;
	QString sDewpoint;

	QTime sunrise;
	QTime sunset;
	
	QString sLongitude;
	QString sLatitude;
	QString sTimeZone;

	QList<XmlForecastDay *> vForecasts;
	
	/*  Weather information are splitted in different xml-files: current_observation, forecast, alerts.
	 *  We will return the results to the widget, when all jobs have been completed.
	 */
	short iActiveJobs;
};

struct ImageData
{
	QByteArray fetchedData;
	QUrl imageUrl;
	QImage image;

	/*  Every time, a weather job is ready, we check if the image job is ready as well in order to
	 *  order to update the source:
	 *  ImageJob is ready:	update source immedatly and delete the weatherData in the list
	 *                        since we do not need them anymore
	 *  otherwise:			we add the weatherData to the list vWeatherSources.
	 *                         When imagejob is ready, than we will update all locations in this list,
	 *                         delete the job and keep the source until all weather has been updated.
	 */
	bool bReady;

	/*  counter of all cities that relay on this image are not ready yet */
	int locationCounter;

	/*  List contains the data of location where the job has been already completed
	 *  and the data are just waiting for the ImageJob to update.
	 */
	QList<XmlWeatherData *> vWeatherSources;
};



struct WundergroundIon::Private
{
	QMap<QString, ConditionIcons>     vIconConditionMap;
	QHash<QString, QString>           vCountryStateMap;
	
	QHash<QString, XmlServiceData *>  vServiceJobs;       // contains all running jobs
	QHash<QString, XmlLookupResult *> vLookupResults;     // key is the location we're looking for
	QHash<QString, XmlWeatherData *>  vWeatherData;       // key is ServiceId (e.g.: airport:EDDC)
	
	//--- Image jobs ---
	// We store the same ImageData in both Hashmaps, therefore we have to make sure,
	// we delete this data just once.
	QHash<QUrl, ImageData *>          vImageData;         // Satellite url is the key to the ImageData
	QHash<KJob *, ImageData *>        vImageJobs;

#if KDE_IS_VERSION(4,4,80)
	QStringList                       sourcesToReset;
#endif
	
	void setup_readStation( const QString & sLocation,
	                        const WeatherStationType type,
	                        QXmlStreamReader & xml,
	                        XmlLookupResult * pResult );
	XmlForecastDay * parseForecastDay( QXmlStreamReader & xml,
	                                   const KLocale::MeasureSystem measureSystem,
	                                   QString & sTimeZone ) const;
	QString parseForecastTemp( QXmlStreamReader & xml,
	                           const KLocale::MeasureSystem measureSystem ) const;
	void parseTextCondition( QXmlStreamReader & xml, XmlWeatherData & data ) const;
	QTime parseTime( QXmlStreamReader & xml ) const;
	
	inline QString stringConverter( const QString & sValue ) const
	{
		return (sValue.isEmpty() || sValue.compare("NA")==0)? "N/A" : sValue;
	}
};


WundergroundIon::WundergroundIon( QObject * parent, const QVariantList & args )
	: IonInterface(parent, args),
	  d( new Private )
{
	Q_UNUSED(args);
	
	dInfo() << "WundergroundIon" << YAWP_VERSION_STRING << "compiled at" << __DATE__ << __TIME__ << "for KDE" << KDE_VERSION_STRING;
	
#if defined(MIN_POLL_INTERVAL)
	setMinimumPollingInterval(MIN_POLL_INTERVAL);
#endif 

	d->vIconConditionMap["chancerain"] = ChanceShowersDay;
	d->vIconConditionMap["chancesnow"] = ChanceSnowDay;
	d->vIconConditionMap["chancetstorms"] = ChanceThunderstormDay;
	d->vIconConditionMap["clear"] = ClearDay;
	d->vIconConditionMap["cloudy"] = Overcast;
	d->vIconConditionMap["flurries"] = Flurries;
	d->vIconConditionMap["fog"] = Mist;
	d->vIconConditionMap["hazy"] = Haze;
	d->vIconConditionMap["mostlycloudy"] = Overcast;
	d->vIconConditionMap["mostlysunny"] = FewCloudsDay;
	d->vIconConditionMap["partlycloudy"] = PartlyCloudyDay;
	d->vIconConditionMap["partlysunny"] = FewCloudsDay;
	d->vIconConditionMap["rain"] = Rain;
	d->vIconConditionMap["sleet"] = RainSnow;
	d->vIconConditionMap["snow"] = Snow;
	d->vIconConditionMap["sunny"] = ClearDay;
	d->vIconConditionMap["tstorms"] = Thunderstorm;
	d->vIconConditionMap["unknown"] = NotAvailable;

	/*  We use the country/state to get the countrycode to show the flag in yaWP.
	 *  But some locations uses country-names and/or state-names we do not know.
	 *  Only this names needs to be inserted in this map (in lower letters) to map this names in the ion.
	 */
	d->vCountryStateMap["dl"] = "Germany";
	d->vCountryStateMap["ci"] = "China";
}

WundergroundIon::~WundergroundIon()
{
	cleanup();
	delete d;
}

void
WundergroundIon::init()
{
	setInitialized(true);
}

void
WundergroundIon::reset()
{
	dStartFunct();
	
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
	dEndFunct();
}

void
WundergroundIon::cleanup()
{
	dStartFunct();
	
	dDebug() << "Delete Jobs";
	QHash<QString, XmlServiceData *>::iterator itService;
	for( itService = d->vServiceJobs.begin(); itService != d->vServiceJobs.end(); ++itService )
	{
		itService.value()->pJob->kill( KJob::Quietly );
		delete itService.value();
	}
	d->vServiceJobs.clear();

	dDebug() << "Delete SearchResults";
	QHash<QString, XmlLookupResult *>::iterator itLookup;
	for( itLookup = d->vLookupResults.begin(); itLookup != d->vLookupResults.end(); ++itLookup )
	{
		delete itLookup.value();
	}
	d->vLookupResults.clear();

	dDebug() << "Delete WeatherData";
	QHash<QString, XmlWeatherData *>::iterator itWeather;
	for( itWeather = d->vWeatherData.begin(); itWeather != d->vWeatherData.end(); ++itWeather )
	{
		XmlWeatherData * pWeather = itWeather.value();
		qDeleteAll(pWeather->vForecasts.begin(), pWeather->vForecasts.end());
		delete pWeather;
	}
	d->vWeatherData.clear();
	
	dDebug() << "Delete image data";
	QHash<QUrl, ImageData *>::iterator itImageData;
	for (itImageData = d->vImageData.begin(); itImageData != d->vImageData.end(); ++itImageData)
	{
		delete itImageData.value();
	}
	d->vImageData.clear();
	
	dDebug() << "Delete Image Jobs";
	QHash<KJob *, ImageData *>::iterator itImageJob;
	for (itImageJob = d->vImageJobs.begin(); itImageJob != d->vImageJobs.end(); ++itImageJob)
	{
		// !!! Do not delete the value of the QHashMap, because this is the same as in vImageData
		
		itImageJob.key()->kill( KJob::Quietly );
	}
	d->vImageJobs.clear();
	
	dEndFunct();
}

bool
WundergroundIon::updateIonSource( const QString & sSource )
{
	dStartFunct() << sSource;
	
	QStringList vTokens = sSource.split('|');
	
	if( vTokens.size() >= 3 && vTokens.at(1) == ActionValidate )
	{
		QString sLocation( vTokens.at(2).simplified() );
		if( d->vLookupResults.contains(sLocation) )	// we are already looking for this location, right now
			return true;
		
		// Look for places to match
		setup_findPlace( sLocation, sSource );

		dEndFunct();
		return true;
	}
	else if( vTokens.size() >= 3 && vTokens.at(1) == ActionWeather )
	{
		bool bValid(false);
		if( vTokens.size() >= 4 )
		{
			/*** Check if the requested job is already running, maybe requested from another widget ***/
			if( d->vWeatherData.contains(vTokens.at(3)) )
				bValid = true;
			else if( getWeatherData( vTokens.at(2).simplified(), vTokens.at(3), sSource ) )
				bValid = true;
		}
		if( !bValid )
			setData( sSource, ActionValidate, QString("%1|invalid|single|%2").arg(IonName).arg(vTokens.at(2)) );
		dEndFunct();
		return true;
	}
	else
	{
#if KDE_IS_VERSION(4,4,80)
		setData(sSource, "validate", QString("%1|malformed").arg(IonName)); 
#else
		setData(sSource, "validate", QString("%1|timeout").arg(IonName));
#endif

	}
	dEndFunct();
	return false;
}

void
WundergroundIon::setup_findPlace( const QString & sLocation, const QString & sSource, const QString & sPath )
{
	dStartFunct();
	
	//--- Check if we already have a job for this location that is downloading the specific path ---
	/*   Search request "China" contains the following two entries:
	 *      <location type="CITY"><name>China, Mexico</name><link>/US/TX/China.html</link></location>
	 *      <location type="CITY"><name>China, Texas</name><link>/US/TX/China.html</link></location>
	 *   in this case we would download that page twice!! Therefore we save the jobs by name.
	 *   The job-name will be created from location and path.
	 */
	QString sKey = QString("%1|%2|%3").arg(ActionValidate).arg(sLocation).arg(sPath);
	if (d->vServiceJobs.contains(sKey))
	{
		dEndFunct();
		return;
	}
	QUrl url(GeoLookupXML+(sPath.isEmpty() ? "/index.xml" : sPath),  QUrl::StrictMode);
	if( sPath.isEmpty() )
		url.addEncodedQueryItem("query", QUrl::toPercentEncoding(sLocation) );
	
	KIO::TransferJob * pJob = KIO::get( url, KIO::Reload, KIO::HideProgressInfo );
	if( pJob )
	{
		pJob->addMetaData("cookies", "none"); // Disable displaying cookies
		pJob->setObjectName(sKey);

		struct XmlServiceData * pXmlData = new XmlServiceData;
		pXmlData->sLocation = sLocation;
		pXmlData->sSource = sSource;
		pXmlData->pJob = pJob;

		d->vServiceJobs.insert( sKey, pXmlData );
		if( !d->vLookupResults.contains(sLocation) )
		{
			struct XmlLookupResult * pResult = new XmlLookupResult;
			pResult->measureSystem = KGlobal::locale()->measureSystem();
			pResult->iActiveChildJobs = 1;
			d->vLookupResults.insert(sLocation, pResult);
			dDebug() << "Adding new SearchResult:" << sLocation;
		}
		else
		{
			struct XmlLookupResult * pResult = d->vLookupResults[sLocation];
			pResult->iActiveChildJobs += 1;
			dDebug() << "Adding child to SearchResult" << sLocation << sSource << pResult->iActiveChildJobs;
		}

		connect( pJob, SIGNAL(data(KIO::Job *, const QByteArray &)), this,
			SLOT(setup_slotDataArrived(KIO::Job *, const QByteArray &)) );
		connect( pJob, SIGNAL(result(KJob *)), this, SLOT(setup_slotJobFinished(KJob *)));
	}
	dEndFunct();
}

void
WundergroundIon::setup_slotDataArrived( KIO::Job * job, const QByteArray & data )
{
	if( data.isEmpty() || !d->vServiceJobs.contains(job->objectName()) )
		return;
	QString sData(data);
	d->vServiceJobs[job->objectName()]->xmlReader.addData( sData.toLatin1() );
}

void
WundergroundIon::setup_slotJobFinished( KJob * job )
{
	dStartFunct();
	if( !d->vServiceJobs.contains(job->objectName()) )
	{
		dEndFunct();
		return;
	}
	
	struct XmlServiceData * pXmlData = d->vServiceJobs[job->objectName()];

	if( d->vLookupResults.contains(pXmlData->sLocation) )
	{
		struct XmlLookupResult * pResult = d->vLookupResults[pXmlData->sLocation];

		if( job->error() != 0 )
		{
			setData( pXmlData->sSource, ActionValidate, QString("%1|timeout").arg(IonName) );
			disconnectSource( pXmlData->sSource, this );
			dWarning() << job->errorString();
		}
		else
		{
			setup_readLookupData( pXmlData->sLocation, pXmlData->sSource, pXmlData->xmlReader, pResult );
		}
	
		pResult->iActiveChildJobs -= 1;
		if( pResult->iActiveChildJobs <= 0 )
		{
			removeAllData(pXmlData->sSource );	// clear the old values
			setData(pXmlData->sSource, Data());	// start the update timer

			dDebug() << "Return the information to the plasmoid.";
			if( pResult->vCities.count() == 0 )
			{
				setData( pXmlData->sSource, ActionValidate,
				         QString("%1|invalid|single|%2").arg(IonName).arg(pXmlData->sLocation) );
			}
			else
			{
				QString sResult( QString("%1|valid|%2")
					.arg(IonName)
					.arg( pResult->vCities.count() == 1 ? "single" : "multiple" ) );
				
				QMap<QString, QString>::const_iterator it = pResult->vCities.constBegin();
				for( ; it != pResult->vCities.constEnd(); ++it )
				{
					sResult.append( "|" + it.value() );
				}
				setData( pXmlData->sSource, ActionValidate, sResult );
				
				QHash<QString, QString>::const_iterator itExtra = pResult->vLocationExtras.constBegin();
				for( ; itExtra != pResult->vLocationExtras.constEnd(); ++itExtra )
				{
					setData( pXmlData->sSource, itExtra.key(), itExtra.value() );
				}
			}
			d->vLookupResults.remove( pXmlData->sLocation );
			delete pResult;
		}
//		else
//			dDebug() << "Active Childjobs:" << pSearchResult->iActiveChildJobs;
	}
	d->vServiceJobs.remove( job->objectName() );
	job->deleteLater();
	delete pXmlData;

//	dDebug() << "Search Jobs: " << d->vSearchJobs.count();
	dEndFunct();
}

bool
WundergroundIon::setup_readLookupData( const QString & sLocation,
                                       const QString & sSource,
                                       QXmlStreamReader & xml,
                                       XmlLookupResult * pLookupResult )
{
	dStartFunct();
	
	short iLevel(0);
	bool  bLocations(false);
	bool  bNearbyStations(false);
	WeatherStationType stationType(InvalidStation);
	
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isStartElement() )
		{
//			dTracing() << xml.name() << "   column" << xml.columnNumber() << "   level" << iLevel;
			
			if( iLevel == 0 && xml.name().compare("locations") == 0 )
			{
				bLocations = true;
			}
			else if( bLocations && iLevel == 2 && xml.name().compare("link") == 0 )
			{
				QString sPath( xml.readElementText() );
				setup_findPlace( sLocation, sSource, sPath );
			}
			
			else if( iLevel == 0 && xml.name().compare("location") == 0 )
			{
			}
			else if( !bLocations && iLevel == 1 )
			{
				if( xml.name().compare("country") == 0 )
					dInfo() << "Country:" << xml.readElementText();
				else if( xml.name().compare("state") == 0 )
					dInfo() << "State:" << xml.readElementText();
				else if( xml.name().compare("city") == 0 )
					dInfo() << "City:" << xml.readElementText();
				else if( xml.name().compare("tz_unix") == 0 )
					dInfo() << "TimeZone:" << xml.readElementText();
				else if( xml.name().compare("lat") == 0 )
					dInfo() << "Latitude:" << xml.readElementText();
				else if( xml.name().compare("lon") == 0 )
					dInfo() << "Longitude:" << xml.readElementText();
				else if( xml.name().compare("zip") == 0 )
					dInfo() << "ZipCode:" << xml.readElementText();
				else
				if( xml.name().compare("nearby_weather_stations") == 0 )
					bNearbyStations = true;
			}
			else if( bNearbyStations && xml.name().compare("airport") == 0 )
				stationType = AirportStation;
			else if( bNearbyStations && xml.name().compare("pws") == 0 )
				stationType = PersonalStation;
			else if( stationType != InvalidStation && xml.name().compare("station") == 0 )
				d->setup_readStation( sLocation, stationType, xml, pLookupResult );

			else if( iLevel == 0 )	// this is a strange xml-page - should not happen.
				return false;
			
//			dTracing() << "NearbyStation:" << bNearbyStations << "   Airport:" << bAirportStations << "   Private:" << bPrivateStations;
			
			iLevel += 1;
		}
		if( xml.isEndElement() )
		{
			if( !bLocations )
			{
				if( iLevel == 1 && xml.name().compare("nearby_weather_stations") == 0 )
					bNearbyStations = false;
				else if( bNearbyStations && (xml.name().compare("airport") == 0 || xml.name().compare("pws") == 0) )
					stationType = InvalidStation;
			}

			iLevel -= 1;
		}
	}
	dEndFunct();
	return true;
}


void
WundergroundIon::slotDataArrived( KIO::Job * job, const QByteArray & data )
{
	if( data.isEmpty() || !d->vServiceJobs.contains(job->objectName()) )
		return;
	QString sData(data);
	d->vServiceJobs[job->objectName()]->xmlReader.addData( sData.toLatin1() );
}

void
WundergroundIon::slotJobFinished( KJob * job )
{
	dStartFunct();
	if( !d->vServiceJobs.contains(job->objectName()) )
	{
		dEndFunct();
		return;
	}
	
	struct XmlServiceData * pXmlData = d->vServiceJobs[job->objectName()];
	
	if( d->vWeatherData.contains(pXmlData->sServiceId) )
	{
		XmlWeatherData * pWeatherData = d->vWeatherData[pXmlData->sServiceId];

		if (job->error() != 0)
		{
			dWarning()  << pXmlData->sSource << job->errorString();
		}
		else 
		{
			if (job->objectName().startsWith(XmlDataCurrentObservation))
				readCurrentObservation(pXmlData->xmlReader, *pWeatherData);
			else if (job->objectName().startsWith(XmlDataForecast))
				readWeatherForecast(pXmlData->xmlReader, *pWeatherData);
		}	
		pWeatherData->iActiveJobs -= 1;
		dDebug() << "active jobs for " << pXmlData->sServiceId << pWeatherData->iActiveJobs;
			
		if (pWeatherData->iActiveJobs <= 0)
		{
			d->vWeatherData.remove(pXmlData->sServiceId);

			ImageData  * pImageData = NULL;
			if( !pWeatherData->imageUrl.isEmpty() && d->vImageData.contains(pWeatherData->imageUrl) )
				pImageData = d->vImageData[pWeatherData->imageUrl];
			
			if (pImageData && !pImageData->bReady)
			{
				//  The attached image data has not been downloaded yet, so we store the weather data
				//  to the pending update sources.
				pImageData->vWeatherSources.append( pWeatherData );
			}
			else
			{
				updateWeatherSource( *pWeatherData, pImageData );
				
				d->vWeatherData.remove(pXmlData->sServiceId);
				qDeleteAll(pWeatherData->vForecasts.begin(), pWeatherData->vForecasts.end());
				delete pWeatherData;
				
				if (pImageData)
				{
					pImageData->locationCounter -= 1;
					
					if (pImageData->locationCounter <= 0)
					{
						d->vImageData.remove( pImageData->imageUrl );
						delete pImageData;
					}
				}
			}
		}
	}
	
	d->vServiceJobs.remove(job->objectName());
	delete pXmlData;
	job->deleteLater();

	dDebug() << "Service Jobs:   " << d->vServiceJobs.count();
	dDebug() << "Weather Data:   " << d->vWeatherData.count();
	dDebug() << "Image Jobs:     " << d->vImageJobs.count();
	dDebug() << "Image Data:     " << d->vImageData.count();

	dEndFunct();
}

void
WundergroundIon::slotImageDataArrived( KIO::Job * pJob, const QByteArray & data )
{
	if( data.isEmpty() || !d->vImageJobs.contains(pJob) )
		return;
	d->vImageJobs[pJob]->fetchedData.append( data );
}

void
WundergroundIon::slotImageJobFinished( KJob * pJob )
{
	if( !d->vImageJobs.contains(pJob) )
		return;
	
	dStartFunct();
	struct ImageData * pImageData = d->vImageJobs[pJob];
	pImageData->bReady = true;

	if( pJob->error() == 0 )
	{
		pImageData->image.loadFromData( pImageData->fetchedData, "PNG" );
	}
	else
	{
		dWarning() << pJob->errorString();
	}
	pImageData->fetchedData.clear();

	//--- update all pending weather sources ---
	while( pImageData->vWeatherSources.count() > 0 )
	{
		XmlWeatherData * pWeather = pImageData->vWeatherSources.takeFirst();
		updateWeatherSource( *pWeather, pImageData );
		qDeleteAll( pWeather->vForecasts.begin(), pWeather->vForecasts.end() );
		delete pWeather;
		pImageData->locationCounter -= 1;
	}

	d->vImageJobs.remove( pJob );
	pJob->deleteLater();
	
	if( pImageData->locationCounter <= 0 )
	{
		d->vImageData.remove( pImageData->imageUrl );
		delete pImageData;
	}

	dDebug() << "Service Jobs:   " << d->vServiceJobs.count();
	dDebug() << "Weather Data:   " << d->vWeatherData.count();
	dDebug() << "Image Jobs:     " << d->vImageJobs.count();
	dDebug() << "Image Data:     " << d->vImageData.count();

	dEndFunct();
}

bool
WundergroundIon::getWeatherData( const QString & sLocation, const QString & sServiceId, const QString & sSource )
{
	dStartFunct() << sSource;

	QStringList vTokens( sServiceId.split(':') );
	if( vTokens.count() != 2 )
	{
		dEndFunct();
		return false;
	}
	
	int iJobCounter(0);
	QUrl urlObservation;
	bool bAirportStation = false;
	
	if( vTokens.at(0).compare("airport") == 0 )
	{
		urlObservation.setUrl("http://api.wunderground.com/auto/wui/geo/WXCurrentObXML/index.xml", QUrl::StrictMode);
		urlObservation.addEncodedQueryItem("query", QUrl::toPercentEncoding(vTokens.at(1)));

		bAirportStation = true;
	}
	else if( vTokens.at(0).compare("pws") == 0 )
	{
		urlObservation.setUrl("http://api.wunderground.com/weatherstation/WXCurrentObXML.asp", QUrl::StrictMode);
		urlObservation.addEncodedQueryItem("ID", QUrl::toPercentEncoding(vTokens.at(1)));
	}
	dDebug() << urlObservation;

	/*** download current observation ***/
	KIO::TransferJob * pJobObservation = KIO::get( urlObservation, KIO::Reload, KIO::HideProgressInfo );
	if( pJobObservation )
	{
		iJobCounter += 1;
		pJobObservation->addMetaData("cookies", "none"); // Disable displaying cookies
		pJobObservation->setObjectName(QString("%1|%2").arg(XmlDataCurrentObservation).arg(sServiceId));
		
		struct XmlServiceData * pXmlData = new XmlServiceData;
		pXmlData->sLocation = sLocation;
		pXmlData->sServiceId = sServiceId;
		pXmlData->sSource = sSource;
		pXmlData->pJob = pJobObservation;
		
		d->vServiceJobs.insert( pJobObservation->objectName(), pXmlData );

		connect( pJobObservation, SIGNAL(data(KIO::Job *, const QByteArray &)), this,
			SLOT(slotDataArrived(KIO::Job *, const QByteArray &)) );
		connect( pJobObservation, SIGNAL(result(KJob *)), this, SLOT(slotJobFinished(KJob *)));
	}
	
	/*** download forecast information ***/
	if (bAirportStation)
	{
		QUrl urlForecast("http://api.wunderground.com/auto/wui/geo/ForecastXML/index.xml");
		urlForecast.addEncodedQueryItem("query", QUrl::toPercentEncoding(vTokens.at(1)));
		KIO::TransferJob * pJobForecast = KIO::get( urlForecast, KIO::Reload, KIO::HideProgressInfo );
		if( pJobForecast )
		{
			iJobCounter += 1;
			pJobForecast->addMetaData("cookies", "none"); // Disable displaying cookies
			pJobForecast->setObjectName(QString("%1|%2").arg(XmlDataForecast).arg(sServiceId));
			
			struct XmlServiceData * pXmlData = new XmlServiceData;
			pXmlData->sLocation = sLocation;
			pXmlData->sServiceId = sServiceId;
			pXmlData->sSource = sSource;
			pXmlData->pJob = pJobForecast;
			
			d->vServiceJobs.insert( pJobForecast->objectName(), pXmlData );

			connect( pJobForecast, SIGNAL(data(KIO::Job *, const QByteArray &)), this,
				SLOT(slotDataArrived(KIO::Job *, const QByteArray &)) );
			connect( pJobForecast, SIGNAL(result(KJob *)), this, SLOT(slotJobFinished(KJob *)));
		}
	}
	
	/*** download alerts ***/
	// Not done yet!!!
	
	if( iJobCounter >= 1 )
	{
		XmlWeatherData * pXmlWeatherData = new XmlWeatherData;
		pXmlWeatherData->sSource = sSource;
		pXmlWeatherData->sLocation = sLocation;
		pXmlWeatherData->iActiveJobs = iJobCounter;
		pXmlWeatherData->measureSystem = KGlobal::locale()->measureSystem();
		
		d->vWeatherData.insert(sServiceId, pXmlWeatherData);
		dDebug() << "Added XmlWeatherData for" << sServiceId;
	}
	dEndFunct();
	return true;
}

void
WundergroundIon::readCurrentObservation( QXmlStreamReader & xml, XmlWeatherData & data )
{
	dStartFunct();

	short iLevel(0);
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isStartElement() )
		{
			if( iLevel == 0 && xml.name().compare("current_observation") != 0 )
			{
				dError() << "Wrong leading XML-Tag in XML-WeatherData:" << xml.name();
				return;
			}
			else if( iLevel == 1 )
			{
				if( xml.name().compare("observation_time_rfc822") == 0 )
					data.sObservationTime = xml.readElementText();
				else if( xml.name().compare("forecast_url") == 0 )
					data.sForecastUrl = xml.readElementText();
				
				// we try to extract the longitude and latitude for the weather image,
				// the longitude & latitude does not match the position information
				// we receive in tag <observation_location>
				else if( xml.name().compare("ob_url") == 0 )
				{
					QString sTmp = xml.readElementText();
					dInfo() << "Latitude/Longitude" << sTmp;
					int pos = sTmp.indexOf("?query=");
					sTmp = sTmp.right(sTmp.length() - pos - 7); // we do not want the text "?query=" in text
					QStringList vPosition = sTmp.split(',', QString::SkipEmptyParts);
					dInfo() << "Latitude/Longitude for image" << vPosition;
					
					if (vPosition.length() == 2)
					{
						QUrl urlSatelliteMap("http://wublast.wunderground.com/cgi-bin/WUBLAST");
						urlSatelliteMap.addEncodedQueryItem("lat", vPosition[0].toLatin1());
						urlSatelliteMap.addEncodedQueryItem("lon", vPosition[1].toLatin1());
						urlSatelliteMap.addEncodedQueryItem("zoom", "3");
						urlSatelliteMap.addEncodedQueryItem("width", "310");
						urlSatelliteMap.addEncodedQueryItem("height", "233");
						urlSatelliteMap.addEncodedQueryItem("key", "sat_ir4_thumb");
						urlSatelliteMap.addEncodedQueryItem("basemap", "1");
						urlSatelliteMap.addEncodedQueryItem("gtt", "0");
						urlSatelliteMap.addEncodedQueryItem("num", "1");
						urlSatelliteMap.addEncodedQueryItem("timelabel", "0");
						urlSatelliteMap.addEncodedQueryItem("delay", "25");
						urlSatelliteMap.addEncodedQueryItem("borders", "1");
						urlSatelliteMap.addEncodedQueryItem("theme", "WUNIDS");
						urlSatelliteMap.addEncodedQueryItem("extension", "png");

						dInfo() << "Satellite Map" << urlSatelliteMap;
						data.imageUrl = urlSatelliteMap;
						
						connectWithImageData(urlSatelliteMap);
					}
				}
				else if( xml.name().compare("relative_humidity") == 0 )
				{
					data.sHumidity = xml.readElementText().simplified();
					if( !data.sHumidity.isEmpty() &&
					    data.sHumidity.compare("NA") != 0 &&
					    data.sHumidity.compare("N/A") != 0 &&
					    data.sHumidity[data.sHumidity.length()-1] != QChar('%') )
					{
						data.sHumidity.append('%');
					}
				}
				else if( xml.name().compare("wind_dir") == 0 )
					data.sWindDirection = xml.readElementText();
				else if( xml.name().compare("wind_mph") == 0 )
					data.sWindSpeed = xml.readElementText();
				else if( xml.name().compare("wind_gust_mph") == 0 )
					data.sWindGust = xml.readElementText();
				else if( xml.name().compare("icon") == 0 )
					data.sCurrentIcon = getWeatherIcon(d->vIconConditionMap, xml.readElementText());
				
				if( data.measureSystem == KLocale::Metric )
				{
					if( xml.name().compare("temp_c") == 0 )
						data.sCurrentTemp = xml.readElementText();
					else if( xml.name().compare("pressure_mb") == 0 )
						data.sPressure = xml.readElementText();
					else if( xml.name().compare("dewpoint_c") == 0 )
						data.sDewpoint = xml.readElementText();
					else if( xml.name().compare("visibility_km") == 0 )
						data.sVisibility = xml.readElementText();
				}
				else
				{
					if( xml.name().compare("temp_f") == 0 )
						data.sCurrentTemp = xml.readElementText();
					else if( xml.name().compare("pressure_in") == 0 )
						data.sPressure = xml.readElementText();
					else if( xml.name().compare("dewpoint_f") == 0 )
						data.sDewpoint = xml.readElementText();
					else if( xml.name().compare("visibility_mi") == 0 )
						data.sVisibility = xml.readElementText();
				}
			}
			else if( iLevel == 2 )
			{
				if( xml.name().compare("longitude") == 0 )
					data.sLongitude = xml.readElementText();
				else if( xml.name().compare("latitude") == 0 )
					data.sLatitude = xml.readElementText();
			}
			iLevel += 1;
		}
		if( xml.isEndElement() )
		{
			iLevel -= 1;
		}
	}
	dEndFunct();
}

void
WundergroundIon::connectWithImageData(const QUrl & urlSatelliteMap)
{
	dStartFunct();
	
	if (urlSatelliteMap.isEmpty())
	{
		dWarning() << "Url for satellite map is empty";
		dEndFunct();
	}
	
	ImageData * pImageData = 0;
	if (!d->vImageData.contains(urlSatelliteMap) )
	{
		KIO::TransferJob * pJob = KIO::get(urlSatelliteMap, KIO::Reload, KIO::HideProgressInfo);
		if (pJob)
		{
//			pJob->addMetaData("cookies", "none"); // Disable displaying cookies
			pImageData = new ImageData;
			pImageData->imageUrl = urlSatelliteMap;
			pImageData->bReady = false;
			pImageData->locationCounter = 1;
			
			d->vImageJobs.insert( pJob, pImageData );
			d->vImageData.insert( urlSatelliteMap, pImageData );

			connect( pJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
			         this, SLOT(slotImageDataArrived(KIO::Job *, const QByteArray &)) );
			connect(  pJob, SIGNAL(result(KJob *)), this, SLOT(slotImageJobFinished(KJob *)) );
		}
	}
	else
	{
		pImageData = d->vImageData[urlSatelliteMap];
		pImageData->locationCounter += 1;
	}
	dDebug() << "Image Jobs: " << d->vImageJobs.count();
	dDebug() << "Image Data: " << d->vImageData.count();
	
	dEndFunct();
}

void
WundergroundIon::readWeatherForecast( QXmlStreamReader & xml, XmlWeatherData & data )
{
	dStartFunct();
	
	short iLevel(0);
	bool bCurrentCondition(false);
	bool bForecast(false);
	bool bMoonPhase(false);
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isStartElement() )
		{
			if( iLevel == 0 && xml.name().compare("forecast") != 0 )
			{
				dError() << "Wrong leading XML-Tag in XML-WeatherData:" << xml.name();
				dEndFunct();
				return;
			}
			else if( iLevel == 1 )
			{
				if( xml.name().compare("txt_forecast") == 0 )
					bCurrentCondition = true;
				else if( xml.name().compare("simpleforecast") == 0 )
					bForecast = true;
				else if( xml.name().compare("moon_phase") == 0 )
					bMoonPhase = true;
			}
			else if( bCurrentCondition && xml.name().compare("forecastday") == 0 )
				d->parseTextCondition(xml, data);
			else if( bForecast && xml.name().compare("forecastday") == 0 )
			{
				XmlForecastDay * pForecast = d->parseForecastDay(xml, data.measureSystem, data.sTimeZone);
				if( pForecast )
				{
					pForecast->sIcon = getWeatherIcon(d->vIconConditionMap, pForecast->sIcon);
					data.vForecasts.append(pForecast);
				}
			}
			else if( bMoonPhase )
			{
				if( xml.name().compare("sunrise") == 0 )
					data.sunrise = d->parseTime(xml);
				else if( xml.name().compare("sunset") == 0 )
					data.sunset = d->parseTime(xml);
			}
			iLevel += 1;
		}
		if( xml.isEndElement() )
		{
			iLevel -= 1;
			if( bCurrentCondition && xml.name().compare("txt_forecast") == 0 )
				bCurrentCondition = false;
			if( bForecast && xml.name().compare("simpleforecast") == 0 )
				bForecast = false;
			else if( bMoonPhase && xml.name().compare("moon_phase") == 0 )
				bMoonPhase = false;
		}
	}
	dEndFunct();
}

void
WundergroundIon::updateWeatherSource( const XmlWeatherData & data, const ImageData * pImageData )
{
	dStartFunct()	<< data.sSource;
	removeAllData( data.sSource );	// clear the old values
	setData(data.sSource, Data());	// start the update timer

	setData(data.sSource, "Credit", i18n("Supported by Weather Underground NOAA Weather Station"));
	
	if (!data.sForecastUrl.isEmpty())
		setData(data.sSource, "Credit Url", data.sForecastUrl);
	
	if( pImageData && !pImageData->image.isNull() )
		setData(data.sSource, "Satellite Map", pImageData->image);

	
	setData(data.sSource, "Place", data.sLocation);

	if( !data.sLongitude.isEmpty() && !data.sLatitude.isEmpty() )
	{
		setData(data.sSource, "Longitude", data.sLongitude);
		setData(data.sSource, "Latitude", data.sLatitude);
	}

	KDateTime obsDate;

	/*** Convert the obervation time to locale time and add the formated time string to data-container */
	if( !data.sObservationTime.isEmpty() &&
	    data.sObservationTime.compare("NA") !=0 && data.sObservationTime.compare("N/A") !=0 )
	{
		obsDate = KDateTime::fromString(data.sObservationTime, "%A, %e %B %Y %H:%M:%S %Z", KSystemTimeZones::timeZones(), true);
		if( obsDate.isValid() && !data.sTimeZone.isEmpty() )
		{
			obsDate = obsDate.toZone( KSystemTimeZones::zone(data.sTimeZone) );
			setData(data.sSource, "Observation Period", obsDate.toString("%d.%m.%Y @ %k:%M"));
		}
	}

	int iTempSystem(0), iSpeedSystem(0), iDistanceSystem(0), iPressureSystem(0);
#if KDE_IS_VERSION(4,3,70)
	if( data.measureSystem == KLocale::Metric )
	{
		iTempSystem = KUnitConversion::Celsius;
		iDistanceSystem = KUnitConversion::Kilometer;
		iPressureSystem = KUnitConversion::Millibar;
	}
	else
	{
		iTempSystem = KUnitConversion::Fahrenheit;
		iDistanceSystem = KUnitConversion::Mile;
		iPressureSystem = KUnitConversion::InchesOfMercury;
	}
	iSpeedSystem = KUnitConversion::MilePerHour;
#elif KDE_VERSION_MINOR <= 3
	if( data.measureSystem == KLocale::Metric )
	{
		iTempSystem = WeatherUtils::Celsius;
		iDistanceSystem = WeatherUtils::Kilometers;
		iPressureSystem = WeatherUtils::Millibars;
	}
	else
	{
		iTempSystem = WeatherUtils::Fahrenheit;
		iDistanceSystem = WeatherUtils::Miles;
		iPressureSystem = WeatherUtils::InchesHG;
	}
	#if KDE_VERSION_MINOR == 3
		iSpeedSystem = WeatherUtils::MilesPerHour;
	#elif KDE_VERSION_MINOR == 2
		iSpeedSystem = WeatherUtils::MilesAnHour;
	#endif
#endif

	setData(data.sSource, "Dewpoint Unit", QString::number(iTempSystem) );
	setData(data.sSource, "Temperature Unit", QString::number(iTempSystem) );
	setData(data.sSource, "Wind Gust Unit", QString::number(iSpeedSystem) );
	setData(data.sSource, "Visibility Unit", QString::number(iDistanceSystem) );
	setData(data.sSource, "Wind Speed Unit",  QString::number(iSpeedSystem) );

	setData(data.sSource, "Condition Icon", getWeatherIcon(d->vIconConditionMap, d->stringConverter(data.sCurrentIcon) ) );
	setData(data.sSource, "Current Conditions", d->stringConverter(data.sDayCondition) );

	setData(data.sSource, "Dewpoint", d->stringConverter(data.sDewpoint) );
	setData(data.sSource, "Humidity", d->stringConverter(data.sHumidity) );
	setData(data.sSource, "Pressure", d->stringConverter(data.sPressure) );
	setData(data.sSource, "Temperature", d->stringConverter(data.sCurrentTemp) );
	setData(data.sSource, "Visibility", d->stringConverter(data.sVisibility) );
	setData(data.sSource, "Wind Direction", d->stringConverter(data.sWindDirection) );
	setData(data.sSource, "Wind Speed", d->stringConverter(data.sWindSpeed) );
	setData(data.sSource, "Wind Gust", d->stringConverter(data.sWindGust) );
	
	if(data.sPressure.isEmpty() || data.sPressure.compare("NA")!=0)
		setData(data.sSource, "Pressure Unit", QString::number(iPressureSystem) );
	
	if( data.sunrise.isValid() && data.sunset.isValid() )
	{
#if KDE_VERSION_MINOR <= 2
		setData(data.sSource, "Sunrise At", data.sunrise.toString("h:m") );
		setData(data.sSource, "Sunset At",  data.sunset.toString("h:m") );
#else
		if( obsDate.isValid() )
		{
			// Format of sunrise and sunset: "Tuesday November 29, 2011 at 07:43"
			QDateTime dateTime(obsDate.dateTime().date(), data.sunrise);
			setData(data.sSource, "Sunrise At", QString("%1, %2 at %3")
				.arg(dateTime.toString("dddd MMMM dd"))
				.arg(dateTime.date().year())
				.arg(dateTime.toString("HH:mm")));
			
			dateTime.setTime(data.sunset);
			setData(data.sSource, "Sunset At", QString("%1, %2 at %3")
				.arg(dateTime.toString("dddd MMMM dd"))
				.arg(dateTime.date().year())
				.arg(dateTime.toString("HH:mm")));
		}
#endif
	}
	if( data.vForecasts.count() > 0 )
	{
		setData(data.sSource, "Total Weather Days", QString::number(data.vForecasts.count()) );
		
		QList<XmlForecastDay *>::const_iterator itDay = data.vForecasts.constBegin();
		short iDay(0);
		for( ; itDay != data.vForecasts.constEnd(); ++itDay, ++iDay )
		{
			const QString * psCurrCondition = NULL;
			if( iDay == 0 && obsDate.isValid() && data.sunrise.isValid() && data.sunset.isValid() )
			{
				QTime obsTime( obsDate.time() );
				if( data.sunrise < obsTime && obsTime < data.sunset )
					psCurrCondition = (data.sDayCondition.isEmpty() ? NULL : &data.sDayCondition);
				else
					psCurrCondition = (data.sNightCondition.isEmpty() ? NULL : &data.sNightCondition);
			}
			setData( data.sSource,
					 QString("Short Forecast Day %1").arg(iDay),
					QString("%1|%2|%3|%4|%5|N/A")
						.arg( (*itDay)->sDayName )
						.arg( d->stringConverter((*itDay)->sIcon) )
						.arg( d->stringConverter( psCurrCondition ? *psCurrCondition : (*itDay)->sDescription) )
						.arg( d->stringConverter((*itDay)->sHighTemp) )
						.arg( d->stringConverter((*itDay)->sLowTemp) ) );
		}
	}
	dEndFunct();
}


void
WundergroundIon::Private::setup_readStation( const QString & sLocation,
                                             const WeatherStationType stationType,
                                             QXmlStreamReader & xml,
                                             XmlLookupResult * pLookupResult )

{
	QString sCity;
	QString sNeighborhood;
	QString sState;
	QString sCountry;
	QString sId;
	QString sDistance;
	KLocale::MeasureSystem system (pLookupResult ? pLookupResult->measureSystem : KLocale::Imperial);
	
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isStartElement() )
		{
			if( xml.name().compare("city") == 0 )
				sCity = QUrl::fromPercentEncoding(xml.readElementText().toUtf8()).simplified();
			else if( xml.name().compare("neighborhood") == 0 )
				sNeighborhood = QUrl::fromPercentEncoding(xml.readElementText().toUtf8()).simplified();
			else if( xml.name().compare("state") == 0 )
			{
				sState = QUrl::fromPercentEncoding(xml.readElementText().toUtf8());
				if( vCountryStateMap.contains(sState.toLower()) )
					sState = vCountryStateMap.value( sState.toLower() );
			}
			else if( xml.name().compare("country") == 0 )
			{
				sCountry = QUrl::fromPercentEncoding(xml.readElementText().toUtf8());
				if( vCountryStateMap.contains(sCountry.toLower()) )
					sCountry = vCountryStateMap.value( sCountry.toLower() );
			}
			else if( stationType == AirportStation && xml.name().compare("icao") == 0 )
				sId = xml.readElementText();
			else if( stationType == PersonalStation && xml.name().compare("id") == 0 )
				sId = xml.readElementText();
			else if( xml.name().compare("distance_km") == 0 && system == KLocale::Metric )
				sDistance = xml.readElementText() + " km";
			else if( xml.name().compare("distance_mi") == 0 && system == KLocale::Imperial )
				sDistance = xml.readElementText() + " mi";
		}
		if( xml.isEndElement() )
		{
			if( xml.name().compare("station") == 0 )
				break;
		}
	}
	dDebug() << (stationType == AirportStation ? "Airport" : "PWS") << sCity << sNeighborhood << sState << sCountry << "ID =" << sId;

	if (sCity.isEmpty() ||
	    sId.isEmpty() ||
	    sId.compare("----") == 0) // station ID is invalid, weather provider will not return any values for station ID "----"
		return;

	if (!sNeighborhood.isEmpty())
		sCity.append(" - " + sNeighborhood);
	
	QString sKey;
	QString sServiceId;
	QString sLocationExtras;
	
	if (stationType == AirportStation)
		sKey = "Airport";
	else
		sKey = "PWS";
	
	if( sState.isEmpty() )
		sKey += QString("%1|%2").arg(sCity).arg(sCountry);
	else
		sKey += QString("%1|%2|%3").arg(sCity).arg(sState).arg(sCountry);

	if( stationType == AirportStation )
	{
		sLocationExtras += "stationtype|Airport";
		sServiceId = QString("airport:%1").arg(sId);
	}
	else
	{
		sLocationExtras += "stationtype|PWS";
		sServiceId = QString("pws:%1").arg(sId);
	}
	if( !sDistance.isEmpty() )
	{
		if (!sLocationExtras.isEmpty())
			sLocationExtras.append("|");
		
		sLocationExtras += "distance|"+sDistance;
	}
	if( sCountry.compare("US") == 0 )
	{
		sCountry = sState;
		sState.clear();
	}
	QString sLocationName = QString("%1, %2").arg(sCity).arg(sCountry);
	if( !sState.isEmpty() )
		sLocationName.append( QString("(%1)").arg(sState) );
	QString sData( QString("place|%1|extra|%2").arg(sLocationName).arg(sServiceId) );

//	dDebug() << sKey << sServiceId << sData << sLocationExtras;
	if( pLookupResult &&
	    sLocationExtras.length() > 0 &&
	    !pLookupResult->vLocationExtras.contains(sServiceId) )
	{
		pLookupResult->vCities.insert(sKey, sData);
		pLookupResult->vLocationExtras.insert(sServiceId, sLocationExtras);
	}
	else
		dWarning() << "StationID already exists:" << sData << "with" << sLocation;

}

XmlForecastDay *
WundergroundIon::Private::parseForecastDay( QXmlStreamReader & xml,
                                            const KLocale::MeasureSystem system,
                                            QString & sTimeZone ) const
{
	XmlForecastDay * pDay = new XmlForecastDay;
	
	short iLevel = 1;	// we have parsed the start tag already, and decided to call this function!!!
//	short iPeriod = 0;
	
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isStartElement() )
		{
//			if( xml.name().compare("period") == 0 )
//				iPeriod = xml.readElementText().toInt();
			if( xml.name().compare("weekday") == 0 )
				pDay->sDayName = i18n(xml.readElementText().left(3).toUtf8().constData());
			else if( xml.name().compare("tz_long") == 0 )
				sTimeZone = xml.readElementText();
			else if( xml.name().compare("conditions") == 0 )
				pDay->sDescription = QUrl::fromPercentEncoding(xml.readElementText().toUtf8());
			else if( xml.name().compare("icon") == 0 )
				pDay->sIcon = xml.readElementText();
			else if( xml.name().compare("high") == 0 )
				pDay->sHighTemp = parseForecastTemp(xml, system);
			else if( xml.name().compare("low") == 0 )
				pDay->sLowTemp = parseForecastTemp(xml, system);
			iLevel += 1;
		}
		if( xml.isEndElement() )
		{
			iLevel -= 1;
			if( xml.name().compare("forecastday") == 0 )
				break;
		}
	}
	return pDay;
}

QString
WundergroundIon::Private::parseForecastTemp( QXmlStreamReader & xml, const KLocale::MeasureSystem system ) const
{
	short iLevel(1);
	QString sTemp;
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isStartElement() )
		{
			if( system == KLocale::Metric && xml.name().compare("celsius") == 0 )
				sTemp = xml.readElementText();
			else if( system == KLocale::Imperial && xml.name().compare("fahrenheit") == 0 )
				sTemp = xml.readElementText();
			
			iLevel += 1;
		}
		if( xml.isEndElement() )
		{
			iLevel -= 1;
			if( iLevel <= 0 )
				break;
		}
	}
	return sTemp;
}

void
WundergroundIon::Private::parseTextCondition( QXmlStreamReader & xml, XmlWeatherData & data ) const
{
	short   iLevel(1);
	short   iPeriode(-1);
	QString sText;
	QString sIcon;
	QDate   date;
	
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isStartElement() )
		{
			if( xml.name().compare("period") == 0 )
				iPeriode = xml.readElementText().toShort();
			else if( xml.name().compare("fcttext") == 0 )
				sText = xml.readElementText();
			else if(xml.name().compare("icon") == 0 )
				sIcon = xml.readElementText();
			else if( xml.name().compare("title") == 0 )
				date = QDate::fromString(xml.readElementText(), "MMMM d, yyyy");
			iLevel += 1;
		}
		if( xml.isEndElement() )
		{
			iLevel -= 1;
			if( xml.name().compare("forecastday") == 0 )
				break;
		}
	}
	
	dDebug() << "Forecast Condition Period: " << iPeriode << sIcon << sText;
	if( !sText.isEmpty() &&
	    date.isValid() &&
	    iPeriode == 0 )
	{
		data.sDayCondition = sText;
		data.sCurrentIcon = sIcon;
	}
}

QTime
WundergroundIon::Private::parseTime( QXmlStreamReader & xml ) const
{
	short iLevel(1);	// we have parsed the start tag already, and decided to call this function!!!
	short iHour(-1);
	short iMinute(-1);
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isStartElement() )
		{
			if( xml.name().compare("hour") == 0 )
				iHour = xml.readElementText().toShort();
			else if( xml.name().compare("minute") == 0 )
				iMinute = xml.readElementText().toShort();
			
			iLevel += 1;
		}
		if( xml.isEndElement() )
		{
			iLevel -= 1;
			if( iLevel <= 0 )
				break;
		}
	}
	if( iHour >= 0 && iMinute >= 0 )
		return QTime(iHour, iMinute, 0);
	return QTime();
}

#if KDE_VERSION_MINOR >= 3
	 K_EXPORT_PLASMA_DATAENGINE(wunderground, WundergroundIon);
#else
	 K_EXPORT_PLASMA_ION(wunderground, WundergroundIon);
#endif
