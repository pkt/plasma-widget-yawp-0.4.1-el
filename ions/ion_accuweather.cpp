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
#include "ion_accuweather.h"
#include "logger/streamlogger.h"

//--- QT4 ---
#include <QDate>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>

//--- KDE4 ---
#include <Plasma/DataContainer>
#include <KDateTime>
#include <kdeversion.h>

#define TAG_TEMP		1
#define	TAG_SPEED		2

const QString AccuWeatherIon::IonName("accuweather");
const QString AccuWeatherIon::ActionValidate("validate");
const QString AccuWeatherIon::ActionWeather("weather");
static const char vWeekdays [7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};


struct AccuWeatherIon::Private
{
	QMap<QString, ConditionIcons>     vConditionList;

	QHash<QString, KJob *>            vActiveJobs;
	
	//--- Search jobs ---
	QHash<KJob *, XmlJobData *>       vSearchJobs;

	//--- Observations jobs ---
	QHash<KJob *, XmlJobData *>       vObservationJobs;

	//--- Image jobs ---
	QHash<QByteArray, ImageData *>    vImageData;	// Satellite url is the key to the ImageData
	QHash<KJob *,  ImageData *>       vImageJobs;
	
#if KDE_IS_VERSION(4,4,80)
	QStringList                       sourcesToReset;
#endif
};

AccuWeatherIon::AccuWeatherIon( QObject * parent, const QVariantList & args )
	: IonInterface(parent, args),
	  d( new Private )
{
	Q_UNUSED(args)
	dInfo() << "AccuWeatherIon" << YAWP_VERSION_STRING << "compiled at" << __DATE__ << __TIME__ << "for KDE" << KDE_VERSION_STRING;

#if defined(MIN_POLL_INTERVAL)
	setMinimumPollingInterval(MIN_POLL_INTERVAL);
#endif 

	//--- DAY ICONS ---
	d->vConditionList["01"] = ClearDay;
	d->vConditionList["02"] = FewCloudsDay;
	d->vConditionList["03"] = PartlyCloudyDay;
	d->vConditionList["04"] = PartlyCloudyDay;
	d->vConditionList["05"] = Haze;
	d->vConditionList["06"] = FewCloudsDay;
	d->vConditionList["07"] = Overcast;
	d->vConditionList["08"] = Overcast;
	d->vConditionList["11"] = Mist;
	d->vConditionList["12"] = Showers;
	d->vConditionList["13"] = ChanceShowersDay;
	d->vConditionList["14"] = ChanceShowersDay;
	d->vConditionList["15"] = Thunderstorm;
	d->vConditionList["16"] = ChanceThunderstormDay;
	d->vConditionList["17"] = ChanceThunderstormDay;
	d->vConditionList["18"] = Rain;
	d->vConditionList["19"] = ChanceSnowDay;
	d->vConditionList["20"] = ChanceSnowDay;
	d->vConditionList["21"] = ChanceSnowDay;
	d->vConditionList["22"] = Snow;
	d->vConditionList["23"] = Snow;
	d->vConditionList["24"] = Snow;
	d->vConditionList["25"] = Rain;
	d->vConditionList["26"] = Rain;
	d->vConditionList["29"] = FreezingRain;
	d->vConditionList["30"] = ClearDay;
	d->vConditionList["31"] = Snow;
	d->vConditionList["32"] = Flurries;

	//--- NIGHT ICONS ---
	d->vConditionList["33"] = ClearNight;
	d->vConditionList["34"] = ClearNight;
	d->vConditionList["35"] = ClearNight;
	d->vConditionList["36"] = ClearNight;
	d->vConditionList["37"] = Haze;
	d->vConditionList["38"] = PartlyCloudyNight;
	d->vConditionList["39"] = ChanceShowersNight;
	d->vConditionList["40"] = ChanceShowersNight;
	d->vConditionList["41"] = ChanceThunderstormNight;
	d->vConditionList["42"] = ChanceThunderstormNight;
	d->vConditionList["43"] = ChanceSnowNight;
	d->vConditionList["44"] = ChanceSnowNight;
}

AccuWeatherIon::~AccuWeatherIon()
{
	dStartFunct();
	
	cleanup();
	delete d;
	
	dEndFunct();
}

void
AccuWeatherIon::init()
{
	setInitialized(true);
}

