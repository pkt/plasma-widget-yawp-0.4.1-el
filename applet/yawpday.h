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

#ifndef YAWPDAY_H
#define YAWPDAY_H

#include "yawpdefines.h"

//--- QT4 ---
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QTime>

//--- KDE4 ---
#include <KTimeZone>

//--- std ---
#include <limits.h>


/**  YawpWeather contains all weather informations. Since some weather-provider split a day in day and night.
 *   This way we have to decide once whether we want to use the values for daytime or nighttime and pass that object
 *   to the painter function.
 *   I moved currentIconName and currentTemperature also to the YawpWeather, so we do not have to decide in the
 *   representation-layer, if it is day or night in order to show or hide the current-weather-informations.
 */
class YawpWeather
{
public:
	YawpWeather();
	~YawpWeather();

	const QString & currentIconName() const;
	void setCurrentIconName(const QString & name);

	const QString & iconName() const;
	void setIconName(const QString & name);

	const QString & description() const;
	void setDescription(const QString & desc);

	short windSpeed() const;
	void setWindSpeed(short speed);
	const QString & windDirection() const;
	void setWindDirection(const QString & direction);
	const QString & windShortText() const;
	void setWindShortText(const QString & text);

	short humidity() const;
	void setHumidity(short humidity);

	short currentTemperature() const;
	void setCurrentTemperature(short current);

	short lowTemperature() const;
	void setLowTemperature(short low);

	short highTemperature() const;
	void setHighTemperature(short high);

	short temperatureRealFeelLow() const;
	void setTemperatureRealFeelLow(short low);

	short temperatureRealFeelHigh() const;
	void setTemperatureRealFeelHigh(short high);

	short dewpoint() const;
	void setDewpoint(short dewpoint);

	short pressure() const;
	void setPressure(short pressure);
	const QString & pressureTendency() const;
	void setPressureTendency(const QString & sTendency);
	const QString & pressureShortText() const;
	void setPressureShortText(const QString & text);

	short uvIndex() const;
	void setUVIndex(short uvIndex);
	const QString & uvRating() const;
	void setUVRating(const QString & sUVRating);

	short visibility() const;
	void setVisibility(short visibility);

	bool dayTime() const;
	void setDayTime( bool bDayTime );

	QStringList & propertyTextLines();
	const QStringList & propertyTextLines() const;

	void clear();

private:
	struct Private;
	Private * d;
};


/**  This class holds the forecast information for one day.
 */
class YawpDay
{
public:
	YawpDay();
	~YawpDay();

	const QDate & date() const { return m_date; }
	void setDate(const QDate & date) { m_date = date; }

	const QTime & sunrise() const { return m_sunrise; }
	void setSunrise(const QTime & time) { m_sunrise = time; }

	const QTime & sunset() const { return m_sunset; }
	void setSunset(const QTime & time) { m_sunset = time; }

	bool hasNightValues() const { return m_bHasNightValues; }
	void setHasNightValues(bool value) { m_bHasNightValues = value; }

	YawpWeather & weather() { return m_vWeather[0]; }
	const YawpWeather & weather() const { return m_vWeather[0]; }
	YawpWeather & nightWeather() { return m_vWeather[1]; }
	const YawpWeather & nightWeather() const { return m_vWeather[1]; }

	void clear();

private:
	QDate			m_date;
	QTime			m_sunrise;
	QTime			m_sunset;

	bool			m_bHasNightValues;
	YawpWeather		m_vWeather[2];
};

/**   This class stores all information for one city. It is the city-based-interface
 *    between WeatherServiceModel and Yawp.
 */
class CityWeather
{
public:
	static void debug_PrintCityWeather( const CityWeather & cityInfo );

	CityWeather();
	CityWeather( const CityWeather & cityInfo );

	~CityWeather();

	/**   Copies all attributes, expect the weather data. */
	CityWeather & copy( const CityWeather & other );
	CityWeather & operator= (const CityWeather & cityInfo);

