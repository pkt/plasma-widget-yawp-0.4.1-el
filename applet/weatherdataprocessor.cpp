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
#include "configdialog/dlgtimezone.h"
#include "weatherdataprocessor.h"
#include "utils.h"
#include "logger/streamlogger.h"
#include "math.h"
#include "float.h"

//--- QT4 ---
#include <QDir>
#include <QFile>
#include <QFontMetrics>
#include <QHash>
#include <QTextLayout>

//--- KDE4 ---
#include <KDateTime>
#include <KTimeZones>
#include <KSystemTimeZones>
#include <Plasma/Theme>

#if (KDE_VERSION_MINOR == 3 && KDE_VERSION_RELEASE >= 70) || KDE_VERSION_MINOR == 4
	#include <KUnitConversion/Value>
#endif

#include <QMessageBox>

#define MAX_PROPERTY_TEXT_WIDTH 258.0f
#include <sys/stat.h>

const QString WeatherDataProcessor::CacheDirectory = QDir::homePath() + QLatin1String("/.cache/yawp");
static const char vWeekdays [7][4] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};


struct WeatherDataProcessor::Private
{
	const Yawp::Storage           * pStorage;

	QHash<QString, QString>         vConditionIcons;

	YAWP_DISTANCE_UNIT              distanceSystem;
	YAWP_PRESSURE_UNIT              pressureSystem;
	YAWP_SPEED_UNIT                 speedSystem;
	YAWP_TEMPERATURE_UNIT           temperatureSystem;

	bool                            requestTimeZone;

	QList<Yawp::DetailsProperty>    vDetailsProperty;

	bool initIconMap(const QString & resource);

	/** Check if a string is empty or contains "N/A" or "N/U".
	 *  In this cases the function returns false, otherwise true.
	 */
	bool checkStringValue( const QString & value ) const;
	
	QString createWindShortText(const YawpWeather & weather) const;
	QString createPressureText(const YawpWeather & weather) const;

	/** Parse a string that contains a float number.
	 *  Returns FLT_MAX when parsing fails.
	 */
	float parseFloat( const QString & value ) const;

	short  convertDistance( const QString & value, YAWP_DISTANCE_UNIT system ) const;
	short  convertPressure( const QString & value, YAWP_PRESSURE_UNIT system ) const;
	short  convertTemp( const QString & value, YAWP_TEMPERATURE_UNIT system ) const;
	short  convertSpeed( const QString & value, YAWP_SPEED_UNIT system ) const;

#if KDE_VERSION_MINOR <= 2
	WeatherUtils::Unit convertSpeed2Distance( WeatherUtils::Unit fromSpeed ) const;
#endif

	void setUVValues( YawpWeather & weather, const QString & sUVIndex, const QString & sUVRating ) const;

	bool isNightTime( QString & sShortDay ) const;
	bool findDateForWeekday( QDate & currDate, const QString & sLookingDayName ) const;

	QString getSourceCacheFileName( const CityWeather & pCity ) const;

	QString createPropertyText( const Yawp::DetailsProperty, const YawpDay & pDay, const YawpWeather & pWeather ) const;
	void createPropertyStringList( const YawpDay & day, YawpWeather & weather ) const;
	QSizeF doTextLayout( QTextLayout & textLayout, qreal dMaxWidth, qreal dOffset ) const;
};

WeatherDataProcessor::WeatherDataProcessor( const Yawp::Storage * pStorage )
	: d( new WeatherDataProcessor::Private )
{
	d->pStorage = pStorage;
	d->initIconMap( ":/iconnames.conf" );
	d->requestTimeZone = false;

	QDir dir(CacheDirectory);
	if( !dir.exists() )
		dir.mkpath(CacheDirectory);
}

WeatherDataProcessor::~WeatherDataProcessor()
{
	delete d;
}

YAWP_DISTANCE_UNIT WeatherDataProcessor::distanceSystem() const {return d->distanceSystem;}
void WeatherDataProcessor::setDistanceSystem( YAWP_DISTANCE_UNIT unitsystem ) {d->distanceSystem = unitsystem;}

YAWP_PRESSURE_UNIT WeatherDataProcessor::pressureSystem() const {return d->pressureSystem;}
void WeatherDataProcessor::setPressureSystem( YAWP_PRESSURE_UNIT unitsystem ) {d->pressureSystem = unitsystem;}

YAWP_TEMPERATURE_UNIT WeatherDataProcessor::temperatureSystem() const {return d->temperatureSystem;}
void WeatherDataProcessor::setTemperatureSystem( YAWP_TEMPERATURE_UNIT unitsystem ) {d->temperatureSystem = unitsystem;}

YAWP_SPEED_UNIT WeatherDataProcessor::speedSystem() const {return d->speedSystem;}
void WeatherDataProcessor::setSpeedSystem( YAWP_SPEED_UNIT unitsystem ) {d->speedSystem = unitsystem;}

void WeatherDataProcessor::setRequestTimeZone(bool request) {d->requestTimeZone = request;}
bool WeatherDataProcessor::requestTimeZone() const {return d->requestTimeZone;}