bool
AccuWeatherIon::updateIonSource( const QString & source )
{
	// We expect the applet to send the source in the following tokenization:
	// ionname|validate|place_name - Triggers validation of place
	// ionname|weather|place_name|extra - Triggers receiving weather of place
	dStartFunct();
	
	QStringList vTokens = source.split('|');

	if( vTokens.count() >= 3 && vTokens.at(1) == ActionValidate )	// Look for places to match
	{
		QString sPlace( vTokens.at(2).simplified() );
		if( !d->vActiveJobs.contains( QString("%1|%2").arg(sPlace).arg(ActionValidate) ) )
			findPlace( sPlace, source );
		dEndFunct();
		return true;
	}
	else if( vTokens.count() >= 3 && vTokens.at(1) == ActionWeather )	// Weather data request for a specific locaion
	{
		if( vTokens.size() >= 4 )
		{
			dDebug() << vTokens.at(3);
			QString sPlace( vTokens.at(2).simplified() );
			QString sLocationCode( vTokens.at(3).simplified().replace('.', '|') );
			if( !d->vActiveJobs.contains( QString("%1|%2").arg(sLocationCode).arg(ActionWeather) ) )
				getWeatherXmlData( sPlace, sLocationCode, source );
			
/*			//---- just for debug ---
			QFile file("/tmp/ion_accuweather_Milwaukee, NC.txt");
			if( file.open(QIODevice::ReadOnly|QIODevice::Text) )
			{
				struct XmlJobData * pXmlData = new XmlJobData;
				pXmlData->sLocation = vTokens.at(2);
				pXmlData->sSource = source;
				
				WeatherData * pWeather = new WeatherData;
				pWeather->city = pXmlData->sLocation;
				pWeather->source = pXmlData->sSource;
				
				pXmlData->xmlReader.addData( file.readAll() );
				readWeatherXmlData( pXmlData->xmlReader, *pWeather );
				updateWeatherSource( *pWeather, NULL );
				
				delete pXmlData;
				delete pWeather;
			}
			else
				dWarning() << "could not open file.";
			//--- end of debug ---
*/
		}
		else
			setData( source, ActionValidate, QString("%1|invalid|single|%2").arg(IonName).arg(vTokens.at(2).simplified()) );

		dEndFunct();
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
	dEndFunct();
	return false;
}

void
AccuWeatherIon::reset()
{
	dStartFunct();

	cleanup();
	
	/**  Triggered when we get initial setup data for ions that provide a list of places
	*/
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
AccuWeatherIon::cleanup()
{
	dStartFunct();
	
	QHash<KJob *, XmlJobData *>::iterator it;
	for( it = d->vSearchJobs.begin(); it != d->vSearchJobs.end(); ++it )
	{
		it.key()->kill( KJob::Quietly );
		delete it.value();
	}
	d->vSearchJobs.clear();

	for( it = d->vObservationJobs.begin(); it != d->vObservationJobs.end(); ++it )
	{
		it.key()->kill( KJob::Quietly );
		delete it.value();
	}
	d->vObservationJobs.clear();

	QHash<KJob *, ImageData *>::iterator itImg;
	for( itImg = d->vImageJobs.begin(); itImg != d->vImageJobs.end(); ++itImg )
	{
		itImg.key()->kill( KJob::Quietly );
		ImageData * pImageData = itImg.value();

		/* Delete all pending WeatherDatas as well. */
		QList<WeatherData*>::iterator itWeather = pImageData->vWeatherSources.begin();
		for( ; itWeather != pImageData->vWeatherSources.end(); ++itWeather )
		{
			WeatherData * pWeather = *itWeather;
			qDeleteAll( pWeather->vForecasts.begin(), pWeather->vForecasts.end() );
			delete pWeather;
		}
		delete pImageData;
	}
	/* contains the same values than vImageJobs, but the key differs so that we will find it by url. */
	d->vImageData.clear();
	d->vImageJobs.clear();
	d->vActiveJobs.clear();
	
	dEndFunct();
}



/*********************************************************************************\
*                         S E A R C H   L O C A T I O N S                         *
\*********************************************************************************/

// Parses city list and gets the correct city based on ID number
void
AccuWeatherIon::findPlace( const QString & location, const QString & source )
{
	dStartFunct();
	QUrl url("http://ruan.accu-weather.com/widget/ruan/city-find.asp",  QUrl::StrictMode);
	url.addEncodedQueryItem("location", QUrl::toPercentEncoding(location) );

	KIO::TransferJob * pJob = KIO::get( url, KIO::Reload, KIO::HideProgressInfo );
	if( pJob )
	{
		/*   we are using the name to descide wheather we have to look in d->vSearchJobs
		 *   or in d->vObservationJobs to find this certain job.
		 */
		pJob->setObjectName( ActionValidate );
//		pJob->addMetaData("cookies", "none"); // Disable displaying cookies

		struct XmlJobData * pXmlData = new XmlJobData;
		pXmlData->sLocation = location;
		pXmlData->sSource = source;

		d->vSearchJobs.insert( pJob, pXmlData );
		d->vActiveJobs.insert( QString("%1|%2").arg(location).arg(ActionValidate), pJob );

		connect( pJob, SIGNAL(data(KIO::Job *, const QByteArray &)), this,
			SLOT(setup_slotDataArrived(KIO::Job *, const QByteArray &)) );
		connect( pJob, SIGNAL(result(KJob *)), this, SLOT(setup_slotJobFinished(KJob *)));
	}
	dEndFunct();
}

void
AccuWeatherIon::setup_slotDataArrived( KIO::Job * job, const QByteArray & data )
{
	if( data.isEmpty() || !d->vSearchJobs.contains(job) )
		return;
//	dStartFunct();
	d->vSearchJobs[job]->xmlReader.addData(data);
//	dEndFunct();
}

void
AccuWeatherIon::setup_slotJobFinished( KJob * job )
{
	if( !d->vSearchJobs.contains(job) )
		return;
	dStartFunct();
	struct XmlJobData * pXmlData = d->vSearchJobs[job];

	if( job->error() != 0 )
	{
		setData( pXmlData->sSource, ActionValidate, QString("%1|timeout").arg(IonName) );
		disconnectSource( pXmlData->sSource, this );
		dWarning() << job->errorString();
	}
	else
	{
		readSearchXmlData( pXmlData->sLocation, pXmlData->sSource, pXmlData->xmlReader );
	}
	d->vSearchJobs.remove( job );
	d->vActiveJobs.remove( QString("%1|%2").arg(pXmlData->sLocation).arg(ActionValidate) );
	job->deleteLater();
	delete pXmlData;

//	dDebug() << "Search Jobs: " << d->vSearchJobs.count();
	dEndFunct();
}

bool
AccuWeatherIon::readSearchXmlData( const QString & location, const QString & source, QXmlStreamReader & xml )
{
	dStartFunct();
	int iState = 0;
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isStartElement() )
		{
			iState += 1;
			if( iState == 2 && xml.name().compare("citylist") == 0 )
				parseSearchLocations( location, source, xml );
		}
		else if( xml.isEndElement() )
			iState -= 1;
	}
	if( xml.hasError() )
		dWarning() << xml.errorString();
	dEndFunct();
	return !xml.hasError();
}

