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

#ifndef UTILS_H
#define UTILS_H

//--- LOCAL ---
#include "yawpdefines.h"

//--- QT4 ---
class QTime;
class CityWeather;

namespace Utils
{
	void OpenUrl( const QString & url );
	
	/*  Extract the country from the string sLocation and try to find the countrycode
	 *  for this country using UsStatesMap and CountryMap.
	 *  We expect a string like this >City, Country< or >City, Country(County)<
	 */
	bool GetCountryCode( const QString & sCountry, QString & sCountryCode, const Yawp::Storage * pStorage );
	void ExtractLocationInfo( const QString & sLocation,
	                          QString & sCity,
	                          QString & sDistrict,
	                          QString & sCountry );

	/* Normalize weather string, split to parts (semicolon, comma, "and")
	 * and return localized weather description
	 */
	QString LocalizedWeatherString(const QString sWeather );

	QStringList GetTimeZones( const CityWeather & cityInfo, const Yawp::Storage * pStorage );

	inline QString
	GetUnitString( int unit )
	{
#if KDE_IS_VERSION(4,3,70)
		KUnitConversion::Value v(1.0, unit);
		QString str = v.unit()->symbol();
#else
		QString str = WeatherUtils::getUnitString( unit, true );
#endif
/*		switch( unit )
		{
		e.g.:
#if KDE_VERSION_MINOR == 4
			case KUnitConversion::Meter:	str = i18n("m"); break;
#else if KDE_VERSION_MINOR == 3

#else if KDE_VERSION_MINOR == 2

#endif
		default:
			break;
		}
*/
		return str;
	}
};

#endif // UTILS_H
