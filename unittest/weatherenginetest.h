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

#ifndef WEATHERENGINETEST_H
#define WEATHERENGINETEST_H

#include <QObject>

#include <KIO/NetAccess>
#include <KIO/Job>
#include <Plasma/DataEngine>

class WeatherEngineTest : public QObject
{
	Q_OBJECT
public:
	WeatherEngineTest( QObject * parent = NULL );
	~WeatherEngineTest();

	bool init();
	bool startTransferJob();

public Q_SLOTS:
	void dataUpdated(const QString & location, const Plasma::DataEngine::Data & data );
	void slotData(KIO::Job * job, const QByteArray & data);
	void slotResult( KJob * job );

private:
	void updatePlaces( const QString & location, const QVariant & data );


	Plasma::DataEngine * m_pWeatherEngine;
};

#endif