/**   This functions extracts the location from the accuweather-location-request-xml.
 */
void
AccuWeatherIon::parseSearchLocations( const QString & location, const QString & source, QXmlStreamReader & xml )
{
	dStartFunct();

	uint iCounter = 0;
	QString sLocations;
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isEndElement() && xml.name() == "citylist" )
		{
			if( iCounter == 0 )
				setData( source, ActionValidate, QString("%1|invalid|single|%2").arg(IonName).arg(location) );
			else if( iCounter == 1 )
				setData( source, ActionValidate, QString("%1|valid|single|%2").arg(IonName).arg(sLocations) );
			else
				setData( source, ActionValidate, QString("%1|valid|multiple|%2").arg(IonName).arg(sLocations) );
			break;
		}
		else if( xml.isStartElement() && xml.name() == "location" )
		{
			QXmlStreamAttributes attributes = xml.attributes();
			QString sCity( QUrl::fromPercentEncoding(attributes.value("city").toString().toUtf8()) );
			QString sState( QUrl::fromPercentEncoding(attributes.value("state").toString().toUtf8()) );
			QString sLocation( QUrl::fromPercentEncoding(attributes.value("location").toString().toUtf8()) );

			if( iCounter > 0 )
				sLocations += "|";
			sLocations += QString("place|%1, %2|extra|%3").arg(sCity).arg(sState).arg(sLocation.replace('|','.'));
			iCounter += 1;
		}
	}
	dEndFunct();
}


/*********************************************************************************\
*                 G E T   W E A T H E R   I N F O R M A T I O N S                 *
\*********************************************************************************/

//--- extract all weather values from the xml ---
void
AccuWeatherIon::getWeatherXmlData( const QString & location, const QString & locationCode, const QString & source )
{
	dStartFunct() << locationCode;
	QUrl urlWeather("http://ruan.accu-weather.com/widget/ruan/weather-data.asp", QUrl::StrictMode);
	urlWeather.addEncodedQueryItem("location", QUrl::toPercentEncoding(locationCode.toUtf8(), "+"));

	dDebug() << urlWeather;

	KIO::TransferJob * pJob = KIO::get( urlWeather, KIO::Reload, KIO::HideProgressInfo );
	if( pJob )
	{
		/*   we are using the name to find the corresponding weatherdata for this job
		 */
		pJob->setObjectName( locationCode );
//		pJob->addMetaData("cookies", "none"); // Disable displaying cookies

		struct XmlJobData * pXmlData = new XmlJobData;
		pXmlData->sSource = source;
		pXmlData->sLocationCode = locationCode;
		pXmlData->sLocation = location;
		pXmlData->imageUrl = getImageUrl( locationCode );

		d->vObservationJobs.insert( pJob, pXmlData);
		d->vActiveJobs.insert( QString("%1|%2").arg(locationCode).arg(ActionWeather), pJob );
		
		if( !pXmlData->imageUrl.isEmpty() )
			connectWithImageData( pXmlData->imageUrl );

		connect( pJob, SIGNAL(data(KIO::Job *, const QByteArray &)), this,
			SLOT(slotDataArrived(KIO::Job *, const QByteArray &)) );
		connect( pJob, SIGNAL(result(KJob *)), this, SLOT(slotJobFinished(KJob *)));

		dDebug() << "Observation Reader: " << d->vObservationJobs.count();
	}
	dEndFunct();
}

void
AccuWeatherIon::slotDataArrived( KIO::Job * pJob, const QByteArray & data )
{
	if( data.isEmpty() || !d->vObservationJobs.contains(pJob) )
		return;
//	dStartFunct();
	XmlJobData * pJobData = d->vObservationJobs[pJob];
	pJobData->xmlReader.addData(data);
/*	
	QFile file(QString("/tmp/ion_accuweather_%1.txt").arg(pJobData->sLocation.replace("|","_")));
	if( file.open(QIODevice::Append|QIODevice::Text) )
	{
		file.write(data);
		file.close();
	}
*/
//	dEndFunct();
}

