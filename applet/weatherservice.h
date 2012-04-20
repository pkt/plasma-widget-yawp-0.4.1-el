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

#ifndef WEATHERSERVICEMODEL_H
#define WEATHERSERVICEMODEL_H

//--- LOCAL ---
#include "yawpday.h"
#include  "yawpdefines.h"

//--- QT4 ---
#include <QAbstractTableModel>
#include <QFile>
#include <QModelIndex>
#include <QObject>
#include <QString>

//--- KDE4 ---
#include <kdeversion.h>
#include <Plasma/DataEngine>

namespace Yawp
{
	class Storage;
}
class WeatherDataProcessor;
class QTimerEvent;

/**   This object manages all cities and interacts with the weatherengine to update the city values.
 *    Furthermore it will be used in the configdialog - Page <Locations>
 *    to show the informations for each configured city.
 */
class WeatherServiceModel : public QAbstractTableModel
{
	 Q_OBJECT
public:

	enum ServiceUpdateFlags { NormalUpdate = 0, TimeoutError = 1, CityInfoUpdate = 2 };
	Q_DECLARE_FLAGS( ServiceUpdate, ServiceUpdateFlags );

	WeatherServiceModel( const Yawp::Storage * pStorage,
	                     QObject  * parent = NULL,
	                     WeatherDataProcessor * pDataProcessor = NULL );
	~WeatherServiceModel();

	/**   Abstract Table Model
	 */
	/** get number of cities */
	int        rowCount ( const QModelIndex & parent = QModelIndex() ) const;
	int        columnCount( const QModelIndex & parent = QModelIndex() ) const;
	QVariant   headerData ( int section,
	                             Qt::Orientation orientation,
	                             int role = Qt::DisplayRole ) const;
	QVariant   data(const QModelIndex &, int) const;

	/**   Remove cities */
	bool       removeRows ( int, int, const QModelIndex & parent = QModelIndex() );

	/**   Add a city to the service model at position index.
	 *    When index < 0, the specific city will be appended at the end.
	 *    Returns the index of the position, where the new city has been inserted
	 *    or -1 when the new city could not be inserted.
	 *    This might happen, when the model alread contains this city.
	 */
	int        addCity( const CityWeather & cityInfo, int index = -1 );

	/**   Move city at position currentIndex to position newIndex.
	 *    Returns true, when city could be successfully moved to the new position.
	 */
	bool       moveCity( int currentIndex, int newIndex );

	/**   Copies the city informations from model other. */
	void       copyCities( const WeatherServiceModel & other );


	/*  Returns the requested city with the up to date weather informations.  */
	CityWeather *	getCityInfo( int index ) const;

	WeatherDataProcessor * dataProcessor() const;
	void setUpdateInterval( int minutes );
	int updateInterval() const;

	/**    Unless a valid WeatherEngine has been set, this function will return false. */
	bool setDetailsPropertyList( const QList<Yawp::DetailsProperty> & vProperties );

public Q_SLOTS:
	void   dataUpdated(const QString & sourceName, const Plasma::DataEngine::Data & data);

	bool   connectEngine();
	bool   disconnectEngine();
	bool   reconnectEngine();

private Q_SLOTS:
	void   slotCheckSourceDates();

Q_SIGNALS:
	void cityUpdated( WeatherServiceModel::ServiceUpdate );
	void   isBusy( bool );

protected:
	void timerEvent( QTimerEvent * e );

private:
	inline void scheduleDelayedUpdate();

	struct Private;
	Private * const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS (WeatherServiceModel::ServiceUpdate);

#endif // WEATHERSERVICEMODEL_H
