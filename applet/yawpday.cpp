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

//--- LOCAL CLASSES ---
#include "yawpday.h"
#include "utils.h"

#include "logger/streamlogger.h"

//--- KDE4 CLASSES ---
#include <KLocalizedString>
#include <KSystemTimeZone>


struct YawpWeather::Private
{
	QString     sCurrIconName;
	QString     sIconName;
	QString     sDescription;
	QString     sUrl;

	short       iWindSpeed;
	QString     sWindDirection;
	QString     sWindShortText;
	short       iHumidity;

	short       iCurrentTemperature;
	short       iTemperatureHigh;
	short       iTemperatureLow;
	short       iTemperatureRealFeelHigh;
	short       iTemperatureRealFeelLow;

	short       iDewpoint;

	short       iPressure;
	QString     sPressureTendency;
	QString     sPressureShortText;
	
	short       iUVIndex;
	QString     sUVRating;
	short       iVisibility;

	bool        bDayTime;
	
	QStringList vPropertyLines;
};

YawpWeather::YawpWeather()
	: d( new Private )
{
	clear();
}

YawpWeather::~YawpWeather()
{
	delete d;
}

void
YawpWeather::clear()
{
	d->sCurrIconName.clear();
	d->sIconName                 = QLatin1String("unknown");
	d->sDescription.clear();

	d->iWindSpeed                = SHRT_MAX;
	d->sWindDirection.clear();
	d->sWindShortText.clear();

	d->iHumidity                 = SHRT_MAX;

	d->iCurrentTemperature       = SHRT_MAX;
	d->iTemperatureHigh          = SHRT_MAX;
	d->iTemperatureLow           = SHRT_MAX;
	d->iTemperatureRealFeelHigh  = SHRT_MAX;
	d->iTemperatureRealFeelLow   = SHRT_MAX;

	d->iDewpoint                 = SHRT_MAX;
	d->iPressure                 = SHRT_MAX;
	d->sPressureTendency.clear();
	d->sPressureShortText.clear();
	d->iUVIndex                  = SHRT_MAX;
	d->sUVRating.clear();
	d->iVisibility               = SHRT_MAX;

	d->bDayTime                  = true;

	d->vPropertyLines.clear();
}

const QString &
YawpWeather::currentIconName() const { return d->sCurrIconName; }

void
YawpWeather::setCurrentIconName(const QString & name) { d->sCurrIconName = name.toLower().replace(" ","-"); }

const QString &
YawpWeather::iconName() const { return d->sIconName; }

void
YawpWeather::setIconName(const QString & name) { d->sIconName = name.toLower().replace(" ","-"); }

const QString &
YawpWeather::description() const { return d->sDescription; }

void
YawpWeather::setDescription(const QString & desc) { d->sDescription = desc; }

const QString &
YawpWeather::windShortText() const {return d->sWindShortText;}

void
YawpWeather::setWindShortText(const QString& text) { d->sWindShortText = text; }

short int
YawpWeather::windSpeed() const { return d->iWindSpeed; }

void
YawpWeather::setWindSpeed(short speed) { d->iWindSpeed = speed; }

const QString &
YawpWeather::windDirection() const { return d->sWindDirection; }

void
YawpWeather::setWindDirection(const QString & direction) { d->sWindDirection = direction; }

short
YawpWeather::humidity() const { return d->iHumidity; }

void
YawpWeather::setHumidity(short humidity) { d->iHumidity = humidity; }

short
YawpWeather::currentTemperature() const { return d->iCurrentTemperature; }

void
YawpWeather::setCurrentTemperature(short current) { d->iCurrentTemperature = current; }

short
YawpWeather::lowTemperature() const { return d->iTemperatureLow; }

void
YawpWeather::setLowTemperature(short low) { d->iTemperatureLow = low; }

short
YawpWeather::highTemperature() const { return d->iTemperatureHigh; }

void
YawpWeather::setHighTemperature(short high) { d->iTemperatureHigh = high; }

short
YawpWeather::temperatureRealFeelLow() const { return d->iTemperatureRealFeelLow; }

void
YawpWeather::setTemperatureRealFeelLow(short low) { d->iTemperatureRealFeelLow = low; }

short
YawpWeather::temperatureRealFeelHigh() const { return d->iTemperatureRealFeelHigh; }

void
YawpWeather::setTemperatureRealFeelHigh(short high) { d->iTemperatureRealFeelHigh = high; }

short
YawpWeather::dewpoint() const { return d->iDewpoint; }

void
YawpWeather::setDewpoint(short dewpoint) { d->iDewpoint = dewpoint; }

short
YawpWeather::pressure() const { return d->iPressure; }

void
YawpWeather::setPressure(short pressure) { d->iPressure = pressure; }

const QString &
YawpWeather::pressureTendency() const { return d->sPressureTendency; }

void
YawpWeather::setPressureTendency(const QString & sTendency) { d->sPressureTendency = sTendency; }