void
AccuWeatherIon::slotJobFinished( KJob * pJob )
{
	dStartFunct();
	if( !d->vObservationJobs.contains(pJob) )
	{
		dWarning() << "Does not contain current xml-job!!";
		dEndFunct();
		return;
	}
	XmlJobData * pXmlData = d->vObservationJobs[pJob];
	ImageData  * pImageData = NULL;

	if( !pXmlData->imageUrl.isEmpty() && d->vImageData.contains(pXmlData->imageUrl) )
		pImageData = d->vImageData[pXmlData->imageUrl];

	if( pJob->error() != 0 )
	{
		dWarning()  << pXmlData->sSource << pJob->errorString();

		//--- we still have to unregister from the image data ---
		if( pImageData )
			pImageData->iLocationCounter -= 1;
		else
			dWarning() << pXmlData->sSource << "xml-job has no image-data!";
	}
	else
	{
		dDebug() << pXmlData->sSource << "xml-job has successfully completed";

		WeatherData * pWeather = new WeatherData;
		pWeather->city = pXmlData->sLocation;
		pWeather->source = pXmlData->sSource;
		pWeather->iUtcHourOffset = 0;
		pWeather->iUtcMinutesOffset = 0;

		readWeatherXmlData( pXmlData->xmlReader, *pWeather );

		if (pImageData && !pImageData->bReady)
		{
			//  The attached image data has not been downloaded yet, so we store the weather data
			//  to the pending update sources.
			pImageData->vWeatherSources.append( pWeather );
		}
		else
		{
			//--- Update the source and disconnect from the image data ---
			updateWeatherSource( *pWeather, pImageData );
			qDeleteAll( pWeather->vForecasts.begin(), pWeather->vForecasts.end() );
			delete pWeather;
			
			if (pImageData)
				pImageData->iLocationCounter -= 1;
		}
	}
	//--- when we do not need the image anymore and the image job has been completed remove the image data ---
	if( pImageData && pImageData->bReady && pImageData->iLocationCounter <= 0 )
	{
		d->vImageData.remove( pXmlData->imageUrl );
		delete pImageData;
	}

	d->vObservationJobs.remove( pJob );
	d->vActiveJobs.remove( QString("%1|%2").arg(pXmlData->sLocationCode).arg(ActionWeather) );
	pJob->deleteLater();
	delete pXmlData;

	dDebug() << "Observation Reader: " << d->vObservationJobs.count();
	dDebug() << "Image Jobs: " << d->vImageJobs.count();
	dDebug() << "Image Data: " << d->vImageData.count();
	dEndFunct();
}

bool
AccuWeatherIon::readWeatherXmlData( QXmlStreamReader & xml, WeatherData & weather )
{
	dStartFunct();
	int iState = 0;
	while( !xml.atEnd() )
	{
		xml.readNext();
//		dTracing() << xml.name() << iState;
		if( xml.isStartElement() )
		{
			if( iState == 0 && xml.name().compare("adc_database") == 0 )
				iState = 1;
			else if( iState == 1 )
			{
				if( xml.name().compare("units") == 0 )
					readUnits( xml, weather );
				else if( xml.name().compare("local") == 0 )
					readLocal( xml, weather );
				else if( xml.name().compare("currentconditions") == 0 )
					readCurrentConditions( xml, weather );
				else if( xml.name().compare("forecast") == 0 )
					iState = 2;
			}
			else if( iState == 2 && xml.name().compare("day") == 0 )
			{
				bool bOk = false;
				int iIndex = xml.attributes().value("number").toString().toInt( &bOk, 10 );
				if( bOk && iIndex > 0 && iIndex <= 5 )
				{
					ForecastDay * pDay = new ForecastDay;
					weather.vForecasts.append( pDay );
					readForecastConditions( xml, *pDay );
				}
			}
//			else
//				dDebug() << xml.name() << iState << "unkown tag....";
		}
		else if( xml.isEndElement() )
		{
			if( iState == 2 && xml.name().compare("forecast") == 0 )
				iState = 1;
		}
	}
	if( xml.hasError() )
		dWarning() << xml.errorString();
	dEndFunct();
	return !xml.hasError();
}

void
AccuWeatherIon::readUnits( QXmlStreamReader & xml, WeatherData & weather )
{
	dStartFunct();
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isEndElement() && xml.name().compare("units") == 0 )
			break;
		else if( xml.isStartElement() )
		{
//			dDebug() << xml.name().toString();
			if( xml.name().compare("temp") == 0 )
				weather.iTempSystem = ( xml.readElementText().compare("F") == 0 ? FAHRENHEIT : CELSIUS );
			else if( xml.name().compare("speed") == 0 )
				weather.iSpeedSystem = ( xml.readElementText().compare("MPH") == 0 ? MPH : KPH );
			else if( xml.name().compare("dist") == 0 )
			{
				QString sDist = xml.readElementText();
				if( sDist.compare("MI") == 0 )
					weather.iDistanceSystem = MILES;
				else
					weather.iDistanceSystem = 0;
			}
			else if( xml.name().compare("pres") == 0 )
			{
				QString sPres = xml.readElementText();
				if( sPres.compare("IN") == 0 )
					weather.iPressureSystem = INCHESHG;
				else
					weather.iPressureSystem = 0;
			}
			else if( xml.name().compare("prec") == 0 )
			{
			}
		}
	}
	if( xml.hasError() )
		dWarning() << xml.errorString();
	dEndFunct();
}

