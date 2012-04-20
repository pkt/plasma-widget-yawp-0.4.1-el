/*************************************************************************\
*   Copyright (C) 2009 by Ulf Kreißig                                     *
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

#ifndef IONLISTMODEL_H
#define IONLISTMODEL_H

//--- LOCAL ---

//--- QT4 ---
#include <QAbstractListModel>
#include <QPointer>
#include <QMutex>
#include <QStringList>

//--- KDE4 ---
namespace Plasma
{
	class DataEngine;
}


class IonListModel : public QAbstractListModel
{
public:
	enum IonModelRole {ProviderNameRole = Qt::UserRole};

	IonListModel( Plasma::DataEngine * engine, QObject * parent = NULL );
	~IonListModel();

	int rowCount( const QModelIndex & parent = QModelIndex() ) const;
	QVariant data( const QModelIndex & index, int role ) const;
	Plasma::DataEngine * engine() const;

private:
	QStringList                m_vProviderDisplayNames;
	QStringList                m_vProviderNames;	// to use when we request information from this ion
	Plasma::DataEngine       * m_pEngine;
};

#endif
