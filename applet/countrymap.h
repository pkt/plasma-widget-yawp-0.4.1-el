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

#ifndef COUNTRYMAP_H
#define COUNTRYMAP_H

//--- QT4 ---
#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QCache>
#include <QMap>
#include <QModelIndex>
#include <QPointer>

class CountryMap : public QObject //: public QAbstractListModel
{
public:
	CountryMap( QObject * parent = NULL );
	~CountryMap();

	QPixmap    getPixmapForCountryCode( const QString & countryCode ) const;

	QString    countryCode( const QString & country ) const;
	QString    country( const QString & country ) const;
	
	QStringList timeZones( const QString & countryCode ) const;

private:
	static const QString            sFlagTemplate;

	struct Private;
	Private * d;
};


class UnitedStatesMap : public QObject //: public QAbstractTableModel
{
public:
	UnitedStatesMap( QObject * parent = NULL );
	~UnitedStatesMap();

	QString		stateCode( const QString & state ) const;
	QString		state( const QString & stateCode ) const;
	
	QStringList timeZones( const QString & stateCode ) const;

private:
	struct Private;
	Private * d;
};

#endif // COUNTRYMAP_H
