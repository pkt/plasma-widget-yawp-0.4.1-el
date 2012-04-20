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

#ifndef ACCUWEATHER_ION_H
#define ACCUWEATHER_ION_H

//--- LOCAL ---
#include "units.h"

//--- QT4 ---
#include <QDate>
#include <QtXml/QXmlStreamReader>

//--- KDE4 ---
#include <KIO/Job>
#include <Plasma/Weather/Ion>


#define MIN_POLL_INTERVAL 3600000L // 1 h 

struct XmlJobData
{
	QXmlStreamReader	xmlReader;
	QString				sLocation;
	QString				sSource;

	//--- just for observation jobs ---
	QString				sLocationCode;
	QByteArray			imageUrl;
};

struct ForecastConditions
{
	QString			weathertext;
	QString			weathericon;
	QString			hightemperature;
	QString			lowtemperature;
	QString			realfeelhigh;
	QString			realfeellow;
	QString			windspeed;
	QString			winddirection;
	QString			windgust;
	QString			uvIndex;
};

struct ForecastDay
{
	QString			obsdate;
	QString			sunrise;
	QString			sunset;

	ForecastConditions	DayTime;
	ForecastConditions	NightTime;
};

struct WeatherData
{
	QString			source;
	QString			locationCode;
	QString			city;
	
	QString			forecastUrl;

	short			iUtcHourOffset;
	short			iUtcMinutesOffset;
	QTime			observationPeriode;
	QString			sLatitude;
	QString			sLongitude;
	
	int				iDistanceSystem;
	int				iPressureSystem;
	int				iSpeedSystem;
	int				iTempSystem;

	QString			temperature;
	QString			realfeel;
	QString			humidity;
	QString			weathertext;
	QString			weathericon;
	QString			windspeed;
	QString			winddirection;
	QString			windgusts;
	QString			windchill;
	QString			visibility;
	QString			dewpoint;
	QString			pressure;
	QString			pressureTendency;
	QString			uvIndex;
	QString			uvRating;

	QList<ForecastDay *>	vForecasts;
};

struct ImageData
{
	QByteArray				fetchedData;
	QByteArray				imageUrl;
	QImage					image;

	/*  Every time, a weather job is ready, we check if the image job is ready as well in order to
	 *  order to update the source:
	 *  ImageJob is ready:	update source immedatly and delete the weatherData in the list
	 *                        since we do not need them anymore
	 *  otherwise:			we add the weatherData to the list vWeatherSources.
	 *                         When imagejob is ready, than we will update all locations in this list,
	 *                         delete the job and keep the source until all weather has been updated.
	 */
	bool					bReady;

	/*  counter of all cities that relay on this image are not ready yet */
	int						iLocationCounter;

	/*  List contains the data of location where the job has been already completed
	 *  and the data are just waiting for the ImageJob to update.
	 */
	QList<WeatherData *>	vWeatherSources;
};


class KDE_EXPORT AccuWeatherIon : public IonInterface
{
	Q_OBJECT

	static const QString IonName;
	static const QString ActionValidate;
	static const QString ActionWeather;

public:
	AccuWeatherIon( QObject * parent, const QVariantList & args = QVariantList() );
	~AccuWeatherIon();

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

	void slotDataArrived( KIO::Job * pJob, const QByteArray & data );
	void slotJobFinished( KJob * pJob );

	void image_slotDataArrived( KIO::Job * job, const QByteArray & data );
	void image_slotJobFinished( KJob * job );

private:
	//--- functions to validate a certain city/location ---
	void findPlace( const QString & place, const QString & source );
	bool readSearchXmlData( const QString & place, const QString & source, QXmlStreamReader & xml );
	void parseSearchLocations( const QString & place, const QString & source, QXmlStreamReader & xml );

	//--- extract all weather values from the xml ---
	void getWeatherXmlData( const QString & city, const QString & locationCode, const QString & source );
	bool readWeatherXmlData( QXmlStreamReader & xml, WeatherData & weather );
	void readUnits( QXmlStreamReader & xml, WeatherData & weather );
	void readLocal( QXmlStreamReader & xml, WeatherData & weather );
	void readCurrentConditions( QXmlStreamReader & xml, WeatherData & weather );
	void readForecastConditions( QXmlStreamReader & xml, ForecastDay & forecastDay );
	void readWeatherConditions( QXmlStreamReader & xml, ForecastConditions & conditions );

	//--- image functions ---
	QByteArray getImageUrl( const QString & sLocationCode ) const;
	void connectWithImageData( const QByteArray & imageUrl );

	//--- update stuff ---
	void updateWeatherSource( const WeatherData & weather, const ImageData * pImageData );
	void updateWeatherCondition( const QString & source,
	                             int iDayIndex,
	                             const QString & dayName,
	                             bool bDayTime,
	                             const ForecastConditions & conditions );
	void updateSun( const QString & source, int iDayIndex, const QString & dayName, const ForecastDay & day );

	inline QString stringConverter( const QString & sValue ) const { return sValue.isEmpty() ? "N/A" : sValue; }
	
	void cleanup();

	struct Private;
	Private * const d;
};

#endif