bool
WeatherDataProcessor::updateLocation( CityWeather & cityInfo,
                                      const Plasma::DataEngine::Data & data ) const
{
	/* When data is empty or data contains validate return.
	 * The data contains the key "validate" in case someone is searching for a city - we do not care about
	 * this data.
	 */
	if( data.isEmpty() || data.contains("validate") )
		return false;

/*** FOR DEBUG - PRINTS A SORTED LIST OF ALL KEYS THAT THIS PROVIDER/ION SUPPORTS ***/
	dInfo() << cityInfo.localizedCityString() << cityInfo.provider() << data;
/************************************************************************************/

	YAWP_TEMPERATURE_UNIT fromTempSystem
		= (YAWP_TEMPERATURE_UNIT)data.value("Temperature Unit").toInt();
	YAWP_TEMPERATURE_UNIT fromDewTempSystem
		= (YAWP_TEMPERATURE_UNIT)data.value("Dewpoint Unit").toInt();
	YAWP_SPEED_UNIT       fromSpeedSystem
		= (YAWP_SPEED_UNIT)data.value("Wind Speed Unit").toInt();
	YAWP_DISTANCE_UNIT    fromVisibilitySystem
		= (YAWP_DISTANCE_UNIT)data.value("Visibility Unit").toInt();
	YAWP_PRESSURE_UNIT    fromPressureSystem
		= (YAWP_PRESSURE_UNIT)data.value("Pressure Unit").toInt();

	//--- delete all old days ---
	cityInfo.deleteAllDays();

	QString sObservation( data.value("Observation Period").toString() );
	if( !sObservation.isEmpty() && sObservation.compare("N/A") != 0 )
	{
		QDateTime obsTime = QDateTime::fromString( sObservation, "dd.MM.yyyy @ h:mm");
		if( !obsTime.isValid() )
			obsTime = KDateTime::fromString(sObservation, "%d.%m.%Y @ %H:%M %z")/*.toLocalZone()*/.dateTime();

		cityInfo.setObservationPeriode(obsTime);
	}
	else
		cityInfo.setObservationPeriode( QDateTime() );

	//--- Get weather forecast information ---
	cityInfo.setCredit( data.value("Credit").toString() );
	
	//-- This is a stupid workaround ---
	// ION BBC Weather Engine is returning the url to the weather forecast of current city
	// Our own ION's accuweather and wunderground is following this example since version 0.4.0
	// ION "Environment Canada" and "NOAA" does not returning "Credit Url" at all, probably because the provider
	// does not support the information to send the url to the weather forecast
	// Our own ION Google is following the example of ION "Environment Canada" and "NOAA"
	// But ION wetter.com is returning the base url to the weather provider.
	// Up to KDE version 4.2 all ION's was returning the base url to the weather provider,
	// but at one point this has been changed and probably KDE-Developer forgot about ION wetter.com
	//... at least I think so...
	// Therefore we just check whether Credit Url contains a path or not - in the second case we suppose
	// it is just the base url to the weather provider. In this case we do not use the credit Url.
	KUrl creditUrl(data.value("Credit Url").toString());
	if (creditUrl.isValid() && creditUrl.hasPath())
		cityInfo.setCreditUrl(data.value("Credit Url").toString());
	
	int iForecastDays = data.value("Total Weather Days", "0").toInt();
	QDate currDate;	// date of current day
	int iTodayDays(0);
	YawpDay * pLastDay = NULL;
	QString sLastDayName;

	for( int iDay = 0; iDay < iForecastDays; ++iDay )
	{
		QVariant var;
		var = data.value( QString("Short Forecast Day %1").arg(iDay) );
		QStringList vTokens = var.toString().split("|", QString::SkipEmptyParts);

		YawpWeather * pWeather = NULL;

		//--- find the date to the dayname ---
		if( vTokens.at(0).compare( "Today", Qt::CaseInsensitive) == 0 ||
		    vTokens.at(0).compare( i18n("Today"), Qt::CaseInsensitive) == 0 ||
		    vTokens.at(0).startsWith( "day", Qt::CaseInsensitive) ||  // <--- envcan specific
		    vTokens.at(0).startsWith( i18n("day"), Qt::CaseInsensitive) )	// <--- wettercom specific
		{
			dTracing() << "Adding Today for " << vTokens;
			iTodayDays += 1;
			pLastDay = new YawpDay;
			cityInfo.days().append( pLastDay );
			pWeather = &pLastDay->weather();
		}
		else if( vTokens.at(0).compare("Tonight", Qt::CaseInsensitive) == 0 ||
		    vTokens.at(0).compare( i18n("Tonight"), Qt::CaseInsensitive) == 0 ||
		    vTokens.at(0).startsWith( "Nite", Qt::CaseInsensitive) ||        // <--- envcan specific
		    vTokens.at(0).startsWith( i18n("Nite"), Qt::CaseInsensitive) ||  // <--- envcan specific
		    vTokens.at(0).startsWith( "Night", Qt::CaseInsensitive) ||       // <--- wettercom specific
		    vTokens.at(0).startsWith( i18n("Night"), Qt::CaseInsensitive) )  // <--- wettercom specific
		{
			dTracing() << "Adding Tonight for " << vTokens;
			iTodayDays += 1;
			if( !pLastDay )
			{
				pLastDay = new YawpDay;
				cityInfo.days().append( pLastDay );
				pWeather = &pLastDay->weather();
			}
			else
			{
				pWeather = &pLastDay->nightWeather();
				pLastDay->setHasNightValues(true);
			}
			pWeather->setDayTime(false);
		}
		else if( !currDate.isValid() )
		{
			dTracing() << "Date is not falid -> check " << vTokens;
			QString sShortDay( vTokens.at(0) );
			bool bNightTime = d->isNightTime( sShortDay );

			QDate date( cityInfo.localTime().date().addDays( iTodayDays > 0 ? 2 : 1 ) );

			if( d->findDateForWeekday( date, sShortDay ) )
			{
				dTracing() << "Adding weather for" << date;
				int iFirstDateIndexDay = cityInfo.days().count();
				sLastDayName = sShortDay;
				currDate = date;

				pLastDay = new YawpDay;
				pLastDay->setDate( currDate );
				cityInfo.days().append( pLastDay );
				pWeather = &pLastDay->weather();
				pWeather->setDayTime( !bNightTime );

				//--- set the date for all previous days according to the current date. ---
				date = date.addDays( -iFirstDateIndexDay );
				for( int iDay = 0; iDay < iFirstDateIndexDay; ++iDay )
				{
					cityInfo.days().at(iDay)->setDate( date );
					date = date.addDays(1);
				}
			}
		}
		else
		{
			dTracing() << "check -> " << vTokens;
			QString sShortDay( vTokens.at(0) );
			bool bNightTime = d->isNightTime( sShortDay );
			if( bNightTime && pLastDay && sLastDayName.compare( sShortDay ) == 0 )
			{
				pWeather = &pLastDay->nightWeather();
				pWeather->setDayTime( false );
				pLastDay->setHasNightValues( true );
			}
			else
			{
				currDate = currDate.addDays(1);
				pLastDay = new YawpDay;
				pLastDay->setDate( currDate );
				cityInfo.days().append( pLastDay );
				sLastDayName = sShortDay;
				pWeather = &pLastDay->weather();
				pWeather->setDayTime( !bNightTime );
			}
		}

		if( pWeather )
		{
			setForecastValues( *pWeather, vTokens, fromTempSystem, fromSpeedSystem );
			setForecastExtraValues( *pWeather,
			                        data.value( QString("Forecast Extra Day %1").arg(iDay) ).toString(),
			                        fromTempSystem, fromSpeedSystem );
		}
	}

	//--- Set the sun for all forecast days - this is an accuweather specific key. ---
	for( int iDay = 1; iDay < cityInfo.days().count(); ++iDay )
		setForecastSun( *cityInfo.days().at(iDay), data.value( QString("Forecast Sun %1").arg(iDay) ).toString() );

	/***  Add the current weather informations to the first day ***
	*/
	QString sTmp;
	YawpDay * pDay = NULL;
	if( cityInfo.days().count() == 0 )
	{
		pDay = new YawpDay;
		pDay->setDate( QDate::currentDate() );
		cityInfo.days().append( pDay );
	}
	else
		pDay = cityInfo.days().at(0);

	/*** PARSE SUNRISE AND SUNSET ***/
	KDateTime sunTime;
	sunTime = KDateTime::fromString(data.value("Sunrise At").toString(), "%A %B %e, %Y at %H:%M %Z", KSystemTimeZones::timeZones(), true);
	if( sunTime.isValid() )
		pDay->setSunrise( sunTime.time() );
	else
		pDay->setSunrise( QTime::fromString(data.value("Sunrise At").toString(), "h:m") );

	sunTime = KDateTime::fromString(data.value("Sunset At").toString(), "%A %B %e, %Y at %H:%M %Z", KSystemTimeZones::timeZones(), true);
	if( sunTime.isValid() )
		pDay->setSunset( sunTime.time() );
	else
		pDay->setSunset(  QTime::fromString(data.value("Sunset At").toString(), "h:m") );


	YawpWeather & weather
		= (pDay->hasNightValues() && !cityInfo.isDayTime(pDay) ? pDay->nightWeather() : pDay->weather());

	if( data.contains("Condition Icon") )
		weather.setCurrentIconName( mapConditionIcon( data.value("Condition Icon").toString() ) );
	if( data.contains("Temperature") )
		weather.setCurrentTemperature( d->convertTemp(  data.value("Temperature").toString(), fromTempSystem ) );
	if( data.contains("Dewpoint") )
		weather.setDewpoint( d->convertTemp( data.value("Dewpoint").toString(), fromDewTempSystem ) );
	if( data.contains("Visibility") )
		weather.setVisibility( d->convertDistance( data.value("Visibility").toString(), fromVisibilitySystem ) );

	weather.setPressure( d->convertPressure( data.value("Pressure").toString(), fromPressureSystem ) );
	sTmp = data.value("Pressure Tendency").toString();
	if( d->checkStringValue( sTmp ) )
		weather.setPressureTendency( sTmp );

	if( data.contains("Wind Speed") )
		weather.setWindSpeed( d->convertSpeed( data.value("Wind Speed").toString(),  fromSpeedSystem ) );
	sTmp = data.value("Wind Direction").toString();
	if( d->checkStringValue( sTmp) )
		weather.setWindDirection( sTmp );
	
	weather.setWindShortText( d->createWindShortText(weather) );
	weather.setPressureShortText( d->createPressureText(weather) );

	sTmp = data.value("Current Conditions").toString();
	if( d->checkStringValue( sTmp ) )
		weather.setDescription( sTmp );

	if( data.contains("Humidity") )
		weather.setHumidity( (short)(data.value("Humidity").toString().replace("%", " ").toDouble()) );

	d->setUVValues( weather, data.value("UV Index").toString(), data.value("UV Rating").toString() );


	//--- remove all old days ---
	//
	dTracing() << "Check and remove old days (currently contains" << cityInfo.days().count() << "days)";
	bool bRemovedOldDays( false );
	currDate = cityInfo.localTime().date();
	QDate yesterday = currDate.addDays(-1);
	while( cityInfo.days().count() > 0 )
	{
		YawpDay * pDay = cityInfo.days().at(0);
		if( pDay->date() >= currDate )
		{
			break;
		}
		else if( pDay->date() == yesterday && pDay->sunrise().isValid() &&
		    QTime::currentTime() < pDay->sunrise() )
		{
			break;
		}
		else
		{
			dTracing() << "Remove day" << pDay->date() << "- locale today is" << currDate;
			bRemovedOldDays = true;
			delete cityInfo.days().takeFirst();
		}
	}
	dTracing() << "contains" << cityInfo.days().count() << "days";

	//--- Add YawpWeather::SunriseSunset property and sort the properties ---
	QList<YawpDay *>::iterator itDay = cityInfo.days().begin();
	for( ; itDay != cityInfo.days().end(); ++itDay )
	{
		YawpDay * pDay = (*itDay);

		d->createPropertyStringList( *pDay, pDay->weather() );

		if( pDay->hasNightValues() )
			d->createPropertyStringList( *pDay, pDay->nightWeather() );
	}

	if( !bRemovedOldDays )
	{
		//--- Add the satellite map to the city ---
		if( data.contains("Satellite Map") )
		{
			QImage img( data.value("Satellite Map").value<QImage>() );
			if( !img.isNull() )
				cityInfo.setSatelliteImage( img );
		}
	}
	
	return true;
}