	/**   Compares two cities expect the weather data. */
	bool isEqual( const CityWeather & other ) const;
	bool operator== (const CityWeather & cityInfo) const;

	bool isValid() const;

	/**   Removes all information: city, country,... and removes all days as well. */
	void clear();

	const QString & city() const {return m_sCity;}
	void setCity( const QString & city );

	/** Return city name with localized country name */
	const QString & localizedCityString() const {return m_sLocalizedCity;}

	const QString & country() const {return m_sCountry;}
	void setCountry(const QString & country);

	/**   Set/Get the countrycode for one city
	 *      (e.g.: 'de' for Germany, 'us' for the United States of America)
	 */
	const QString & countryCode() const;
	void setCountryCode(const QString & cc);
	
	/** Set/Get timezone for city
	 *  (e.g.: 'Europe/Berlin')
	 */
	const KTimeZone & timeZone() const;
	bool setTimeZone(const QString & timezone);

	/**   Set/Get the provider, to get the weatherdata from */
	const QString & provider() const {return m_sProvider;}
	void setProvider( const QString & provider ) {m_sProvider = provider;}

	/**   Some providers use a certain code, we need to get the weather-xml. */
	const QString & extraData() const {return m_sExtraData;}
	void setExtraData( const QString & data ) {m_sExtraData = data;}

	/**   This list keeps all forecast information for a city. */
	const QList<YawpDay *> & days() const {return m_vDays;}
	QList<YawpDay *> & days() {return m_vDays;}

	/**   Removes all forecast informations for a city. */
	void deleteAllDays();

	/**  Returns the datetime of the last successfull update. */
	const QDateTime & lastUpdate() const { return m_lastUpdate; }
	void setLastUpdate( const QDateTime & date ) { m_lastUpdate = date; }

	/** Return the observation periode - this is the last time, the weatherstation has update the data */
	const QDateTime & observationPeriode() const { return m_observationPeriode; }
	void setObservationPeriode( const QDateTime & date ) { m_observationPeriode = date; }

	void setSatelliteImage( const QImage & img ) { m_satelliteImage = img; }
	const QImage & satelliteImage() const { return m_satelliteImage; }

	const QString & credit() const { return m_sCredit; }
	void setCredit(const QString & sCredit) { m_sCredit = sCredit; }
	const QString & creditUrl() const { return m_sCreditUrl; }
	void setCreditUrl(const QString & sCreditUrl) { m_sCreditUrl = sCreditUrl; }
	
	/*** Returns the current local time depending on specifed timezone. */
	QDateTime localTime() const;
	
	/*** Converts dateTime from local timezone (specified by CityInfo::timeZone()) to system-timezone. */
	QDateTime fromLocalTime(const QDateTime & dateTime) const;
	
	/*** Converts dateTime from system-timezone to local timezone (specified by CityInfo::timeZone()) */
	QDateTime toLocalTime(const QDateTime & dateTime) const;
	
	/*** Check if last observation time was during daytime or nightime.
	 *  We use oberservation time instead of current time, to show the weather with the "Current Weather Information"
	 *  until we update this information. This way it might happens, that we see the icon for daytime event if it
	 *  is already nightime and vice versa.
	 */
	bool isDayTime( const YawpDay * day ) const;

private:
	void	createLocalizedCityString();

	//--- cityname for visualisation ---
	QString           m_sCity;
	QString           m_sCountry;
	QString           m_sCountryCode;
	QString           m_sProvider;
	QString           m_sLocalizedCity;
	KTimeZone         m_timeZone;

	//--- some provider identify each city through an internal key, so we store this key here ---
	QString           m_sExtraData;

	QList<YawpDay *>  m_vDays;
	QDateTime         m_lastUpdate;
	QDateTime         m_observationPeriode;

	QString           m_sCredit;
	QString           m_sCreditUrl;

	QImage            m_satelliteImage;
};

#endif
