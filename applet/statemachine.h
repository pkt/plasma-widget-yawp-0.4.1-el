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

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

//--- LOCAL CLASSES ---
#include "yawpdefines.h"

//--- QT4 CLASSES ---
//--- KDE4 CLASSES ---

class CityWeather;
class WeatherServiceModel;
class YawpDay;
class YawpWeather;

class StateMachine
{
public:
	StateMachine();
	~StateMachine();
	
	void setServiceModel( const WeatherServiceModel * pModel );
	const WeatherServiceModel * serviceModel() const;
	
	bool setCurrentCityIndex( int index );
	int currentCityIndex() const;
	const CityWeather * currentCity() const;
	
	bool setCurrentPage( Yawp::PageType page );
	Yawp::PageType currentPage() const;
	bool hasPage( Yawp::PageType ) const;

	bool setDetailsDayIndex( int index );
	int  detailsDayIndex() const;

	/* This function is setting the current property page (sub page that contains the details information in the details page).
	 * If index > maxPropertyPage and bCycle == true, than we set currentPropertyPage = 0 otherwise we will not set a new page index.
	 */
	bool setCurrentPropertyPage( int index, bool bCycle = false );
	int currentPropertyPage() const;
	int maxPropertyPage() const;
	
	bool toggleIconState( int index );
	bool setIconState( int index, bool );
	bool iconState( int index ) const;

	const YawpDay * day( int index ) const;
	const YawpWeather * weather( int index ) const;
	const YawpWeather * weather( int index, bool bState ) const;
	
	/* reset the internal states to default:
	 * The following items will not be changed/reseted:
	 *   - serviceModel()
	 *   - currentCity()
	 *   - currentCityIndex()
	 *  Because than you could not operate on the state machine !!!
	 */
	void reset();

private:
	struct Private;
	Private * d;
};

#endif // STATEMACHINE_H