bool
WeatherDataProcessor::updateCountryInfo( CityWeather & cityInfo, const Plasma::DataEngine::Data & data ) const
{
	if( data.isEmpty() || data.contains("validate") )
		return false;

	/*   Some weatherengines provides more informations for a certain location
	 *   doing a data request than searching for a city, so we update the locationinformations here.
	 */
	bool bUpdate(false);
	QString sCountry, sCountryCode;
	sCountry = data.value("Country").toString();
	if( !sCountry.isEmpty() &&
	    cityInfo.country().compare( sCountry ) != 0 &&
        Utils::GetCountryCode( sCountry, sCountryCode, d->pStorage ) )
	{
		cityInfo.setCountry( sCountry );
		cityInfo.setCountryCode( sCountryCode );
		bUpdate = true;
	}
	else if( cityInfo.countryCode().isEmpty() || cityInfo.country().isEmpty() )
	{
		QString sCity, sDistrict;

		if( cityInfo.countryCode().isEmpty() && data.contains("Place") )
		{
			Utils::ExtractLocationInfo(data.value("Place").toString(), sCity, sDistrict, sCountry);
			if( cityInfo.country().isEmpty() && !sCountry.isEmpty() )
			{
				cityInfo.setCountry( sCountry );
				bUpdate = true;
			}
			if( cityInfo.countryCode().isEmpty() && Utils::GetCountryCode( sCountry, sCountryCode, d->pStorage ) )
			{
				cityInfo.setCountryCode( sCountryCode );
				bUpdate = true;
			}
		}
	}
	return bUpdate;
}

