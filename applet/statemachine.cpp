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
#include "statemachine.h"
#include "yawpday.h"
#include "weatherservice.h"
#include "utils.h"

#include "logger/streamlogger.h"

//--- QT4 CLASSES ---
#include <QMutex>

//--- KDE4 CLASSES ---

#include <math.h>


struct StateMachine::Private
{
	QMutex                        mutex;
	const WeatherServiceModel   * pServiceModel;
	int                           iCityIndex;
	const CityWeather           * pCity;
	Yawp::PageType                currentPage;
	int                           iDetailsDayIndex;
	int                           iDetailsPropertyPageIndex;
	
	QList<bool>                   vIconState;
	
	const YawpDay     * getDay( int & index ) const;
	const YawpWeather * getWeather( const YawpDay * pDay, int index, bool bState ) const;
	int                 getMaxPropertyPage( const YawpWeather * pWeather ) const;
};

StateMachine::StateMachine()
	: d( new Private )
{
	d->pServiceModel = NULL;
	d->iCityIndex    = 0;
	d->pCity         = NULL;
	reset();
}

StateMachine::~StateMachine()
{
	delete d;
}

void
StateMachine::setServiceModel( const WeatherServiceModel * pModel )
{
	QMutexLocker locker(&d->mutex);
	d->pServiceModel = pModel;
}

const WeatherServiceModel *
StateMachine::serviceModel() const
{
	return d->pServiceModel;
}

bool
StateMachine::setCurrentCityIndex( int index )
{
	QMutexLocker locker(&d->mutex);
	if( !d->pServiceModel )
		return false;
	bool bReturn( false );
	int iMaxCount( d->pServiceModel->rowCount()-1 );
	int idx( qMin( index, iMaxCount ) );

	if( idx >= 0 )
	{
		d->iCityIndex = idx;
		d->pCity = d->pServiceModel->getCityInfo(idx);
		d->vIconState.clear();
		bReturn = true;
	}
	else
		d->pCity = NULL;
	return bReturn;
}

const CityWeather *
StateMachine::currentCity() const
{
	return d->pCity;
}

int
StateMachine::currentCityIndex() const
{
	return d->iCityIndex;
}

bool
StateMachine::setCurrentPage( Yawp::PageType page )
{
	QMutexLocker locker(&d->mutex);

	bool bReturn(false);
	if( hasPage(page) )
	{
		if( d->currentPage != page && page != Yawp::DetailsPage )
			d->iDetailsPropertyPageIndex = 0;
		d->currentPage = page;
		bReturn = true;
	}
	return bReturn;
}

Yawp::PageType
StateMachine::currentPage() const
{
	if( hasPage( d->currentPage ) )
		return d->currentPage;
	else
		return Yawp::DetailsPage;
}

bool
StateMachine::hasPage( Yawp::PageType page ) const
{
	if( !d->pCity )
		return false;
	switch( page )
	{
	case Yawp::PreviewPage:
		return (d->pCity->days().count() != 1);
		break;
	case Yawp::DetailsPage:
		return true;
		break;
	case Yawp::SatellitePage:
		return  !d->pCity->satelliteImage().isNull();
		break;
	default:
		break;
	}
	return false;
}

bool
StateMachine::setDetailsDayIndex( int index )
{
	QMutexLocker locker(&d->mutex);

	if( !d->pCity )
		return false;

	int iMaxCity( d->pCity->days().count() - 1 );
	index = qMin( index, iMaxCity );
	bool bReturn(false);
	if( index >= 0 )
	{
		d->iDetailsDayIndex = index;
		bReturn = true;
	}
	return bReturn;
}

int
StateMachine::detailsDayIndex() const
{
	if( !d->pCity )
		return -1;
	int iMaxCity( d->pCity->days().count() - 1 );
	return qMin( d->iDetailsDayIndex, iMaxCity );
}

