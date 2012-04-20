/*************************************************************************\
*   Copyright (C) 2009 by                                     *
*                                                             *
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
#include "ionlistmodel.h"
#include "logger/streamlogger.h"

//--- QT4 ---

//--- KDE4 ---
#include <Plasma/DataEngine>


IonListModel::IonListModel( Plasma::DataEngine * engine, QObject * parent )
	: QAbstractListModel( parent ),
	  m_pEngine( engine )
{
	QStringList ionList;
	foreach(const QVariant & item, engine->query("ions"))
	{
		ionList.append( item.toString() );
	}
	ionList.sort();
	foreach(const QString & ionName, ionList)
	{
		QStringList pluginInfo = ionName.split("|");
		if (pluginInfo.count() == 2)
		{
			m_vProviderDisplayNames.append( pluginInfo.at(0) );
			m_vProviderNames.append( pluginInfo.at(1) );
		}
	}
}

IonListModel::~IonListModel()
{
	dDebug() << "IonListModel will be removed...";
}

int
IonListModel::rowCount( const QModelIndex & parent ) const
{
	Q_UNUSED( parent );
	return m_vProviderDisplayNames.count();
}

QVariant
IonListModel::data( const QModelIndex & index, int role ) const
{
	QVariant var;
	if( index.isValid() && index.row() < m_vProviderDisplayNames.count() )
	{
		if( Qt::DisplayRole == role )
			var = m_vProviderDisplayNames.at( index.row() );
		else if( ProviderNameRole == role )
			var = m_vProviderNames.at( index.row() );
	}
	return var;
}

Plasma::DataEngine *
IonListModel::engine() const
{
	return m_pEngine;
}