void
AccuWeatherIon::readLocal( QXmlStreamReader & xml, WeatherData & weather )
{
	dStartFunct();
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isEndElement() && xml.name().compare("local") == 0 )
			break;
		else if( xml.isStartElement() )
		{
			if( xml.name().compare("lat") == 0 )
				weather.sLatitude = xml.readElementText();
			else if( xml.name().compare("lon") == 0 )
				weather.sLongitude = xml.readElementText();
/*			else if( xml.name().compare("time") == 0 )
				weather.sObservationPeriode = xml.readElementText();
*/			else if( xml.name().compare("timeZone") == 0 )
			{
				QString sTimeZone(xml.readElementText());
				int iPos( sTimeZone.indexOf(QChar(':')) );
				if( iPos > 0 )
				{
					weather.iUtcHourOffset = sTimeZone.left(iPos).toShort();
					weather.iUtcMinutesOffset = sTimeZone.right(sTimeZone.length()-iPos-1).toShort();
				}
			}
		}
	}
	if( xml.hasError() )
		dWarning() << xml.errorString();
	dEndFunct();

}

void
AccuWeatherIon::readCurrentConditions( QXmlStreamReader & xml, WeatherData & weather )
{
	dStartFunct();
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isEndElement() && xml.name().compare("currentconditions") == 0 )
			break;
		else if( xml.isStartElement() )
		{
//			dDebug() << xml.name().toString();
			if( xml.name().compare("url") == 0)
				weather.forecastUrl = xml.readElementText();
			else if( xml.name().compare("temperature") == 0 )
				weather.temperature = xml.readElementText();
			else if( xml.name().compare("realfeel") == 0 )
				weather.realfeel = xml.readElementText();
			else if( xml.name().compare("humidity") == 0 )
				weather.humidity = xml.readElementText();
			else if( xml.name().compare("weathertext") == 0 )
				weather.weathertext = xml.readElementText();
			else if( xml.name().compare("weathericon") == 0 )
				weather.weathericon = getWeatherIcon( d->vConditionList, xml.readElementText() );
			else if( xml.name().compare("windspeed") == 0 )
				weather.windspeed = xml.readElementText();
			else if( xml.name().compare("winddirection") == 0 )
				weather.winddirection = xml.readElementText();
			else if( xml.name().compare("windgusts") == 0 )
				weather.windgusts = xml.readElementText();
			else if( xml.name().compare("windchill") == 0 )
				weather.windchill = xml.readElementText();
			else if( xml.name().compare("dewpoint") == 0 )
				weather.dewpoint = xml.readElementText();
			else if( xml.name().compare("pressure") == 0 )
			{
				weather.pressureTendency = xml.attributes().value("state").toString();
				if( weather.pressureTendency.compare("Unavailable", Qt::CaseInsensitive) == 0 )
					weather.pressureTendency.clear();
				else
					weather.pressure = xml.readElementText();
			}
			else if( xml.name().compare("visibility") == 0 )
				weather.visibility = xml.readElementText();
			else if( xml.name().compare("precip") == 0 )
			{
			}
			else if( xml.name().compare("uvindex") == 0 )
			{
				weather.uvIndex = xml.attributes().value("index").toString();
				weather.uvRating = xml.readElementText();
			}
			
			// this tag contains the same value, than local/time but should make content more clear
			else if (xml.name().compare("observationtime") == 0)
			{
				QString timeString = xml.readElementText();
				weather.observationPeriode = QTime::fromString(timeString, "h:m ap");
			}
		}
	}
	if( xml.hasError() )
		dWarning() << xml.errorString();
	dEndFunct();
}

void
AccuWeatherIon::readForecastConditions( QXmlStreamReader & xml, ForecastDay & forecast )
{
	dStartFunct();
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isEndElement() && xml.name().compare("day") == 0 )
			break;
		else if( xml.isStartElement() )
		{
//			dDebug() << xml.name().toString();
			if( xml.name().compare("obsdate") == 0 )
				forecast.obsdate = xml.readElementText();
			else if( xml.name().compare("sunrise") == 0 )
				forecast.sunrise = xml.readElementText();
			else if( xml.name().compare("sunset") == 0 )
				forecast.sunset = xml.readElementText();
			else if( xml.name().compare("daytime") == 0 )
			{
				dTracing() << "Parse day time values:";
				readWeatherConditions( xml, forecast.DayTime );
			}
			else if( xml.name().compare("nighttime") == 0 )
			{
				dTracing() << "Parse night time values:";
				readWeatherConditions( xml, forecast.NightTime );
			}
		}
	}
	dTracing() << "Obsdate:" << forecast.obsdate;
	dTracing() << "Sunrise/Sunset:" << forecast.sunrise << " / " << forecast.sunset;

	if( xml.hasError() )
		dWarning() << xml.errorString();
	dEndFunct();
}

