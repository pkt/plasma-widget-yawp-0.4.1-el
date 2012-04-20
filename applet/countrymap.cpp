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

//--- LOCAL ---
#include "countrymap.h"
#include "logger/streamlogger.h"

//--- QT4 ---
#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QMutex>
#include <QTextStream>
#include <QTime>
#include <QVariant>
#include <QDebug>

//--- KDE4 ---
#include <KGlobal>
#include <KLocale>
#include <KStandardDirs>


//#include <KSystemTimeZones>
//#include <KTimeZone>

const QString CountryMap::sFlagTemplate("l10n/%1/flag.png");

struct CountryInfo
{
	QString      sCountryCode;
	QString      sCountryName;
	QStringList  vTimeZones;
};

class CountryMapLoader
{
public:
	CountryMapLoader( const QString & sResourceFile );
	~CountryMapLoader();
	
	const CountryInfo * getCountryByName(const QString & countryName) const;
	const CountryInfo * getCountryByCode(const QString & countryCode) const;

private:
	// countrycode is the key
	QHash<QString, CountryInfo *> vCountries;
};

CountryMapLoader::CountryMapLoader( const QString & sFile )
{
	dInfo() << "Reading " << sFile;
	QFile resource(sFile);
	if( resource.open( QIODevice::ReadOnly | QIODevice::Text ) )
	{
		QTextStream stream(&resource);
		while( !stream.atEnd() )
		{
			QString sLine = stream.readLine();
			QStringList vTokens = sLine.split(QChar('|'), QString::KeepEmptyParts);
			if( vTokens.count() >= 3 )
			{
				CountryInfo * info = new CountryInfo;
				info->sCountryCode = vTokens.at(0).trimmed();
				info->sCountryName = vTokens.at(1).trimmed();
				
				for( int i = 2; i < vTokens.count(); i++ )
				{
					info->vTimeZones.append(vTokens.at(i).trimmed());
					
/*					// just 4 debug
					KTimeZone zone = KSystemTimeZones::zone(vTokens.at(i).trimmed());
					if( zone.countryCode().compare(info->sCountryCode, Qt::CaseInsensitive) != 0 )
						dWarning() << "CountryCode for " << info->sCountryName
							<< " does not match countrycodes "
							<< info->sCountryCode
							<< zone.countryCode();
*/				}
				vCountries.insert(info->sCountryCode.toLower(), info);
			}
		}
		resource.close();
		dTracing() << "Loaded" << vCountries.count() << "countries for" << sFile;
	}
	else
		dWarning() << "Could not read file" << sFile;

}

CountryMapLoader::~CountryMapLoader()
{
	qDeleteAll( vCountries );
}

const CountryInfo *
CountryMapLoader::getCountryByName(const QString & countryName) const
{
	QHash<QString, CountryInfo *>::const_iterator it = vCountries.constBegin();
	for( ; it != vCountries.constEnd(); ++it )
	{
//		dTracing() << "country code =" << it.key() << "   country =" << it.value()->sCountryName;
		if( it.value()->sCountryName.compare(countryName, Qt::CaseInsensitive) == 0 )
			return it.value();
	}
	return NULL;
}

const CountryInfo *
CountryMapLoader::getCountryByCode(const QString & countryCode) const
{
	return vCountries.value(countryCode.toLower());
}


/******************************************************************************/

struct CountryMap::Private
{
	mutable QCache<QString, QPixmap> * pPixmapCache;	// country code is key
	CountryMapLoader                 * pLoader;
	
	QMutex                             syncMutex;
	
	QPixmap * getPixmapForCountryCode( const QString & countryCode )
	{
		if( countryCode.isEmpty() )
			return NULL;

		QString sCode( countryCode.toLower() );
		QPixmap * pm = NULL;
		pm = pPixmapCache->object(sCode);
		if( !pm )
		{
			QString sFlag( KStandardDirs::locate("locale", sFlagTemplate.arg(sCode)) );
			if( !sFlag.isEmpty() )
			{
				pm = new QPixmap(sFlag);
				pPixmapCache->insert( sCode, pm );
			}
		}
		return pm;
	}
};

CountryMap::CountryMap( QObject * parent )
	: QObject(parent),
	  d(new Private)
{
	d->pPixmapCache = new QCache<QString, QPixmap>(75);
	d->pLoader = new CountryMapLoader( QLatin1String(":/countries.lst") );
}

CountryMap::~CountryMap()
{
	dDebug() << "CountryMap will be removed...";
	delete d->pPixmapCache;
	delete d->pLoader;
	delete d;
}

QPixmap
CountryMap::getPixmapForCountryCode( const QString & countryCode ) const
{
	QMutexLocker locker( &d->syncMutex );
	QPixmap * pm = d->getPixmapForCountryCode( countryCode );
	if( pm )
		return *pm;
	return QPixmap();
}

QString
CountryMap::countryCode( const QString & countryName ) const
{
	const CountryInfo * info = d->pLoader->getCountryByName(countryName);
	if( info )
		return info->sCountryCode;
	return QString();
}

QString
CountryMap::country( const QString & countryCode ) const
{
	const CountryInfo * info = d->pLoader->getCountryByCode(countryCode);
	if( info )
		return info->sCountryName;
	return QString();
}

QStringList
CountryMap::timeZones( const QString & countryCode ) const
{
	const CountryInfo * info = d->pLoader->getCountryByCode(countryCode);
	if( info )
		return info->vTimeZones;
	return QStringList();
}


/******************************************************************************/

struct UnitedStatesMap::Private
{
	CountryMapLoader * pLoader;
};

UnitedStatesMap::UnitedStatesMap( QObject * parent )
	: QObject(parent),
	  d(new Private)
{
	d->pLoader = new CountryMapLoader( QLatin1String(":/us_states.lst") );
}

UnitedStatesMap::~UnitedStatesMap()
{
	dDebug() << "UnitedStatesMap will be removed...";
	delete d->pLoader;
	delete d;
}

QString
UnitedStatesMap::stateCode( const QString & state ) const
{
	const CountryInfo * info = d->pLoader->getCountryByName(state);
	if( info )
		return info->sCountryCode;
	return QString();
}

QString
UnitedStatesMap::state( const QString & stateCode ) const
{
	const CountryInfo * info = d->pLoader->getCountryByCode(stateCode);
	if( info )
		return info->sCountryName;
	return QString();
}

QStringList
UnitedStatesMap::timeZones( const QString & stateCode ) const
{
	const CountryInfo * info = d->pLoader->getCountryByCode(stateCode);
	if( info )
		return info->vTimeZones;
	return QStringList();
}