bool
WeatherDataProcessor::saveData( const CityWeather & cityInfo, const Plasma::DataEngine::Data & data ) const
{
	if( !cityInfo.isValid() || cityInfo.days().count() == 0 )
		return false;

	bool bReturn = false;
	QFile file( d->getSourceCacheFileName(cityInfo) );
	if( file.open(QIODevice::WriteOnly|QIODevice::Truncate) )
	{
		QDataStream ds(&file);
		ds << cityInfo.days().at(0)->date();
		ds << (quint64)(cityInfo.days().count());
		Plasma::DataEngine::Data::const_iterator it = data.begin();
		for( ; it != data.end(); ++it )
			ds << it.key() << it.value();
		file.close();
		bReturn = true;
	}
	return bReturn;
}

bool
WeatherDataProcessor::loadData( CityWeather & cityInfo ) const
{
	if( !cityInfo.isValid() )
		return false;

	bool bReturn = false;
	QFile file( d->getSourceCacheFileName(cityInfo) );
	if( file.open(QIODevice::ReadOnly) )
	{
		QDataStream ds(&file);
		QDate date;
		quint64 iForecasts;
		ds >> date >> iForecasts;
		if( date.addDays(iForecasts-1) >= QDate::currentDate() )
		{
			Plasma::DataEngine::Data data;
			QString sKey;
			QVariant value;
			while( !ds.atEnd() )
			{
				ds >> sKey >> value;
				data.insert( sKey, value );
			}
			updateLocation( cityInfo, data );
			bReturn = true;
		}
		else
			dTracing() << "cached values are out of date for" << cityInfo.city() << cityInfo.country()
				<< "  provider:" << cityInfo.provider();
		file.close();
	}
	return bReturn;
}

void
WeatherDataProcessor::createDetailsPropertyMap( const QList<Yawp::DetailsProperty> & vProperties )
{
	d->vDetailsProperty.clear();
	d->vDetailsProperty = vProperties;
}