void
AccuWeatherIon::readWeatherConditions( QXmlStreamReader & xml, ForecastConditions & conditions )
{
	dStartFunct();
	while( !xml.atEnd() )
	{
		xml.readNext();
		if( xml.isEndElement() && (xml.name().compare("daytime") == 0 || xml.name().compare("nighttime") == 0) )
			break;
		else if( xml.isStartElement() )
		{
//			dDebug() << xml.name().toString();
			if( xml.name().compare("txtshort") == 0 )
				conditions.weathertext = xml.readElementText();
			else if( xml.name().compare("weathericon") == 0 )
				conditions.weathericon = getWeatherIcon( d->vConditionList, xml.readElementText() );
			else if( xml.name().compare("hightemperature") == 0 )
				conditions.hightemperature = xml.readElementText();
			else if( xml.name().compare("lowtemperature") == 0 )
				conditions.lowtemperature = xml.readElementText();
			else if( xml.name().compare("realfeelhigh") == 0 )
				conditions.realfeelhigh = xml.readElementText();
			else if( xml.name().compare("realfeellow") == 0 )
				conditions.realfeellow = xml.readElementText();
			else if( xml.name().compare("windspeed") == 0 )
				conditions.windspeed = xml.readElementText();
			else if( xml.name().compare("winddirection") == 0 )
				conditions.winddirection = xml.readElementText();
			else if( xml.name().compare("windgust") == 0 )
				conditions.windgust = xml.readElementText();
			else if( xml.name().compare("maxuv") == 0 )
				conditions.uvIndex = xml.readElementText();
		}
	}
/*	dTracing() << "Text:" << conditions.weathertext;
	dTracing() << "Icon:" << conditions.weathericon;
	dTracing() << "Temp:" << conditions.hightemperature << "/" << conditions.lowtemperature;
	dTracing() << "Realfeel:" << conditions.realfeelhigh << "/" << conditions.realfeellow;
	dTracing() << "Wind:" << conditions.windspeed << conditions.winddirection;
	dTracing() << "WindGust:" << conditions.windgust;
	dTracing() << "Max UV:" << conditions.uvIndex;
*/
	if( xml.hasError() )
		dWarning() << xml.errorString();
	dEndFunct();
}

void
AccuWeatherIon::updateWeatherSource( const WeatherData & weather, const ImageData * pImageData )
{
	dStartFunct()	<< weather.source;
	removeAllData( weather.source );	// clear the old values
	setData(weather.source, Data());	// start the update timer

	setData( weather.source, "Dewpoint Unit", QString::number(weather.iTempSystem) );
	setData( weather.source, "Temperature Unit", QString::number(weather.iTempSystem) );
	setData( weather.source, "Wind Speed Unit",  QString::number(weather.iSpeedSystem) );
	setData( weather.source, "Wind Gust Unit", QString::number(weather.iSpeedSystem) );
	setData( weather.source, "Visibility Unit", QString::number(weather.iDistanceSystem) );

	setData( weather.source, "Place", weather.city );
	setData( weather.source, "Condition Icon", stringConverter(weather.weathericon) );
	setData( weather.source, "Current Conditions", stringConverter(weather.weathertext) );
	setData( weather.source, "Dewpoint", stringConverter(weather.dewpoint) );
	setData( weather.source, "Humidity", stringConverter(weather.humidity) );
	setData( weather.source, "Pressure", stringConverter(weather.pressure) );
	if( !weather.pressure.isEmpty() )
	{
		setData( weather.source, "Pressure Tendency", stringConverter(weather.pressureTendency) );
		setData( weather.source, "Pressure Unit", QString::number(weather.iPressureSystem) );
	}
	setData( weather.source, "Temperature", stringConverter(weather.temperature) );
	setData( weather.source, "UV Index", stringConverter(weather.uvIndex) );
	setData( weather.source, "UV Rating", stringConverter(weather.uvRating) );
	setData( weather.source, "Visibility", stringConverter(weather.visibility) );
	setData( weather.source, "Wind Direction", stringConverter(weather.winddirection) );
	setData( weather.source, "Wind Speed", stringConverter(weather.windspeed) );
	setData( weather.source, "Wind Gust", stringConverter(weather.windgusts) );
	setData( weather.source, "Windchill", stringConverter(weather.windchill) );
	
	if( !weather.sLatitude.isEmpty() && !weather.sLongitude.isEmpty() )
	{
		setData( weather.source, "Longitude", weather.sLongitude );
		setData( weather.source, "Latitude", weather.sLatitude );
	}

	if( weather.vForecasts.count() > 0 )
	{
		QDate currDate( QDate::fromString(weather.vForecasts.at(0)->obsdate, "M/d/yyyy") );
		dInfo() << "Observation day: " << currDate;

		QTime sunrise( QTime::fromString(weather.vForecasts.at(0)->sunrise, "h:mm AP") );
		QTime sunset( QTime::fromString(weather.vForecasts.at(0)->sunset, "h:mm AP") );
		if( sunrise.isValid() && sunset.isValid() && currDate.isValid())
		{
#if KDE_VERSION_MINOR <= 2
			setData( weather.source, "Sunrise At", sunrise.toString("h:m") );
			setData( weather.source, "Sunset At", sunset.toString("h:m") );
#else
			// Format of sunrise and sunset: "Tuesday November 29, 2011 at 07:43"
			QDateTime dateTime(currDate, sunrise);
			setData(weather.source, "Sunrise At", QString("%1, %2 at %3")
				.arg(dateTime.toString("dddd MMMM dd"))
				.arg(dateTime.date().year())
				.arg(dateTime.toString("HH:mm")));
			
			dateTime.setTime(sunset);
			setData(weather.source, "Sunset At", QString("%1, %2 at %3")
				.arg(dateTime.toString("dddd MMMM dd"))
				.arg(dateTime.date().year())
				.arg(dateTime.toString("HH:mm")));
#endif
		}

		if( weather.observationPeriode.isValid() )
		{
			KDateTime dateTime( currDate, weather.observationPeriode, KDateTime::Spec::OffsetFromUTC(weather.iUtcHourOffset) );
			if( dateTime.isValid() )
				setData( weather.source, "Observation Period", dateTime.toString("%d.%m.%Y @ %H:%M") );
		}

		short iDay(0);
		QList<ForecastDay *>::const_iterator itDay = weather.vForecasts.constBegin();
		for( ; itDay != weather.vForecasts.constEnd(); ++itDay, ++iDay )
		{
			QString sDayName("???");
			int iWeekday = currDate.dayOfWeek();
			if( iWeekday >= 1 && iWeekday<=7 )
				sDayName = i18n(vWeekdays[iWeekday-1]);

			updateWeatherCondition( weather.source, iDay, sDayName, true, (*itDay)->DayTime );
			updateWeatherCondition( weather.source, iDay, sDayName, false, (*itDay)->NightTime );

			if( iDay > 0 )
				updateSun( weather.source, iDay, sDayName, **itDay );

			currDate = currDate.addDays(1);
		}
		setData( weather.source, "Total Weather Days", QString::number(2*weather.vForecasts.count()) );
	}

	if( pImageData && !pImageData->image.isNull() )
		setData( weather.source, "Satellite Map", pImageData->image );

	setData( weather.source, "Credit", i18n("Supported by AccuWeather") );
	
	if (!weather.forecastUrl.isEmpty())
		setData( weather.source, "Credit Url", weather.forecastUrl );
	
	dEndFunct();
}

