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

#ifndef ION_WUNDERGROUND_H
#define ION_WUNDERGROUND_H

//--- LOCAL ---
#include <units.h>

//--- Qt4 ---
#include <QDate>
#include <QtXml/QXmlStreamReader>

//--- KDE4 ---
#include <KIO/Job>

#include <Plasma/Weather/Ion>


#define MIN_POLL_INTERVAL 3600000L // 1 h 

struct XmlWeatherData;
struct XmlLookupResult;
struct ImageData;

class KDE_EXPORT WundergroundIon : public IonInterface
{
	Q_OBJECT

	static const QString IonName;
	static const QString ActionValidate;
	static const QString ActionWeather;
	static const QString GeoLookupXML;
	
	static const QString XmlDataCurrentObservation;
	static const QString XmlDataForecast;
//	static const QString XmlDataAlerts;

public:
	WundergroundIon( QObject * parent = 0, const QVariantList & args = QVariantList() );
	~WundergroundIon();

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
	void setup_slotDataArrived( KIO::Job * job, const QByteArray & data );
	void setup_slotJobFinished( KJob * job );

	void slotDataArrived( KIO::Job * job, const QByteArray & data );
	void slotJobFinished( KJob * job );
	
	void slotImageDataArrived( KIO::Job * job, const QByteArray & data );
	void slotImageJobFinished( KJob * job );

private:
	void setup_findPlace( const QString & sLocation, const QString & sSource, const QString & sPath = QString() );
	bool setup_readLookupData( const QString & sLocation, const QString & sSource, QXmlStreamReader & xml, XmlLookupResult * pResult );

	bool getWeatherData( const QString & sLocation, const QString & sServiceId, const QString & sSource );
	void readCurrentObservation( QXmlStreamReader & xml, XmlWeatherData & data );
	void readWeatherForecast( QXmlStreamReader & xml, XmlWeatherData & data );
	void connectWithImageData(const QUrl & url);
	void updateWeatherSource( const XmlWeatherData & data, const ImageData * imageData);

	void cleanup();
	
	struct Private;
	Private * const d;
};

#endif
