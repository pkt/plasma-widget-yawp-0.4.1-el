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

#include "yawpdefines.h"

#include "countrymap.h"
#include "ionlistmodel.h"
#include "logger/streamlogger.h"

#include <QMutex>

#include <Plasma/DataEngine>

struct Yawp::Storage::Private
{
	Plasma::DataEngine       * pEngine;
	mutable CountryMap       * pCountryMap;
	mutable UnitedStatesMap  * pUnitedStatesMap;
	mutable IonListModel     * pIonListModel;
	
	QMutex                     syncMutex;
};

Yawp::Storage::Storage()
	: d( new Yawp::Storage::Private)
{
	d->pCountryMap = NULL;
	d->pUnitedStatesMap = NULL;
	d->pIonListModel = NULL;
	d->pEngine = NULL;
}

Yawp::Storage::~Storage()
{
	if( d->pCountryMap != NULL )
		delete d->pCountryMap;
	if( d->pUnitedStatesMap != NULL )
		delete d->pUnitedStatesMap;
	if( d->pIonListModel != NULL )
		delete d->pIonListModel;
	delete d;
}

void
Yawp::Storage::setEngine(Plasma::DataEngine * pEngine)
{
	d->pEngine = pEngine;
}

Plasma::DataEngine *
Yawp::Storage::engine() const
{
	return d->pEngine;
}

CountryMap *
Yawp::Storage::countryMap() const
{
	QMutexLocker locker(&d->syncMutex);
	if( !d->pCountryMap )
		d->pCountryMap = new CountryMap();
	return d->pCountryMap;
}

UnitedStatesMap *
Yawp::Storage::unitedStatesMap() const
{
	QMutexLocker locker(&d->syncMutex);
	if( !d->pUnitedStatesMap )
		d->pUnitedStatesMap = new UnitedStatesMap();
	return d->pUnitedStatesMap;
}


IonListModel *
Yawp::Storage::ionListModel() const
{
	QMutexLocker locker(&d->syncMutex);
	if( !d->pEngine )
		dError() << "DataEngine has not been set!";
	if( !d->pIonListModel )
		d->pIonListModel = new IonListModel(d->pEngine);
	return d->pIonListModel;
}