void
AccuWeatherIon::updateWeatherCondition( const QString & source,
                                        int iDayIndex,
                                        const QString & dayName,
                                        bool bDayTime,
                                        const ForecastConditions & conditions )
{
	QString sKey( QString("Short Forecast Day %1").arg( iDayIndex *2 + (bDayTime ? 0 : 1) ) );
	QString sDayName;
	if( iDayIndex == 0 )
		sDayName = (bDayTime ? "Today" : "Tonight");
	else
		sDayName = (bDayTime ? dayName : dayName + " night");

	setData( source, sKey,
		 QString("%1|%2|%3|%4|%5|N/A")
			.arg( sDayName )
			.arg( stringConverter(conditions.weathericon) )
			.arg( stringConverter(conditions.weathertext) )
			.arg( stringConverter(conditions.hightemperature) )
			.arg( stringConverter(conditions.lowtemperature) ) );

	sKey = QString("Forecast Extra Day %1").arg( iDayIndex *2 + (bDayTime ? 0 : 1) );
	setData( source, sKey,
	 	QString("%1|%2|%3|%4|%5|%6|%7|%8")
			.arg( sDayName )
			.arg( stringConverter(conditions.windspeed) )
			.arg( stringConverter(conditions.winddirection) )
			.arg( stringConverter(conditions.windgust) )
			.arg( stringConverter(conditions.uvIndex) )
			.arg( "N/A" )	// UV Rating
			.arg( stringConverter(conditions.realfeelhigh) )
			.arg( stringConverter(conditions.realfeellow) ) );
}

void
AccuWeatherIon::updateSun( const QString & source,
						   int iDayIndex,
						   const QString & sDayName,
						   const ForecastDay & day )
{
	QTime sunrise( QTime::fromString(day.sunrise, "h:m AP") );
	QTime sunset( QTime::fromString(day.sunset, "h:m AP") );

	setData( source, QString("Forecast Sun %1").arg(iDayIndex),
		QString("%1|%2|%3")
			.arg(sDayName)
			.arg(sunrise.isValid() ? sunrise.toString("hh:mm") : "N/A")
			.arg(sunset.isValid() ? sunset.toString("hh:mm") : "N/A") );
}


/*********************************************************************************\
*                 G E T   W E A T H E R   I N F O R M A T I O N S                 *
\*********************************************************************************/

