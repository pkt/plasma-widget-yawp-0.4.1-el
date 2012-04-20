/***************************************************************************
 *   Copyright (C) 2007-2008 by Shawn Starr <shawn.starr@rogers.com>       *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

/*************************************************************************\
*   Copyright (C) 2009 by  Ulf Kreißig                                    *
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

#ifndef ION_GOOGLE_H
#define ION_GOOGLE_H

//--- LOCAL ---
#include <units.h>

//--- QT4 ---
#include <QDate>
#include <QtXml/QXmlStreamReader>

//--- KDE4 ---
#include <KIO/Job>
#include <Plasma/Weather/Ion>


#define MIN_POLL_INTERVAL 3600000L // 1 h 

struct XmlServiceData;
struct XmlWeatherData;


class KDE_EXPORT GoogleIon : public IonInterface
{
	Q_OBJECT

	static const QString IonName;
	static const QString ActionValidate;
	static const QString ActionWeather;

public:
	GoogleIon( QObject * parent = 0, const QVariantList & args = QVariantList() );
	~GoogleIon();

	/**
	* This method is called when the DataEngine is started. When this
	* method is called the DataEngine is fully constructed and ready to be
	* used. This method should be reimplemented by DataEngine subclasses
	* which have the need to perform a startup routine.
	**/
	void init();

	/**
	* Reimplement to fetch the data from the ion.
	* @arg source the name of the datasource.
	* @return true if update was successful, false if failed
	*/
	bool updateIonSource( const QString & );

public slots:
	 virtual void reset();

private slots:
	void setup_slotJobFinished( KJob * job );

	void slotDataArrived( KIO::Job * job, const QByteArray & data );
	void slotJobFinished( KJob * job );

private:
	//--- functions to validate a certain city/location ---
	void findPlace( const QString & place, const QString & source );
	
	//--- extract all weather values from the xml ---
	void getWeatherData( const QString & place, const QString & source );
	void updateWeatherSource( const XmlWeatherData & data, const QString & sSource, const QString & sLocation );
	
	QString getIconName( const QString & sNodeValue );
	
	void cleanup();

	struct Private;
	Private * const d;
};

#endif	// ION_GOOGLE_H
