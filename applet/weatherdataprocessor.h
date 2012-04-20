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


#ifndef WEATHERDATAPROCESSOR_H
#define WEATHERDATAPROCESSOR_H

//--- LOCAL ---
#include "yawpday.h"
#include "yawpdefines.h"

//--- KDE4 ---
#include <Plasma/DataEngine>


class WeatherDataProcessor
{
public:
	static const QString CacheDirectory;

	WeatherDataProcessor( const Yawp::Storage * pStorage );
	~WeatherDataProcessor();

	bool updateLocation( CityWeather & cityInfo, const Plasma::DataEngine::Data & data ) const;
	bool updateCountryInfo( CityWeather & cityInfo, const Plasma::DataEngine::Data & data ) const;

	bool saveData( const CityWeather & cityInfo, const Plasma::DataEngine::Data & data ) const;
	bool loadData( CityWeather & cityInfo ) const;
	
	void setRequestTimeZone(bool request);
	bool requestTimeZone() const;

	YAWP_DISTANCE_UNIT distanceSystem() const;
	void setDistanceSystem( YAWP_DISTANCE_UNIT unitsystem );

	YAWP_PRESSURE_UNIT pressureSystem() const;
	void setPressureSystem( YAWP_PRESSURE_UNIT unitsystem );

	YAWP_SPEED_UNIT speedSystem() const;
	void  setSpeedSystem( YAWP_SPEED_UNIT unitsystem );

	YAWP_TEMPERATURE_UNIT temperatureSystem() const;
	void  setTemperatureSystem( YAWP_TEMPERATURE_UNIT unitsystem );

	void createDetailsPropertyMap( const QList<Yawp::DetailsProperty> & vProperties );

private:

	bool setForecastValues( YawpWeather                   & weather,
	                        const QStringList             & vForecast,
	                        YAWP_TEMPERATURE_UNIT           fromTempSystem,
	                        YAWP_SPEED_UNIT                 fromSpeedSystem ) const;

	bool setForecastExtraValues( YawpWeather                   & weather,
	                             const QString                 & sValue,
	                             YAWP_TEMPERATURE_UNIT           fromTempSystem,
	                             YAWP_SPEED_UNIT                 fromSpeedSystem ) const;

	bool setForecastSun( YawpDay & day, const QString & sValue ) const;

	inline QString mapConditionIcon( const QString & sIconName ) const;


	struct Private;
	Private * d;
};

#endif // WEATHERDATAPROCESSOR_H