QByteArray
AccuWeatherIon::getImageUrl( const QString & sLocationCode ) const
{
	QStringList lst = sLocationCode.split('|',QString::SkipEmptyParts);

	//--- When lst contains just one entry, we assume it is a city in the United States ---
	if (lst.count() == 1)
		return QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/EI/iseun.jpg");

	QByteArray sUrl;
	
	if (lst[0].compare("EUR") == 0)
		sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/iseurm.jpg");
	
	else if (lst[0].compare("SAM") == 0)
	{
		if (lst[1].compare("AR") == 0 || lst[1].compare("CL") == 0)
			sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/iscsam.jpg");
		else
			sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/isnsam.jpg");
	}
	
	else if (lst[0].compare("NAM") == 0)
	{
		if (lst[1].compare("CA") == 0)
			sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/ir/iscanm.jpg");
		else if (lst[1].compare("MX") == 0)
			sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/ismex.jpg");
		else
			sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/ir/isun.jpg");
	}

	else if (lst[0].compare("CAC") == 0)
		sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/iscar.jpg");
	
	else if (lst[0].compare("OCN") == 0)
		sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/isaust.jpg");
	
	else if (lst[0].compare("ASI") == 0)
	{
		if (lst[1].compare("IN") == 0)
			sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/isindia.jpg");
		else if (lst[1].compare("RU") == 0)
		{
			QString code = lst.count() >= 3 ? lst[2].left(4) : "";
			
			if (code.compare("RS00") == 0 ||
			    code.compare("RS01") == 0 ||
			    code.compare("RS02") == 0 ||
			    code.compare("RS03") == 0)
			{
				sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/isasia.jpg");
			}
			else
				sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/iseurm.jpg");
		}
		else
			sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/isasia.jpg");
	}

	else if (lst[0].compare("MEA") == 0)
		sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/ismide.jpg");
	
	else if (lst[0].compare("AFR") == 0)
	{
		if (lst[1].compare("MA") == 0 ||
		    lst[1].compare("DZ") == 0 ||
		    lst[1].compare("TN") == 0 ||
		    lst[1].compare("LY") == 0 ||
		    lst[1].compare("EG") == 0)
		{
			sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/isafrn.jpg");
		}
		else
			sUrl = QByteArray("http://sirocco.accuweather.com/sat_mosaic_400x300_public/IR/isafrs.jpg");
	}
	
	return sUrl;
}

void
AccuWeatherIon::connectWithImageData( const QByteArray & imageUrl )
{
	dStartFunct();
	ImageData * pImageData = NULL;
	if( !d->vImageData.contains( imageUrl ) )
	{
		KIO::TransferJob * pJob = KIO::get( KUrl(imageUrl), KIO::Reload, KIO::HideProgressInfo );
		if( pJob )
		{
//			pJob->addMetaData("cookies", "none"); // Disable displaying cookies
			pImageData = new ImageData;
			pImageData->imageUrl = imageUrl;
			pImageData->bReady = false;
			pImageData->iLocationCounter = 1;
			d->vImageJobs.insert( pJob, pImageData );
			d->vImageData.insert( imageUrl, pImageData );

			connect( pJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
			         this, SLOT(image_slotDataArrived(KIO::Job *, const QByteArray &)) );
			connect(  pJob, SIGNAL(result(KJob *)), this, SLOT(image_slotJobFinished(KJob *)) );
		}
	}
	else
	{
		pImageData = d->vImageData[imageUrl];
		pImageData->iLocationCounter += 1;
	}
	dEndFunct();
}

void
AccuWeatherIon::image_slotDataArrived( KIO::Job * pJob, const QByteArray & data )
{
	if( data.isEmpty() || !d->vImageJobs.contains(pJob) )
		return;
	d->vImageJobs[pJob]->fetchedData.append( data );
}

void
AccuWeatherIon::image_slotJobFinished( KJob * pJob )
{
	if( !d->vImageJobs.contains(pJob) )
		return;
	dStartFunct();
	struct ImageData * pImageData = d->vImageJobs[pJob];
	pImageData->bReady = true;

	if( pJob->error() == 0 )
	{
		pImageData->image.loadFromData( pImageData->fetchedData, "JPG" );
	}
	else
	{
		dWarning() << pJob->errorString();
	}
	pImageData->fetchedData.clear();

	//--- update all pending weather sources ---
	while( pImageData->vWeatherSources.count() > 0 )
	{
		WeatherData * pWeather = pImageData->vWeatherSources.takeFirst();
		updateWeatherSource( *pWeather, pImageData );
		qDeleteAll( pWeather->vForecasts.begin(), pWeather->vForecasts.end() );
		delete pWeather;
		pImageData->iLocationCounter -= 1;
	}

	d->vImageJobs.remove( pJob );
	pJob->deleteLater();
	if( pImageData->iLocationCounter <= 0 )
	{
		d->vImageData.remove( pImageData->imageUrl );
		delete pImageData;
	}

	dDebug() << "Image Jobs: " << d->vImageJobs.count();
	dDebug() << "Image Data: " << d->vImageData.count();
	dEndFunct();
}

#if KDE_VERSION_MINOR >= 3
	 K_EXPORT_PLASMA_DATAENGINE(accuweather, AccuWeatherIon);
#else
	 K_EXPORT_PLASMA_ION(accuweather, AccuWeatherIon);
#endif