bool
WeatherDataProcessor::setForecastValues(
		YawpWeather                   & weather,
		const QStringList             & vForecast,
		YAWP_TEMPERATURE_UNIT           fromTempSystem,
		YAWP_SPEED_UNIT                 fromSpeedSystem ) const
{
	Q_UNUSED( fromSpeedSystem );

	bool bReturn(false);
	if( vForecast.count() >= 5 )
	{
		weather.setIconName( mapConditionIcon( vForecast.at(1) ) );
		weather.setDescription( vForecast.at(2) );
		weather.setHighTemperature( d->convertTemp(vForecast.at(3), fromTempSystem) );
		weather.setLowTemperature(  d->convertTemp(vForecast.at(4), fromTempSystem) );
		bReturn = true;
	}
	return bReturn;
}

bool
WeatherDataProcessor::setForecastExtraValues(
		YawpWeather                   & weather,
		const QString                 & sExtras,
		YAWP_TEMPERATURE_UNIT           fromTempSystem,
		YAWP_SPEED_UNIT                 fromSpeedSystem ) const
{
	if( sExtras.isEmpty() )
		return false;

	bool bReturn(false);
	QStringList vForecast = sExtras.split("|", QString::SkipEmptyParts);

	if( vForecast.count() >= 8 )
	{
		weather.setWindSpeed( d->convertSpeed(vForecast.at(1), fromSpeedSystem) );
		if( d->checkStringValue( vForecast.at(2) ) )
			weather.setWindDirection( vForecast.at(2) );
		d->setUVValues( weather, vForecast.at(4), vForecast.at(5) );
		weather.setTemperatureRealFeelHigh( d->convertTemp(vForecast.at(6), fromTempSystem) );
		weather.setTemperatureRealFeelLow(  d->convertTemp(vForecast.at(7), fromTempSystem) );

		weather.setWindShortText( d->createWindShortText(weather) );
		bReturn = true;
	}
	return bReturn;
}

bool
WeatherDataProcessor::setForecastSun( YawpDay & day, const QString & sValue ) const
{
	QStringList vSun = sValue.split("|", QString::SkipEmptyParts);

	if( vSun.count() >= 3 )
	{
		day.setSunrise( QTime::fromString(vSun.at(1), "hh:mm") );
		day.setSunset( QTime::fromString(vSun.at(2), "hh:mm") );
		return true;
	}
	return false;
}

void
WeatherDataProcessor::Private::setUVValues( YawpWeather & weather,
                                            const QString & sUVIndex,
                                            const QString & sUVRating ) const
{
	float fValue = parseFloat( sUVIndex );
	if( fValue < 1.0 || fValue > 11.0 )
		return;
	short iIndex = (short)fValue;
	weather.setUVIndex( iIndex );
	if( checkStringValue( sUVRating ) )
		weather.setUVRating( i18n(sUVRating.toUtf8().constData()) );
	else
	{
		//--- according to http://www.epa.gov/sunwise/uviscale.html ---
		if( iIndex <= 2 )
			weather.setUVRating( i18nc("UV Index Low", "Low") );
		else if( iIndex >= 3 && iIndex <= 5 )
			weather.setUVRating( i18nc("UV Index Moderate", "Moderate") );
		else if( iIndex >=6 && iIndex <= 7 )
			weather.setUVRating( i18nc("UV Index High", "High") );
		else if( iIndex >= 8 && iIndex <= 10 )
			weather.setUVRating( i18nc("UV Index Very High", "Very High") );
		else if( iIndex >= 11 )
			weather.setUVRating( i18nc("UV Index Extreme", "Extreme") );
	}
}

bool
WeatherDataProcessor::Private::checkStringValue( const QString & value ) const
{
	if( value.isEmpty() ||
	    value.compare("N/A", Qt::CaseInsensitive) == 0 ||
	    value.compare("N/U", Qt::CaseInsensitive) == 0 )
	{
		return false;
	}
	return true;
}

QString
WeatherDataProcessor::Private::createWindShortText(const YawpWeather & weather) const
{
	QString sText;
	if( weather.windSpeed() < SHRT_MAX )
	{
		sText.append( QString("%1 %2 ")
			.arg( weather.windSpeed() )
			.arg( Utils::GetUnitString( speedSystem ) ) );
		sText.append( i18nc("Wind direction", weather.windDirection().toUtf8().constData()) );
	}
	return sText;
}

QString
WeatherDataProcessor::Private::createPressureText(const YawpWeather & weather) const
{
	QString sText;
	if( weather.pressure() < SHRT_MAX )
	{
		sText.append( QString("\n%1 %2")
			.arg( weather.pressure() )
			.arg( Utils::GetUnitString(pressureSystem) ));

		if( !weather.pressureTendency().isEmpty() )
		{
			// Pressure tendency arrow
			QChar sPressureSymbol;
			QChar sPressureShort = weather.pressureTendency()[0].toLower();
			if (sPressureShort == QChar('s'))
				sPressureSymbol = QChar(0x279C); // steady
			if (sPressureShort == QChar('r') || sPressureShort == QChar('i'))
				sPressureSymbol = QChar(0x279A); // rising
			if (sPressureShort == QChar('f') || sPressureShort == QChar('d') )
				sPressureSymbol = QChar(0x2798); // falling
			
			sText.append( QString(" %1").arg(sPressureSymbol) );
		}
	}
	return sText;
}

float
WeatherDataProcessor::Private::parseFloat( const QString & value ) const
{
	if( !checkStringValue( value ) )
		return FLT_MAX;

	bool bOk = false;
	float fValue = value.toFloat( &bOk );
	if( bOk )
		return fValue;
	return FLT_MAX;
}

