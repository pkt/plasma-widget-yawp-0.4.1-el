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

/* The weatherserviceclass controls all locations and
 * interacts with the dataengine in order to keep all locations up to date.
 */


//--- LOCAL ---
#include "countrymap.h"
#include "ionlistmodel.h"
#include "weatherdataprocessor.h"
#include "weatherservice.h"
#include "utils.h"
#include "logger/streamlogger.h"

//--- QT4 ---
#include <QBasicTimer>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QTimerEvent>
#include <QVariant>

//--- KDE4 ---
#include <KLocale>
#include <Solid/Networking>


#define COL_CITY           0
#define COL_PROVIDER       1
#define COL_UPDATETIME     2
#define COL_TIMEZONE       3
#define COL_PROVIDERCODE   4


struct tCityData
{
	CityWeather   cityInfo;
	bool          bConnected;	// with dataengine connected
};

struct WeatherServiceModel::Private
{
	WeatherServiceModel           * pParent;
	int                             iUpdateInterval;	// in minutes

	QList<tCityData *>              vLocations;

	const Yawp::Storage           * pStorage;
	WeatherDataProcessor          * pDataProcessor;

	QMutex                          updateMutex;

	ServiceUpdate                   updateFlags;
	QBasicTimer                     delayedUpdateEventTimer;
	QBasicTimer                     manualUpdateTimeoutTimer;
	QDateTime                       manualUpdateStartDatetime;

	QDateTime                       lastSourceCheckDatetime;


	QString     getSourceString( const CityWeather & cityInfo ) const;
	void        findCity( const QString & sProvider,
	                      const QString & sCity,
	                      const QString & sData,
	                      int & index,
	                      QList<tCityData *>::iterator & it ) const;
	tCityData * createNewData( const CityWeather & otherCity ) const;
	void        loadCachedValues( CityWeather & cityInfo );
};


WeatherServiceModel::WeatherServiceModel( const Yawp::Storage * pStorage,
                                          QObject * parent,
                                          WeatherDataProcessor * pDataProcessor )
	: QAbstractTableModel( parent ),
	  d( new WeatherServiceModel::Private )
{
	d->pParent = this;
	d->pStorage = pStorage;
	d->pDataProcessor = pDataProcessor;
	d->iUpdateInterval = 0;
}

WeatherServiceModel::~WeatherServiceModel()
{
	dDebug() << "Delete WeatherServiceModel..." << objectName();
	qDeleteAll( d->vLocations.begin(), d->vLocations.end() );
	if( d->pDataProcessor )
		delete d->pDataProcessor;
	delete d;
}

bool
WeatherServiceModel::connectEngine()
{
	QMutexLocker locker( &d->updateMutex );
	if( !d->pDataProcessor || d->iUpdateInterval <= 0 )
		return false;

	dStartFunct();
	d->pDataProcessor->setRequestTimeZone(true);
	Plasma::DataEngine * pEngine = d->pStorage->ionListModel()->engine();

	foreach( tCityData * pData, d->vLocations )
	{
		if( !pData->bConnected )
		{
			QString sSource( d->getSourceString(pData->cityInfo) );
			pEngine->connectSource( sSource, this, d->iUpdateInterval*60*1000 );
			pData->bConnected = true;
		}
	}
	connect( Solid::Networking::notifier(), SIGNAL(shouldConnect()), this, SLOT(slotCheckSourceDates()) );
	dEndFunct();
	return true;
}

bool
WeatherServiceModel::disconnectEngine()
{
	QMutexLocker locker( &d->updateMutex );
	if( !d->pDataProcessor )
		return false;

	dStartFunct();
	disconnect( Solid::Networking::notifier(), SIGNAL(shouldConnect()), this, SLOT(slotCheckSourceDates()) );
	d->delayedUpdateEventTimer.stop();

	Plasma::DataEngine * pEngine = d->pStorage->ionListModel()->engine();
	foreach( tCityData * pData, d->vLocations )
	{
		if( pData->bConnected )
		{
			QString sSource( d->getSourceString(pData->cityInfo) );
			pEngine->disconnectSource( sSource, this );
			pData->bConnected = false;
		}
	}
	dEndFunct();
	return true;
}

