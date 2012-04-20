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

#include "logger/streamlogger.h"

#include <QCoreApplication>
#include <QtCore>

#include <QtDebug>

QQueue<int> g_queue;
int g_maxlen;
QMutex g_mutex;
QWaitCondition g_queueNotFull;
QWaitCondition g_queueNotEmpty;


class Producer : public QThread
{
public:
	Producer(QObject* parent = 0) : QThread(parent) {}
	
protected:
	void produceMessage()
	{
		dStartFunct() << objectName() << "is producing...";
		QMutexLocker locker(&g_mutex);

		if( g_queue.size() == g_maxlen )
		{
			dInfo() << "g_queue is full, waiting...";
			g_queueNotFull.wait(&g_mutex);
		}
		
		g_queue.enqueue( (rand()%100)+1 );
		g_queueNotEmpty.wakeAll();
		
		dEndFunct();
	}
	
	void run()
	{
		forever
		{
			produceMessage();
			msleep( (rand()%3000)+1 );
		}
	}
};

class Consumer : public QThread
{
public:
	Consumer(QObject* parent = 0) : QThread(parent) {}
	
protected:
	void consumeMessage()
	{
		dStartFunct() << objectName() << "is consuming...";
		QMutexLocker locker(&g_mutex);

		if( g_queue.isEmpty() )
		{
			dWarning() << "g_queue is empty, waiting...";
			g_queueNotFull.wakeAll();
			g_queueNotEmpty.wait(&g_mutex);
		}
		int val = g_queue.dequeue();
		dInfo() << objectName() << "consumed message" << val;
		dEndFunct();
	}

	void run()
	{
		forever
		{
			consumeMessage();
			msleep( (rand()%4000)+1 );
		}
	}
};

void
debugMessages_recursive( int iType, bool bBottomTop )
{
	dStartFunct();
	switch(iType)
	{
		case 0:	dTracing() << "Tracing output...do something"; break;
		case 1:	dDebug() << "Debug output...do something"; break;
		case 2:	dInfo() << "Info output...do something"; break;
		case 3:	dWarning() << "Warning output...do something"; break;
		case 4:	dCritical() << "Critical output...do something"; break;
		case 5:	dError() << "Error output...do something"; break;
	}
	if( iType < 5 && bBottomTop )
		debugMessages_recursive(iType+1, bBottomTop);
	else if( iType > 0 && !bBottomTop )
		debugMessages_recursive(iType-1, bBottomTop);
	dEndFunct();
}

int
main( int argc, char** argv )
{
//	DStreamLogger::setLogLevel( DStreamLogger::Debug );
	dStartFunct();
	debugMessages_recursive(0, true);
	debugMessages_recursive(5, false);
	
	QStringList vLst;
	vLst << "Hello" << "qt" << "developer" << ";-)";
	dInfo() << vLst;
	
	QMap<int, QString> vIntMap;
	vIntMap.insert(1, "one");
	vIntMap.insert(2, "two");
	vIntMap.insert(3, "three");
	vIntMap.insert(4, "four");
	vIntMap.insert(26, "twenty six");
	dWarning() << vIntMap;
	
	QMap<double, QString> vDoubleMap;
	vDoubleMap.insert(1.0, "one");
	vDoubleMap.insert(2.0, "two");
	vDoubleMap.insert(3.0, "three");
	vDoubleMap.insert(4.0, "four");
	vDoubleMap.insert(26.0, "twenty six");
	dInfo() << vDoubleMap;
	
	QHash<QString, QString> vGermanDictionary;
	vGermanDictionary.insert("one", "eins");
	vGermanDictionary.insert("two", "zwei");
	vGermanDictionary.insert("three", "drei");
	vGermanDictionary.insert("four", "vier");
	vGermanDictionary.insert("twenty six", "sechs und zwanzig");
	dInfo() << vGermanDictionary;

	dDebug() << QVariant();
	dDebug() << QVariant((unsigned int)5) << ", " << QVariant((signed int)-5) << ", " << QVariant((short)34);
	dDebug() << QVariant((quint64)4589) << ", " << QVariant((double)75.5);
	dDebug() << QVariant(QString("Test")) << ", " << QVariant((QChar)'c') << ", "  << QVariant(vLst);
	dDebug() << QVariant((bool)true);
	
	dDebug() << QTime::currentTime() << ", " << QVariant(QTime::currentTime());
	dDebug() << QDate::currentDate() << ", " << QVariant(QDate::currentDate());
	dDebug() << QDateTime::currentDateTime() << ", " << QVariant(QDateTime::currentDateTime());
	
	QUrl url("http://yawp.svn.sourceforge.net/viewvc/yawp/trunk/applet/CMakeLists.txt?revision=289&view=markup");
	dDebug() << url << ", " << QVariant(url);

	dDebug() << QSize(123,456) << ", " << QVariant(QSize(123,456));
	dDebug() << QSizeF(123.5,456.5) << ", "<< QVariant(QSizeF(123.5,456.589));
	dDebug() << QPoint(123,456) << ", " << QVariant(QPoint(123,456));
	dDebug() << QPointF(123.5,456.5) << ", "<< QVariant(QPointF(123.5,456.589));
	dDebug() << QRect(123,234,55,66) << ", " << QVariant(QRect(123,234,55,66));
	dDebug() << QRectF(123.1,234.3,55.5,66.8) << ", " << QVariant(QRectF(123.1,234.3,55.5,66.8));
	
	
	/*	QCoreApplication app(argc, argv);
	
	//--- multithreading test ---
	g_maxlen = 10;
	Producer producer;
	Consumer consumer;
	
	producer.setObjectName("Producer");
	consumer.setObjectName("Consumer");

	producer.start();
	consumer.start();

	producer.wait();
	consumer.wait();
*/	
	dEndFunct();
	return 0;
}