short
WeatherDataProcessor::Private::convertDistance( const QString & value, YAWP_DISTANCE_UNIT fromSystem ) const
{
	float fValue = parseFloat( value );
	if( fValue == FLT_MAX )
		return SHRT_MAX;

	if( fromSystem != distanceSystem )
	{
#if KDE_IS_VERSION(4,3,70)
		KUnitConversion::Value v( (double) fValue, fromSystem);
		fValue = v.convertTo(distanceSystem).number();
#elif KDE_VERSION_MINOR == 3
		fValue = WeatherUtils::convertDistance( fValue, fromSystem, distanceSystem );
#else
		fValue = WeatherUtils::convert( fValue, fromSystem, distanceSystem );
#endif
	}
	return (short)qRound(fValue);
}

short
WeatherDataProcessor::Private::convertPressure( const QString & value, YAWP_PRESSURE_UNIT fromSystem ) const
{
	float fValue = parseFloat( value );
	if( fValue == FLT_MAX )
		return SHRT_MAX;
	if( fromSystem != pressureSystem )
	{
#if KDE_IS_VERSION(4,3,70)
		KUnitConversion::Value v( (double) fValue, fromSystem);
		fValue = v.convertTo(pressureSystem).number();
#elif KDE_VERSION_MINOR >= 3
		fValue = WeatherUtils::convertPressure( fValue, fromSystem, pressureSystem );
#else
		fValue = WeatherUtils::convert( fValue, fromSystem, pressureSystem );
#endif
	}
	return (short)qRound(fValue);
}


short
WeatherDataProcessor::Private::convertTemp( const QString & value, YAWP_TEMPERATURE_UNIT fromSystem ) const
{
	float fValue = parseFloat( value );
	if( fValue == FLT_MAX )
		return SHRT_MAX;

	if( fromSystem != temperatureSystem )
	{
#if KDE_IS_VERSION(4,3,70)
		KUnitConversion::Value v( (double) fValue, fromSystem);
		fValue = v.convertTo(temperatureSystem).number();
#elif KDE_VERSION_MINOR >= 3
		fValue = WeatherUtils::convertTemperature( fValue, fromSystem, temperatureSystem );
#else
		fValue = WeatherUtils::convert( fValue, fromSystem, temperatureSystem );
#endif
	}
	return (short)qRound(fValue);
}

short
WeatherDataProcessor::Private::convertSpeed( const QString & value, YAWP_SPEED_UNIT fromSystem ) const
{
	float fValue = parseFloat( value );
	if( fValue == FLT_MAX )
		return SHRT_MAX;

	if( fromSystem != speedSystem )
	{
#if KDE_IS_VERSION(4,3,70)
		KUnitConversion::Value v( (double) fValue, fromSystem);
		fValue = v.convertTo(speedSystem).number();
#elif KDE_VERSION_MINOR >= 3
		fValue = WeatherUtils::convertSpeed( fValue, fromSystem, speedSystem );
#else
		/*   There is a bug - WeatherUtils can not convert speed values
		 *   from one to another unit system. Therefore i convert the units to distances.
		 *   I do not know if we need this in KDE 4.3 as well.
		 *   If this is the case we might want to convert the from and to speed unit systems in
		 *   function WeatherServiceModel::collectWeatherInformations() and pass them to this function...
		 */
		WeatherUtils::Unit from = convertSpeed2Distance( fromSystem );
		WeatherUtils::Unit to   = convertSpeed2Distance( speedSystem );
		fValue = WeatherUtils::convert( fValue, from, to );
#endif
	}
	return (short)qRound(fValue);
}

#if KDE_VERSION_MINOR <= 2
WeatherUtils::Unit
WeatherDataProcessor::Private::convertSpeed2Distance( WeatherUtils::Unit fromSpeed ) const
{
	WeatherUtils::Unit toDistance;
	switch( fromSpeed )
	{
	case WeatherUtils::MilesAnHour:      toDistance = WeatherUtils::Miles; break;
	case WeatherUtils::KilometersAnHour: toDistance = WeatherUtils::Kilometers; break;
	default:
		toDistance = fromSpeed;
	}
	return toDistance;
}
#endif

bool
WeatherDataProcessor::Private::isNightTime( QString & sShortDay ) const
{
	bool bNight(false);

	int iPos = sShortDay.lastIndexOf(QChar(' '));
	if( iPos > 0 )
	{
		QString sNightTag( sShortDay.right(sShortDay.length()-iPos-1) );
		sShortDay = sShortDay.left(iPos);

		if( sNightTag.compare("nt", Qt::CaseInsensitive) == 0 ||
			sNightTag.compare(i18n("nt"), Qt::CaseInsensitive) == 0 )
		{
			bNight = true;
		}
		else if( sNightTag.compare("night", Qt::CaseInsensitive) == 0 ||
			sNightTag.compare(i18n("night"), Qt::CaseInsensitive) == 0 )
		{
			bNight = true;
		}
	}
	return bNight;
}