bool
WeatherServiceModel::reconnectEngine()
{
	QMutexLocker locker( &d->updateMutex );
	if( !d->pDataProcessor || d->iUpdateInterval <= 0 )
		return false;
	if( d->manualUpdateTimeoutTimer.isActive() )
		return false;

	emit isBusy( true );

	d->manualUpdateTimeoutTimer.start( 35*1000, this );	// timeout after 35 seconds
	d->manualUpdateStartDatetime = QDateTime::currentDateTime();

	QTimer::singleShot( 0,      this, SLOT(disconnectEngine()) );
	QTimer::singleShot( 2*1000, this, SLOT(connectEngine()) );
	return true;
}

int
WeatherServiceModel::rowCount( const QModelIndex & parent ) const
{
	Q_UNUSED(parent)
	return d->vLocations.count();
}

int
WeatherServiceModel::columnCount( const QModelIndex & parent ) const
{
	Q_UNUSED(parent)
	return 5;
}

QVariant
WeatherServiceModel::headerData( int section,
                                 Qt::Orientation orientation,
                                 int role ) const
{
	if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
	{
		QVariant var;
		switch( section )
		{
		case COL_CITY:          var = i18n("Location"); break;
		case COL_PROVIDER:      var = i18n("Provider"); break;
		case COL_UPDATETIME:    var = i18n("Last update"); break;
		case COL_TIMEZONE:      var = i18n("Timezone"); break;
		case COL_PROVIDERCODE:  var = i18n("Provider code"); break;
		}
		return var;
	}
	else
		return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant
WeatherServiceModel::data( const QModelIndex & index, int role ) const
{
	QVariant var;
	if( !index.isValid() || index.row() >= d->vLocations.count() )
		return var;

	const tCityData * pData = d->vLocations.at(index.row());
	const CityWeather * pCity = &pData->cityInfo;

	if( role == Qt::DisplayRole )
	{
		switch( index.column() )
		{
		case COL_CITY:
			var = pCity->localizedCityString();
			break;

		case COL_PROVIDER:
			var = pCity->provider();
			break;

		case COL_UPDATETIME:
			if( pCity->lastUpdate().isValid() )
				var = KGlobal::locale()->formatDateTime( pCity->lastUpdate(), KLocale::FancyShortDate, true );
			else
				var = i18n("no update");
			break;
			
		case COL_TIMEZONE:
			var = (pCity->timeZone().isValid() ? pCity->timeZone().name() : "?");
			break;
			
		case COL_PROVIDERCODE:
			var = pCity->extraData();
			break;
		}
	}
	else if( role == Qt::DecorationRole )
	{
		if( index.column() == COL_CITY && !pCity->countryCode().isEmpty() )
			var = d->pStorage->countryMap()->getPixmapForCountryCode( pCity->countryCode() );
	}
	else if( role == Qt::BackgroundRole )
	{
		if( index.column() == COL_UPDATETIME && pCity->lastUpdate().isValid() )
		{
			//--- weather informations are up to date ---
			if( pCity->lastUpdate().secsTo( QDateTime::currentDateTime() ) < d->iUpdateInterval*60*1000 )
				var = QColor("#FFC0CB");
			else
				var = QColor("#CCFF99");
		}
		else if( index.column() == COL_TIMEZONE )
		{
			var = QColor( pCity->timeZone().isValid() ? "#CCFF99" : "#FFC0CB" );
		}
	}
	return var;
}

bool
WeatherServiceModel::removeRows( int row, int count, const QModelIndex & parent )
{
	Q_UNUSED(parent)
	QMutexLocker locker( &d->updateMutex );
	if( row >= 0 && count > 0 && row+count-1 < d->vLocations.count() )
	{
		QList<tCityData *>::iterator itStart = d->vLocations.begin() + row;
		QList<tCityData *>::iterator itEnd   = d->vLocations.begin() + row + count;
		beginRemoveRows( parent, row, row+count-1 );
		qDeleteAll( itStart, itEnd );
		d->vLocations.erase( itStart, itEnd );
		endRemoveRows();
		return true;
	}
	else
		dDebug() << "Invalid rows to remove...";
	return false;
}

int
WeatherServiceModel::addCity( const CityWeather & cityInfo, int index )
{
	QMutexLocker locker( &d->updateMutex );
	if( !cityInfo.isValid() )
	{
		dDebug() << "Invalid city...";
		return -1;
	}

	foreach( const tCityData * pData, d->vLocations )
	{
		// compare the two cities
		if( pData->cityInfo.isEqual( cityInfo ) )
		{
			dDebug() << "City " << cityInfo.city() << cityInfo.provider() << "already exist.";
			return -1;
		}
	}
	
	tCityData * pData = d->createNewData( cityInfo );
	if( index < 0 || index >= d->vLocations.count() )
		index = d->vLocations.count();

	d->loadCachedValues( pData->cityInfo );

	beginInsertRows( QModelIndex(), index, index );
	d->vLocations.insert( index, pData );
	endInsertRows();
	return index;
}

bool
WeatherServiceModel::moveCity( int currentIndex, int newIndex )
{
	QMutexLocker locker( &d->updateMutex );
	if( currentIndex < 0 || currentIndex >= d->vLocations.count() )
		return false;
	if( newIndex < 0 || newIndex >= d->vLocations.count() )
		newIndex = d->vLocations.count();
	if( currentIndex == newIndex )
		return false;

	beginRemoveRows( QModelIndex(), currentIndex, currentIndex );
	tCityData * pData = d->vLocations.takeAt(currentIndex);
	endRemoveRows();

	beginInsertRows( QModelIndex(), newIndex, newIndex );
	d->vLocations.insert( newIndex, pData );
	endInsertRows();

	return true;
}

void
WeatherServiceModel::copyCities( const WeatherServiceModel & other )
{
	QMutexLocker locker( &d->updateMutex );
	dStartFunct();
	dInfo() << "CopyCities for " << objectName();

	int iIndex = 0;
	const tCityData * pOtherData = NULL;
	tCityData * pData = NULL;

	QList<tCityData *>::const_iterator itOtherData = other.d->vLocations.constBegin();
	QList<tCityData *>::iterator itData = d->vLocations.begin();
	while( itOtherData != other.d->vLocations.constEnd() )
	{
		pOtherData = *itOtherData;
		pData = NULL;

		dTracing() << "Copy: " << pOtherData->cityInfo.localizedCityString() << pOtherData->cityInfo.timeZone().name();
		
		if( itData == d->vLocations.end() )
		{
			pData = d->createNewData( pOtherData->cityInfo );
			pData->cityInfo.setLastUpdate( pOtherData->cityInfo.lastUpdate() );
			dTracing() << "   Create new city";
		}
		else if( !pOtherData->cityInfo.isEqual( (*itData)->cityInfo ) )
		{
			int iResultIndex = iIndex+1;
			QList<tCityData *>::iterator itResult = itData+1;
			const CityWeather * pOtherCity = &pOtherData->cityInfo;

			d->findCity( pOtherCity->provider(), pOtherCity->city(), pOtherCity->extraData(),
			             iResultIndex, itResult );

			if( itResult != d->vLocations.end() )
			{
				// remove city from current position to insert city at another position
				beginRemoveRows( QModelIndex(), iResultIndex, iResultIndex );
				pData = (*itResult);
			
				//--- set timezone ---
				pData->cityInfo.setTimeZone( pOtherData->cityInfo.timeZone().name() );
				d->vLocations.erase(itResult);
				endRemoveRows();
			}
			else
			{
				// create a new city
				pData = d->createNewData( pOtherData->cityInfo );
				pData->cityInfo.setLastUpdate( pOtherData->cityInfo.lastUpdate() );
			}
		}
		else	// city is already on right position
		{
			//--- set timezone ---
			(*itData)->cityInfo.setTimeZone( pOtherData->cityInfo.timeZone().name() );
		}

		//--- insert the city ---
		if( pData )
		{
			if( pData->cityInfo.days().count() == 0 )
				d->loadCachedValues( pData->cityInfo );

			beginInsertRows( QModelIndex(), iIndex, iIndex );
			itData = d->vLocations.insert( itData, pData );
			endInsertRows();
		}

		if( itData != d->vLocations.end() )
			++itData;
		++itOtherData;
		++iIndex;
	}

	/*  If this model contains cities behind the position, this cities are obsoleted and will be deleted.
	 */
	if( itData != d->vLocations.end() )
	{
		beginRemoveRows( QModelIndex(), iIndex, d->vLocations.count()-1 );
		qDeleteAll( itData, d->vLocations.end() );
		d->vLocations.erase( itData, d->vLocations.end() );
		endRemoveRows();
	}
	dEndFunct();
}

/*  Returns the requested city with the up to date weather informations.
 */
CityWeather *
WeatherServiceModel::getCityInfo( int index ) const
{
	CityWeather * pCity = NULL;
	if( index >= 0 && index < d->vLocations.count() )
	{
		pCity = &d->vLocations.at(index)->cityInfo;
	}
	return pCity;
}

WeatherDataProcessor *
WeatherServiceModel::dataProcessor() const
{
	return d->pDataProcessor;
}

void WeatherServiceModel::setUpdateInterval( int minutes ) {d->iUpdateInterval = minutes; }
int WeatherServiceModel::updateInterval() const {return d->iUpdateInterval; }

bool
WeatherServiceModel::setDetailsPropertyList( const QList<Yawp::DetailsProperty> & vProperties )
{
	bool bReturn(false);
	if( d->pDataProcessor )
	{
		d->pDataProcessor->createDetailsPropertyMap( vProperties );
		bReturn = true;
	}
	return bReturn;
}


void
WeatherServiceModel::dataUpdated( const QString & sourceName, const Plasma::DataEngine::Data & data )
{
	if( !d->pDataProcessor )
		return;
	
	dStartFunct() << sourceName << " data received: " << data;

	const QStringList vTokens = sourceName.split("|");
	if( vTokens.count() < 3 || vTokens.at(1).compare("weather") != 0 )
		return;

	QString sExtra;
	if( vTokens.count() >= 4 )
		sExtra = vTokens.at(3);

	int iIndex = 0;
	QList<tCityData *>::iterator it = d->vLocations.begin();
	d->findCity( vTokens.at(0), vTokens.at(2), sExtra, iIndex, it );

	bool bUpdateCityInfo = false;
	if( d->pDataProcessor->updateCountryInfo( (*it)->cityInfo, data ) )
		bUpdateCityInfo = true;
	
	if( it != d->vLocations.end() &&
	    d->pDataProcessor->updateLocation( (*it)->cityInfo, data ) )
	{
		if( bUpdateCityInfo )
			d->updateFlags |= CityInfoUpdate;
		
		d->pDataProcessor->saveData( (*it)->cityInfo, data );
		(*it)->cityInfo.setLastUpdate( QDateTime::currentDateTime() );

		//--- check if all sources has been updated ---
		bool bUpdateFinished(true);
		if( d->manualUpdateTimeoutTimer.isActive() )
		{
			foreach( tCityData * pData, d->vLocations )
			{
				if( pData->cityInfo.lastUpdate() < d->manualUpdateStartDatetime )
				{
					bUpdateFinished = false;
					break;
				}
			}
		}
		if( bUpdateFinished )
			scheduleDelayedUpdate();
	}
	dEndFunct();
}

void
WeatherServiceModel::timerEvent( QTimerEvent * e )
{
	if( e->timerId() == d->delayedUpdateEventTimer.timerId() ||
	    e->timerId() == d->manualUpdateTimeoutTimer.timerId() )
	{
		if( d->manualUpdateTimeoutTimer.isActive() )
			emit isBusy( false );

		d->delayedUpdateEventTimer.stop();
		d->manualUpdateTimeoutTimer.stop();

		emit cityUpdated( d->updateFlags );
		d->updateFlags = NormalUpdate;
	}
	else
		QAbstractTableModel::timerEvent( e );
}

inline void
WeatherServiceModel::scheduleDelayedUpdate()
{
	if( !d->delayedUpdateEventTimer.isActive() )
		d->delayedUpdateEventTimer.start(10, this);
}

void
WeatherServiceModel::slotCheckSourceDates()
{
	if( d->iUpdateInterval <= 0 || d->vLocations.count() == 0 )
		return;
	const QDateTime now( QDateTime::currentDateTime() );
	if( d->lastSourceCheckDatetime.isValid() &&
	    d->lastSourceCheckDatetime.secsTo( now ) <= 5*60*1000 )
	{
		return;
	}

	bool bUpdateNeeded(false);
	foreach( const tCityData * pData, d->vLocations )
	{
		if( !pData->bConnected )
			continue;

		if( pData->cityInfo.lastUpdate().isValid() )
		{
			if( pData->cityInfo.lastUpdate().secsTo( now ) > d->iUpdateInterval*60*1000 )
			{
				bUpdateNeeded = true;
				break;
			}
		}
		else
			bUpdateNeeded = true;
	}

	if( bUpdateNeeded )
		reconnectEngine();
}


/********************************************************************\
*                 WEATHERDATA::PRIVATE MEMBERS                       *
\********************************************************************/

QString
WeatherServiceModel::Private::getSourceString( const CityWeather & cityInfo ) const
{
	if( cityInfo.extraData().isEmpty() )
		return QString("%1|weather|%2")
			.arg(cityInfo.provider()).arg(cityInfo.city());
	else
		return QString("%1|weather|%2|%3")
			.arg(cityInfo.provider()).arg(cityInfo.city()).arg(cityInfo.extraData());
}

void
WeatherServiceModel::Private::findCity( const QString & sProvider,
                                        const QString & sCity,
                                        const QString & sData,
                                        int & index,
                                        QList<tCityData *>::iterator & it ) const
{
	for( ; it != vLocations.end(); ++it, ++index )
	{
		const CityWeather * pCity = &(*it)->cityInfo;
		if( pCity->provider() == sProvider &&
		    pCity->city() == sCity &&
		    pCity->extraData() == sData )
		{
			break;
		}
	}
}

tCityData *
WeatherServiceModel::Private::createNewData( const CityWeather & otherCity ) const
{
	tCityData * pData = new tCityData;
	pData->cityInfo.copy( otherCity );
	pData->bConnected = false;
	return pData;
}

void
WeatherServiceModel::Private::loadCachedValues( CityWeather & cityInfo )
{
	if( pDataProcessor )
	{
		QString sourceString = getSourceString( cityInfo );
		Plasma::DataEngine * pEngine = pStorage->engine();
		Plasma::DataEngine::Data data;
		
		if (pEngine != 0)
			data = pEngine->query( sourceString );

		if (data.isEmpty())
		{
			dTracing() << "Load cached data for" << cityInfo.city();
			pDataProcessor->loadData( cityInfo );
		}
		else
		{
			dTracing() << "Update location for" << cityInfo.city();
			pDataProcessor->updateLocation( cityInfo, data );
		}
	}
}
