/*************************************************************************\
*   Copyright (C) 2009 by Marián Kyral                                    *
*   mkyral@email.cz                                                       *
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

#ifndef UNITS_H
#define UNITS_H

//--- KDE4 ---
#include <kdeversion.h>


#if KDE_IS_VERSION(4,3,70)
	#include <KUnitConversion/Converter>
	#define FAHRENHEIT KUnitConversion::Fahrenheit
	#define CELSIUS KUnitConversion::Celsius
	#define MPH KUnitConversion::MilePerHour
	#define KPH KUnitConversion::KilometerPerHour
	#define MILES KUnitConversion::Mile
	#define INCHESHG KUnitConversion::InchesOfMercury
#elif KDE_VERSION_MINOR <= 3
	#include <Plasma/Weather/WeatherUtils>
	#define FAHRENHEIT WeatherUtils::Fahrenheit
	#define CELSIUS WeatherUtils::Celsius
	#if KDE_VERSION_MINOR == 3
		#define MPH WeatherUtils::MilesPerHour
		#define KPH WeatherUtils::KilometersPerHour
	#else
		#define MPH WeatherUtils::MilesAnHour
		#define KPH WeatherUtils::KilometersAnHour
	#endif
	#define MILES WeatherUtils::Miles
	#define INCHESHG WeatherUtils::InchesHG
#endif

#endif