bool
WeatherDataProcessor::Private::findDateForWeekday( QDate & currDate, const QString & sLookingDayName ) const
{
	QString sLookingDayNameUtf8 = sLookingDayName.toUtf8();
	QString vWeekdayUtf8;

	for(int iOffset = 0; iOffset < 6; ++iOffset )
	{
		dTracing() << "dayoffset" << iOffset << " -> " << currDate << "(dayindex" << currDate.dayOfWeek() << ")";
		int iWeekday = currDate.dayOfWeek() -1;
		if( iWeekday < 0 || iWeekday >= 7 )
			return false;	// this should not happen

		dTracing() << "  1) compare" << sLookingDayNameUtf8 << " with " << iWeekday << " -> " << QString(vWeekdays[iWeekday]).toUtf8();
		if( sLookingDayNameUtf8.startsWith( QString(vWeekdays[iWeekday]).toUtf8(), Qt::CaseInsensitive ) )
			return true;
		
		dTracing() << "  2) compare" << sLookingDayNameUtf8 << " with " << iWeekday << " -> " << i18n(vWeekdays[iWeekday]).toUtf8();
		if( sLookingDayNameUtf8.startsWith( i18n(vWeekdays[iWeekday]).toUtf8(), Qt::CaseInsensitive ) )
			return true;

		vWeekdayUtf8 = i18n(vWeekdays[iWeekday]).toUtf8();
		dTracing() << "  3) compare" << vWeekdayUtf8 << " with " << sLookingDayNameUtf8;
		if( vWeekdayUtf8.startsWith( sLookingDayNameUtf8, Qt::CaseInsensitive ) )
			return true;

		currDate = currDate.addDays(-1);
	}
	return false;
}

QString
WeatherDataProcessor::Private::getSourceCacheFileName( const CityWeather & cityInfo ) const
{
	QString sCityName( cityInfo.city() );
	for( int i=0; i < sCityName.length(); ++i )
	{
		QCharRef chr = sCityName[i];
		if( !chr.isLetterOrNumber() )
			chr = QChar(' ');
	}
	sCityName = sCityName.simplified().replace(QChar(' '), QChar('_'));
	QDir dir( CacheDirectory );
	return dir.absoluteFilePath( QString("%1_%2.dat").arg(cityInfo.provider()).arg(sCityName) );
}


bool
WeatherDataProcessor::Private::initIconMap(const QString & resource)
{
	vConditionIcons.clear();

	QFile file(resource);
	if( !file.open(QIODevice::ReadOnly) )
		return false;
	QTextStream stream(&file);
	QString line;
	QString icon;
	QString value;
	while(!stream.atEnd()) {
		line = stream.readLine().trimmed();
		if( line.isEmpty() || line.at(0) == '#' )
			continue;

		QStringList list( line.split( QLatin1String("="), QString::SkipEmptyParts ) );
		if( list.size() >= 2 )
		{
			QString sIcon( list.at(0).simplified() );
			QString sValue( list.at(1).simplified() );
			vConditionIcons[sIcon] = sValue;
		}
	}
	file.close();
	return true;
}

void
WeatherDataProcessor::Private::createPropertyStringList( const YawpDay & day, YawpWeather & weather ) const
{
	Plasma::Theme * theme = Plasma::Theme::defaultTheme();
	QFont font = theme->font(Plasma::Theme::DefaultFont);

	font.setBold(false);
	font.setPixelSize(15);
	QTextLayout textLayout;
	textLayout.setFont(font);
	
	QString sOffsetText(3, QChar(' '));
	QFontMetrics fm(font);
	qreal dOffsetWidth = (qreal)fm.width(sOffsetText);
	
	//  go though all property items that has been specified to show in details area
	//
	QList<Yawp::DetailsProperty>::const_iterator itMap = vDetailsProperty.begin();
	for( ; itMap != vDetailsProperty.end(); itMap++ )
	{
		//--- Create property text for requested property ---
		QString sText = createPropertyText(*itMap, day, weather);
		
		//--- When propery text is empty, continue with next property ---
		if( sText.isEmpty() )
			continue;
	
		/*   When a text needs to be wrapped around several lines.
		 *   than we will indent all lines expect the first one.
		 *   This way it is easy to see when a new property text starts.
		 */
		sText.replace('\n', QChar::LineSeparator);
		textLayout.setText( sText );
		doTextLayout( textLayout, MAX_PROPERTY_TEXT_WIDTH, dOffsetWidth);
		
		/*   Add the propery text lines for current property to property-text-line-list
		 */
		for( int iLine = 0; iLine < textLayout.lineCount(); iLine++ )
		{
			qreal dMaxLineWidth = (iLine == 0 ? MAX_PROPERTY_TEXT_WIDTH : MAX_PROPERTY_TEXT_WIDTH-dOffsetWidth);

			QTextLine line = textLayout.lineAt(iLine);
			int iStart = line.textStart();
			int iWidth = line.textLength();

			QChar::Category category = textLayout.text().at(iStart+iWidth-1).category();
			iWidth += (category == QChar::Separator_Line || category == QChar::Separator_Space ? -1 : 0);

			// When text line does not fit we have to trim textline
			// methode doTextLayout will abort text layout when one line does not fit
			QString sText = textLayout.text().mid( iStart, iWidth );
			if( line.naturalTextWidth() > dMaxLineWidth )
				sText =  fm.elidedText(sText, Qt::ElideRight, dMaxLineWidth);
			
			//--- Finally, append the text line to the property text lines ---
			if( iLine == 0 )
				weather.propertyTextLines().append( sText );
			else
				weather.propertyTextLines().append( sOffsetText + sText );
		}
	}
}

