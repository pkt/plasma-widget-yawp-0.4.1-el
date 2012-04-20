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

#ifndef YAWP_DEFINES_H
#define YAWP_DEFINES_H

//--- LOCAL ---
#include "pageanimator.h"

//--- QT4 ---
#include <QFlags>
#include <QString>

//--- KDE4 ---
#include <kdeversion.h>
#include <Plasma/DataEngine>

#if KDE_IS_VERSION(4,3,70)
	#include <kunitconversion/converter.h>
	#define YAWP_DISTANCE_UNIT    KUnitConversion::UnitId
	#define YAWP_PRESSURE_UNIT    KUnitConversion::UnitId
	#define YAWP_SPEED_UNIT       KUnitConversion::UnitId
	#define YAWP_TEMPERATURE_UNIT KUnitConversion::UnitId
#elif KDE_VERSION_MINOR >= 3
	#include <plasma/weather/weatherutils.h>

	#define YAWP_DISTANCE_UNIT    WeatherUtils::DistanceUnit
	#define YAWP_PRESSURE_UNIT    WeatherUtils::PressureUnit
	#define YAWP_SPEED_UNIT       WeatherUtils::SpeedUnit
	#define YAWP_TEMPERATURE_UNIT WeatherUtils::TemperatureUnit
#else
	#include <plasma/weather/weatherutils.h>

	#define YAWP_DISTANCE_UNIT    WeatherUtils::Unit
	#define YAWP_PRESSURE_UNIT    WeatherUtils::Unit
	#define YAWP_SPEED_UNIT       WeatherUtils::Unit
	#define YAWP_TEMPERATURE_UNIT WeatherUtils::Unit
#endif

class CountryMap;
class UnitedStatesMap;
class IonListModel;

class QWidget;

namespace Plasma
{
	class DataEngine;
}

namespace Yawp
{

/** Stores how we want to display a day in the panel. */
enum PanelDayFormatFlags { PanelTemperature = 1, PanelIcon = 2 };
Q_DECLARE_FLAGS( PanelDayFormat, PanelDayFormatFlags );
Q_DECLARE_OPERATORS_FOR_FLAGS (PanelDayFormat );

/** Pagetype, we use this type in Yawp to store the current display-page and how the panel-tooltip should look like. */
enum PageType { None = 0, PreviewPage = 1, DetailsPage = 2, SatellitePage = 4, ThemeBackground = 8 };
Q_DECLARE_FLAGS( ExtendedTooltipOptions, PageType );
Q_DECLARE_OPERATORS_FOR_FLAGS( ExtendedTooltipOptions );

/** Property that can be displayed on the DetailsPage */
enum DetailsProperty{
	Dewpoint = 1,
	Pressure = 2,
	RealfeelTemperature = 3,
	SunriseSunset = 4,  // exception, because it is not included in YawpWeather but in YawpDay
	UV = 5,             // UV Index and UV Rating
	Visibility = 6,
	WeatherDescription = 7
};


/** This structure holds all config-informations and will be used for synchronising configdialog and yawp.
*/
struct ConfigData
{
	int                        iCityIndex;

	//--- update behaviour options ---
	int                        iUpdateInterval;		// in minutes
	int                        iStartDelay;			// in minutes
	bool                       bTraverseLocationsPeriodically;
	int                        iTraverseLocationTimeout;

	//--- theme options ---
	bool                       bUseCustomTheme;
	bool                       bUseCustomThemeBackground;
	bool                       bUseCustomFontColor;
	bool                       bDisableTextShadows;

	QString                    sBackgroundName;
	QString                    sCustomThemeFile;

	QColor                     fontColor;
	QColor                     lowFontColor;
	QColor                     shadowsFontColor;

	YAWP_DISTANCE_UNIT         distanceSystem;
	YAWP_TEMPERATURE_UNIT      temperatureSystem;
	YAWP_SPEED_UNIT            speedSystem;
	YAWP_PRESSURE_UNIT         pressureSystem;

	//--- panel options ---
	PanelDayFormat             todaysWeatherPanelFormat;
	PanelDayFormat             forecastWeatherPanelFormat;
	int                        iPanelForecastDays;
	bool                       bUseCompactPanelLayout;
	bool                       bUseInteractivePanelWeatherIcons;

	//--- desktop animation options ---
	int                        iAnimationDuration;

	PageAnimator::Transition   daynamesAnimation;
	PageAnimator::Transition   detailsAnimation;
	PageAnimator::Transition   pageAnimation;
	PageAnimator::Transition   iconAnimation;

	//--- Panel Tooltip options ---
	bool                       bUseExtendedTooltip;
	ExtendedTooltipOptions     extendedTooltipOptions;


	/** Contains all property in the right order, the user wants to see on the DetailsPage.
 	 *  The Property at index 0 is the first property on the first page, as long
 	 *  YawpWeather::propertyKeys() also contains this property.
 	 */
	QList<DetailsProperty>     vDetailsPropertyRankingList;
};

class Storage
{
public:
	Storage();
	~Storage();
	
	void                      setEngine(Plasma::DataEngine * pEngine);
	Plasma::DataEngine      * engine() const;

	
	CountryMap              * countryMap() const;
	UnitedStatesMap         * unitedStatesMap() const;
	
	IonListModel            * ionListModel() const;
	
private:
	struct Private;
	Private * d;
};

}	// end of namespace Yawp

#endif