bool
StateMachine::setCurrentPropertyPage( int index, bool bCycle )
{
	int iDayIndex(d->iDetailsDayIndex);
	const YawpDay * pDay = d->getDay(iDayIndex);
	if( !pDay || iDayIndex < 0 )
		return 0;
	const YawpWeather * pWeather = d->getWeather( pDay, iDayIndex, iconState(iDayIndex) );
	int iMaxPage( d->getMaxPropertyPage(pWeather) );
	bool bReturn(true);
	if( index >= 0 && index < iMaxPage )
		d->iDetailsPropertyPageIndex = index;
	else if( bCycle )
		d->iDetailsPropertyPageIndex = 0;
	else
		bReturn = false;
	return bReturn;
}

int
StateMachine::currentPropertyPage() const
{
	int index(d->iDetailsDayIndex);
	const YawpDay * pDay = d->getDay(index);
	if( !pDay || index < 0 )
		return 0;
	const YawpWeather * pWeather = d->getWeather( pDay, index, iconState(index) );
	return qMin(d->iDetailsPropertyPageIndex, d->getMaxPropertyPage(pWeather)-1);	
}

int
StateMachine::maxPropertyPage() const
{
	int index(d->iDetailsDayIndex);
	const YawpDay * pDay = d->getDay(index);
	if( !pDay || index < 0 )
		return 0;
	const YawpWeather * pWeather = d->getWeather( pDay, index, iconState(index) );
	return d->getMaxPropertyPage(pWeather);
}

bool
StateMachine::toggleIconState( int index )
{
	QMutexLocker locker(&d->mutex);

	if( !d->pCity || index >= d->pCity->days().count() )
		return false;
	if( !d->pCity->days().at(index)->hasNightValues() )
		return false;
	while( d->vIconState.count() <= index )
		d->vIconState.append( true );
	d->vIconState[index] = 	!d->vIconState[index];
	return true;
}

bool
StateMachine::setIconState( int index, bool bState )
{
	QMutexLocker locker(&d->mutex);

	if( !d->pCity || index >= d->pCity->days().count() )
		return false;
	if( !d->pCity->days().at(index)->hasNightValues() )
		return false;
	while( d->vIconState.count() <= index )
		d->vIconState.append( true );
	d->vIconState[index] = bState;
	return true;
}

bool
StateMachine::iconState( int index ) const
{
	if( !d->pCity )
		return true;
	index = qMin( index, d->pCity->days().count()-1 );
	if( index < 0 || index >= d->vIconState.count() )
		return true;
	const YawpDay * pDay = d->pCity->days().at(index);
	return (!d->vIconState.at(index) && pDay->hasNightValues() ? false : true);
}

const YawpDay *
StateMachine::day( int index ) const
{
	return d->getDay( index );
}

const YawpWeather *
StateMachine::weather( int index ) const
{
	const YawpDay * pDay = d->getDay( index );
	if( !pDay )
		return NULL;
	return d->getWeather( pDay, index, iconState(index) );
}

const YawpWeather *
StateMachine::weather( int index, bool bState ) const
{
	const YawpDay * pDay = d->getDay( index );
	if( !pDay )
		return NULL;
	return d->getWeather( pDay, index, bState );
}

void
StateMachine::reset()
{
	QMutexLocker locker(&d->mutex);
	
	d->currentPage   = Yawp::PreviewPage;
	d->iDetailsDayIndex = 0;
	d->iDetailsPropertyPageIndex = 0;
	
	d->vIconState.clear();
}


const YawpDay *
StateMachine::Private::getDay( int & index ) const
{
	if( !pCity )
		return NULL;
	index = qMin( index, pCity->days().count()-1 );
	return (index >= 0 ? pCity->days().at(index) : NULL);	
}

const YawpWeather *
StateMachine::Private::getWeather( const YawpDay * pDay, int index, bool bState ) const
{
	if( !this->pCity || !pDay )
		return NULL;
	if( index == 0 )
	{
		if( bState )
			bState = pCity->isDayTime(pDay);
		else
			bState = !pCity->isDayTime(pDay);
	}
	return (!bState && pDay->hasNightValues() ? &pDay->nightWeather() : &pDay->weather());
}

int
StateMachine::Private::getMaxPropertyPage( const YawpWeather * pWeather ) const
{
	if( !pWeather )
		return 0;
	return 1+(int)ceil( (float)pWeather->propertyTextLines().count() / 3.0f );
}