inline QString
WeatherDataProcessor::mapConditionIcon( const QString & iconName ) const
{
	return d->vConditionIcons.value( iconName, "weather-none-available" );
}

QString
WeatherDataProcessor::Private::createPropertyText( const Yawp::DetailsProperty property,
                                                   const YawpDay & day,
                                                   const YawpWeather & weather ) const
{
	QString sText;
  
	switch( property )
	{
	case Yawp::Dewpoint:
		if( weather.dewpoint() < SHRT_MAX )
			return i18n("Dewpoint: %1", weather.dewpoint())	+ QChar(0xB0);
		break;

	case Yawp::Pressure:
		if( weather.pressure() < SHRT_MAX )
		{
			sText = i18n("Pressure: %1 %2",
				weather.pressure(),
				Utils::GetUnitString(pressureSystem));

			if( weather.pressureTendency() != "Unavailable" &&
			    !weather.pressureTendency().isEmpty() )
			{
				sText.append( QString("  %1")
					.arg( i18nc("Pressure", weather.pressureTendency().toUtf8().constData())) );
			}
		}
		break;

	case Yawp::RealfeelTemperature:
		{
			int iTempCount = 0;
			
			QString sHigh(i18n("N/A"));
			QString sLow(i18n("N/A"));
			
			if( weather.temperatureRealFeelHigh() < SHRT_MAX )
			{
				sHigh = QString::number(weather.temperatureRealFeelHigh()) + QChar(0xB0);
				iTempCount += 1;
			}
			if( weather.temperatureRealFeelLow() < SHRT_MAX )
			{
				sLow = QString::number(weather.temperatureRealFeelLow()) + QChar(0xB0);
				iTempCount += 1;
			}
			
			//--- Return the string only when we have at least one valid temperature ---
			if( iTempCount > 0 )
				return i18n("Real feel: %1 / %2", sHigh, sLow );
		}
		break;

	case Yawp::SunriseSunset:
		{
			int iTempCount = 0;
			
			QString sSunrise(i18n("N/A"));
			QString sSunset(i18n("N/A"));
			
			if( day.sunrise().isValid() )
			{
				sSunrise = KGlobal::locale()->formatTime(day.sunrise());
				iTempCount += 1;
			}
			if( day.sunset().isValid() )
			{
				sSunset = KGlobal::locale()->formatTime(day.sunset());
				iTempCount += 1;
			}
			if( iTempCount > 0 )
				return i18n("Sunrise at %1, sunset at %2", sSunrise, sSunset);
		}
		break;

	case Yawp::UV:
		if( weather.uvIndex() < SHRT_MAX )
		{
			return i18n("UV Index: %1 %2",
				weather.uvIndex(), weather.uvRating());
		}
		break;

	case Yawp::Visibility:
		if( weather.visibility() < SHRT_MAX )
		{
			return i18n("Visibility: %1 %2",
				weather.visibility(),
				Utils::GetUnitString( distanceSystem ));
		}
		break;

	case Yawp::WeatherDescription:
		if( !weather.description().isEmpty() )
		{
			// weather condition from Wunderground contains >&deg;<
			// for the degree-character,
			// Utils::LocalizedWeatherString will replace 'C - Celsius' and 
			// 'F - Fahrenheit' to lower capitals
			// so we replace both of them after string has been translated
			// through Utils::LocalizedWeatherString as NASTY WORKAROUND
			sText = Utils::LocalizedWeatherString(weather.description());
			int iPos = 0;

			while( iPos >= 0 )
			{
				iPos = sText.indexOf("&deg;", iPos);
				if( iPos > 0 )
				{
					sText.replace(iPos, 5, QChar(0xB0));

					// Utils::LocalizedWeatherString is adding a space-character
					// between degree-character and the temperature unit sign (C/F)
					// this is the reason, why we replace two characters
					if( sText.mid(iPos+1, 1).compare(" ") == 0 )
						sText.remove(iPos+1, 1);
					
					QString sTemp = sText.mid(iPos+1, 1);
					if( sTemp.compare("f", Qt::CaseSensitive) == 0)
						sText.replace(iPos+1, 1, 'F');
					else if( sTemp.compare("c", Qt::CaseSensitive) == 0 )
						sText.replace(iPos+1, 1, 'C');
				}
				iPos = (iPos+1 < sText.length() && iPos >= 0 ? iPos+1 : -1);
			}
		}
		break;
	}
	return sText;
}

QSizeF
WeatherDataProcessor::Private::doTextLayout( QTextLayout & textLayout, qreal dMaxWidth, qreal dOffset ) const
{
	qreal dHeight(0.0);
	qreal dWidthUsed(0.0);
	qreal dMinWidth( 0.25f * dMaxWidth );

	textLayout.beginLayout();
	while (true)
	{
		QTextLine line = textLayout.createLine();
		if (!line.isValid())
			break;

		line.setLineWidth(dMaxWidth - (textLayout.lineCount()<=1 ? 0.0f : dOffset ));
		line.setPosition(QPointF(0, dHeight));
		dHeight += line.height();
		dWidthUsed = qMax(dWidthUsed, line.naturalTextWidth());

		if( line.naturalTextWidth() > dMaxWidth || line.naturalTextWidth() < dMinWidth )
			break;
	}
	textLayout.endLayout();

	return QSizeF(dWidthUsed, dHeight);
}