const QString &
YawpWeather::pressureShortText() const { return d->sPressureShortText; }

void
YawpWeather::setPressureShortText(const QString& text) { d->sPressureShortText = text; }

short
YawpWeather::uvIndex() const { return d->iUVIndex; }

void
YawpWeather::setUVIndex(short uvIndex) { d->iUVIndex = uvIndex; }

const QString &
YawpWeather::uvRating() const { return d->sUVRating; }

void
YawpWeather::setUVRating(const QString & sUVRating) { d->sUVRating = sUVRating; }

short
YawpWeather::visibility() const { return d->iVisibility; }

void
YawpWeather::setVisibility(short visibility) { d->iVisibility = visibility; }

bool
YawpWeather::dayTime() const { return d->bDayTime; }

void
YawpWeather::setDayTime( bool bDayTime ) { d->bDayTime = bDayTime; }

QStringList &
YawpWeather::propertyTextLines() { return d->vPropertyLines; }

const QStringList &
YawpWeather::propertyTextLines() const { return d->vPropertyLines; }



YawpDay::YawpDay()
{
	clear();
}

YawpDay::~YawpDay()
{
}

void
YawpDay::clear()
{
	m_date			= QDate();
	m_sunrise		= QTime();
	m_sunset		= QTime();

	m_bHasNightValues	= false;
	m_vWeather[0].clear();
	m_vWeather[1].clear();
}

CityWeather::CityWeather()
{
	clear();
}

CityWeather::CityWeather( const CityWeather & other )
{
	*this = other;
}

CityWeather::~CityWeather()
{
	deleteAllDays();
}

void
CityWeather::setCity( const QString & city )
{
	m_sCity = city;
	createLocalizedCityString();
}

void
CityWeather::setCountry(const QString & country)
{
	m_sCountry = country;
	createLocalizedCityString();
}

CityWeather &
CityWeather::copy(const CityWeather & other)
{
	if (this == &other)
		return *this;

	m_sCity           = other.m_sCity;
	m_sCountry        = other.m_sCountry;
	m_sCountryCode    = other.m_sCountryCode;
	m_sProvider       = other.m_sProvider;
	m_sExtraData      = other.m_sExtraData;
	m_sLocalizedCity  = other.m_sLocalizedCity;

	m_lastUpdate      = other.m_lastUpdate;
	m_satelliteImage  = other.m_satelliteImage;
	
	m_timeZone        = other.m_timeZone;
	
	return *this;
}

CityWeather &
CityWeather::operator=(const CityWeather & other)
{
	return copy( other );
}

/*   Some providers do not provide all informations when searching for a city.
 *   Therefore we check on dataupdate if we can add country and countrycode.
 *   This is the reason, why we can not include sCountryCode and sCountry to compare two cities.
 */
bool
CityWeather::isEqual( const CityWeather & other ) const
{
	if( (quintptr)this == (quintptr)(&other) )
		return true;
	if( m_sProvider.compare(other.m_sProvider) != 0 )
		return false;
	if( !m_sExtraData.isEmpty() && !other.m_sExtraData.isEmpty() )
		return (m_sExtraData.compare(other.m_sExtraData) == 0);
	return (m_sCity.compare(other.m_sCity) == 0 && m_sCountry.compare(other.m_sCountry) == 0);
}

bool
CityWeather::operator== (const CityWeather & other) const
{
	return isEqual(other);
}

const QString &
CityWeather::countryCode() const
{
	return m_sCountryCode;
}

void
CityWeather::setCountryCode(const QString & cc)
{
	m_sCountryCode = cc;
}

const KTimeZone &
CityWeather::timeZone() const
{
	return m_timeZone;
}

bool
CityWeather::setTimeZone( const QString & timezone )
{
	if( !m_timeZone.isValid() || m_timeZone.name().compare(timezone, Qt::CaseInsensitive) != 0 )
		m_timeZone = KSystemTimeZones::zone(timezone);
	
	if( m_timeZone.isValid() && !m_timeZone.countryCode().isEmpty() )
		m_sCountryCode = m_timeZone.countryCode().toLower();

	return m_timeZone.isValid();
}


bool
CityWeather::isValid() const
{
	return (!m_sCity.isEmpty() && !m_sProvider.isEmpty());
}

void
CityWeather::clear()
{
	m_sCity.clear();
	m_sCountry.clear();
	m_sCountryCode.clear();
	m_sExtraData.clear();
	m_sProvider.clear();
	m_sLocalizedCity.clear();
	m_lastUpdate = QDateTime();
	m_satelliteImage = QImage();
	deleteAllDays();

	m_sCredit.clear();
	m_sCreditUrl.clear();
	
	m_timeZone = KTimeZone();
}

void
CityWeather::deleteAllDays()
{
	qDeleteAll( m_vDays.begin(), m_vDays.end() );
	m_vDays.clear();
}

void
CityWeather::createLocalizedCityString()
{
	QString sCity, sDistrict, sCountry;
	Utils::ExtractLocationInfo( m_sCity, sCity, sDistrict, sCountry );

	/*** Sometimes a provider returns more information to a city, when we request weather datas.
	*    Therefore the country does not has to be a part of the m_sCity string.
	*/
	if( sCountry.isEmpty() )
		sCountry = m_sCountry;
	else if( !m_sCountry.isEmpty() && sCountry.compare(m_sCountry) != 0 )
	{
		sDistrict = sCountry;
		sCountry = m_sCountry;
	}

	/*** Create the string.
	*/
	m_sLocalizedCity = sCity;
	if( !sCountry.isEmpty() )
		m_sLocalizedCity += ", " + i18nc("Country or state", sCountry.toUtf8().constData());
	if( !sDistrict.isEmpty() )
		m_sLocalizedCity += " (" + sDistrict + ")";
}

QDateTime
CityWeather::localTime() const
{
	return toLocalTime(QDateTime::currentDateTime());
}

QDateTime
CityWeather::toLocalTime(const QDateTime & dateTime) const
{
	KTimeZone local = KSystemTimeZones::local();
	if( m_timeZone.isValid() && local.name() != m_timeZone.name() )
		return KSystemTimeZones::local().convert(m_timeZone, dateTime);
	return dateTime;
}

QDateTime
CityWeather::fromLocalTime(const QDateTime & dateTime) const
{
	KTimeZone local = KSystemTimeZones::local();
	if( m_timeZone.isValid() && local.name() != m_timeZone.name() )
		return m_timeZone.convert(local, dateTime);
	return dateTime;
}

bool
CityWeather::isDayTime( const YawpDay * day ) const
{
	if( !day || !day->sunset().isValid() || !day->sunrise().isValid() )
		return true;

	QDateTime currentTime;
	if( observationPeriode().isValid() )
		currentTime = observationPeriode();
	else
		currentTime = localTime();
	
	QDateTime sunrise( currentTime.date(), day->sunrise() );
	QDateTime sunset( currentTime.date(), day->sunset() );
	
	bool bReturn = (currentTime > sunrise && currentTime  < sunset);

	dTracing() << "City: " << m_sLocalizedCity <<"   current Time:" << currentTime
	           << "   sunrise:" << sunrise << "   sunset:" << sunset
	           << "   daytime:" << bReturn;

	return bReturn;
}

void
CityWeather::debug_PrintCityWeather( const CityWeather & cityInfo )
{
	dDebug() << QString("City <%1> has weatherinfos for %2 day(s)")
		.arg(cityInfo.city()).arg(cityInfo.days().count());

	foreach( const YawpDay * pDay, cityInfo.days() )
	{
		dDebug();
		dDebug() << "Date: " << pDay->date();
		dDebug() << "   date          = " << pDay->date();
		dDebug() << "   sunrise       = " << pDay->sunrise();
		dDebug() << "   sunset        = " << pDay->sunset();
		dDebug();

		int iMax = (pDay->hasNightValues() ? 2 : 1);
		for( int i=0; i<iMax; ++i )
		{
			const YawpWeather & weather = (i == 0 ? pDay->weather() : pDay->nightWeather());
			const QString sPrefix(i==0 ? "" : "night ");

			dDebug() << QString("   %1current temp  = %2")
				.arg(sPrefix).arg(weather.currentTemperature()).toUtf8().constData();
			dDebug() << QString("   %1current icon  = %2")
				.arg(sPrefix).arg(weather.currentIconName()).toUtf8().constData();

			dDebug() << QString("   %1iconName      = %2")
				.arg(sPrefix).arg(weather.iconName()).toUtf8().constData();
			dDebug() << QString("   %1low           = %2")
				.arg(sPrefix).arg(weather.lowTemperature()).toUtf8().constData();
			dDebug() << QString("   %1realfeellow   = %2")
				.arg(sPrefix).arg(weather.temperatureRealFeelLow()).toUtf8().constData();
			dDebug() << QString("   %1high          = %2")
				.arg(sPrefix).arg(weather.highTemperature()).toUtf8().constData();
			dDebug() << QString("   %1realfeelhigh  = %2")
				.arg(sPrefix).arg(weather.temperatureRealFeelHigh()).toUtf8().constData();
			dDebug() << QString("   %1description   = %2")
				.arg(sPrefix).arg(weather.description()).toUtf8().constData();
			dDebug() << QString("   %1humidity      = %2")
				.arg(sPrefix).arg(weather.humidity()).toUtf8().constData();
			dDebug() << QString("   %1windSpeed     = %2")
				.arg(sPrefix).arg(weather.windSpeed()).toUtf8().constData();
			dDebug() << QString("   %1windDirection = %2")
				.arg(sPrefix).arg(weather.windDirection()).toUtf8().constData();

			dDebug() << QString("  %1property text lines = %2")
				.arg(sPrefix).arg(weather.propertyTextLines().count()).toUtf8().constData();
			  
			dDebug();
		}
	}
}
