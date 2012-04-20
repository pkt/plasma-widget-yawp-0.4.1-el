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

//	This Logsystem based on source code of class QDebug - thanks a lot for open source !!!
//

/****************************************************************************
**
** Copyright (C) 1992-2007 Trolltech ASA. All rights reserved.
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.0, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** In addition, as a special exception, Trolltech, as the sole copyright
** holder for Qt Designer, grants users of the Qt/Eclipse Integration
** plug-in the right for the Qt/Eclipse Integration to link to
** functionality provided by Qt Designer and its related libraries.
**
** Trolltech reserves all rights not expressly granted herein.
** 
** Trolltech ASA (c) 2007
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


/*	This is a simple debug change to understand the program flow better.
 *	Everytime a new subfunction will be called, we push it more to the right:
 *	So it might look like this:
 *            13:02::10.880  Info    main (Line 20):          unittest start
 *            13:02::10.880  Info    testfunct1 (Line 22):        subfunction call
 *            13:02::10.881  Info    testfunct1 (Line 23):        the function is doing something...
 *            13:02::10.881  Info    testfunct2 (Line 25):            second subfunction call
 *            13:02::10.881  Info    testfunct2 (Line 26):            the function is doing something...
 *            13:02::10.881  Info    testfunct2 (Line 27):            second subfunction end
 *            13:02::10.881  Info    testfunct1 (Line 29):        subfunction end
 *            13:02::10.881  Info    main (Line 31):          unittest end
 *            ...
 *	TODO:
 *		It might be good idea to save the spacer for all thread independent.
 *		So it could look like this:
 *		...
 *		13:02::10.881  (thread 1)  Info   calculate (Line xx):    is doing something
 *		13:02::10.881  (thread 2)  Info   calculate (Line xx):        is doing something
 *		13:02::10.881  (thread 2)  Info   calculate (Line xx):        is doing something
 *		13:02::10.881  (thread 1)  Info   calculate (Line xx):    is doing something
 *		...
 *		
 *	Additional the msgFileDebug can be used to redirect the debugmessages to file.
 *	Just install this messagehandler like this
 *		qInstallMsgHandler( msgFileDebug );
 */


#ifndef DSTREAMLOGGER_H
#define DSTREAMLOGGER_H

//--- QT4 ---
#include <QCache>
#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QIODevice>
#include <QMutex>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QTime>
#include <QVariant>
#include <QUrl>


class DStreamLogger : private QObject
{
public:
	enum MessageType { Tracing=0, Debug, Info, Warning, Critical, Error };

private:
#ifdef ENABLE_DSTREAMLOGGER
	enum OutputFlags { Unknown, File, StandardOutput, StandardError };
	Q_DECLARE_FLAGS(OutputType, OutputFlags);
	
	struct Stream
	{
		Stream(MessageType msgtype, QIODevice * device)
			: file(),
			  ts(device),
			  msgType(msgtype),
			  bSpace(true),
			  iRef(1),
			  outputType(Unknown),
			  iTabSwitch(0)
		{
		}
		
		Stream(MessageType msgtype, QString * buf)
			: file(),
			  ts(buf, QIODevice::WriteOnly),
			  msgType(msgtype),
			  bSpace(true),
			  iRef(1),
			  outputType(Unknown),
			  iTabSwitch(0)
		{
		}
		
		Stream(MessageType msgtype, OutputType outType)
			: file(),
			  ts(&buffer, QIODevice::WriteOnly),
			  msgType(msgtype),
			  bSpace(true),
			  iRef(1),
			  outputType(outType),
			  iTabSwitch(0)
		{
		}
		
		Stream(MessageType msgtype, const QString & logfile)
			: file(logfile),
			  ts(&buffer, QIODevice::WriteOnly),
			  msgType(msgtype),
			  bSpace(true),
			  iRef(1),
			  outputType(File),
			  iTabSwitch(0)
		{
		}

		void createHeader( const QObject * parent, const char * fileName, int line, const char * funcinfo );

		QFile            file;
		QTextStream      ts;
		MessageType      msgType;
		QString          buffer;
		bool             bSpace;
		int              iRef;
		OutputType       outputType;
		signed short     iTabSwitch;
	};

	struct TabInfo
	{
		TabInfo() : iSpacer(0) {}
		unsigned short   iSpacer;
	};
	
	static Stream * createStream(MessageType type, const QString & logfile);

	void tabRight();
	void tabLeft();

	QMutex   mutex;
	Stream * stream;
	static QCache<quintptr, TabInfo>   m_vTabInfo;

#endif
	static MessageType                 m_logLevel;

public:
	static inline void setLogLevel(MessageType level) {m_logLevel = level;}
	static inline MessageType logLevel() {return m_logLevel;}

	DStreamLogger( MessageType type,
	               const QString & logfile,
	               const char * fileName,
	               int line,
	               const char * funcinfo,
	               signed short = 0 );
	
	DStreamLogger( MessageType type,
	               QIODevice * device,
	               const char * fileName,
	               int line,
	               const char * funcinfo,
	               signed short = 0 );
	
	DStreamLogger( MessageType type,
	               const char * fileName,
	               int line,
	               const char * funcinfo,
	               signed short = 0 );
	
	inline DStreamLogger(const DStreamLogger & o)
		: QObject(NULL)
#ifdef ENABLE_DSTREAMLOGGER
		, stream(NULL)
#endif
	{
#ifdef ENABLE_DSTREAMLOGGER
		QMutexLocker locker(&mutex);
		if( o.stream )
		{
			stream = o.stream;
			stream->iRef += 1;
		}
#else
	Q_UNUSED(o);
#endif
	}

    inline DStreamLogger & operator=(const DStreamLogger & other);
	~DStreamLogger();


	inline DStreamLogger & space()
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->bSpace = true; stream->ts << " ";}
#endif
		return *this;
	}

	inline DStreamLogger & nospace()
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->bSpace = false;}
#endif
		return *this;
	}

	inline DStreamLogger & maybeSpace()
	{
#ifdef ENABLE_DSTREAMLOGGER
		if (stream && stream->bSpace) stream->ts << " ";
#endif
		return *this;
	}

	inline DStreamLogger & operator<<(QChar t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << "\'" << t << "\'";}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(bool t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << (t ? "true" : "false");}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}
		
	inline DStreamLogger & operator<<(char t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(signed short t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}
	
	inline DStreamLogger & operator<<(unsigned short t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}
		
	inline DStreamLogger & operator<<(signed int t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(unsigned int t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(signed long t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(unsigned long t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(qint64 t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(quint64 t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(float t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(double t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}


	inline DStreamLogger & operator<<(const char* t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << QString::fromAscii(t);}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}

	inline DStreamLogger & operator<<(const QString & t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << "\"" << t  << "\"";}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}
		
	inline DStreamLogger & operator<<(const QLatin1String & t)
	{
#ifdef ENABLE_DSTREAMLOGGER		
		if(stream) {stream->ts << "\""  << t.latin1() << "\"";}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}
		
	inline DStreamLogger & operator<<(const QByteArray & t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts  << "\"" << t << "\"";}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}
		
	inline DStreamLogger & operator<<(const void * t)
	{
#ifdef ENABLE_DSTREAMLOGGER
		if(stream) {stream->ts << t;}
#else
		Q_UNUSED(t)
#endif
		return maybeSpace();
	}
};

inline DStreamLogger & DStreamLogger::operator=(const DStreamLogger & other)
{
#ifdef ENABLE_DSTREAMLOGGER
    if (this != &other)
	{
        DStreamLogger copy(other);
        qSwap(stream, copy.stream);
    }
#else
	Q_UNUSED(other)
#endif
	return *this;
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QStringRef & stringRef )
{
#ifdef ENABLE_DSTREAMLOGGER
	return debug << stringRef.toString();
#else
	Q_UNUSED(stringRef)
	return debug.space();
#endif
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QDate & date)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QDate(" << date.toString() << ")";
#else
	Q_UNUSED(date)
#endif
	return debug.space();
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QTime & date)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QTime(" << date.toString() << ")";
#else
	Q_UNUSED(date)
#endif
	return debug.space();
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QDateTime & date)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QDateTime(" << date.toString() << ")";
#else
	Q_UNUSED(date)
#endif
	return debug.space();
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QUrl & url)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QUrl(" << url.toString() << ")";
#else
	Q_UNUSED(url)
#endif
	return debug.space();
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QSize & size)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QSize(" << size.width() << ", " << size.height() << ")";
#else
	Q_UNUSED(size)
#endif
	return debug.space();
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QSizeF & size)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QSizeF(" << size.width() << ", " << size.height() << ")";
#else
	Q_UNUSED(size)
#endif
	return debug.space();
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QPoint & point)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QPoint(" << point.x() << ", " << point.y() << ")";
#else
	Q_UNUSED(point)
#endif
	return debug.space();
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QPointF & point)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QPoint(" << point.x() << ", " << point.y() << ")";
#else
	Q_UNUSED(point)
#endif
	return debug.space();
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QRect & rect)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QRect(" << rect.x() << ", " << rect.y() << ", " << rect.width() << ", " << rect.height() << ")";
#else
	Q_UNUSED(rect)
#endif
	return debug.space();
}

inline DStreamLogger & operator<<(DStreamLogger debug, const QRectF & rect)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QRectF(" << rect.x() << ", " << rect.y() << ", " << rect.width() << ", " << rect.height() << ")";
#else
	Q_UNUSED(rect)
#endif
	return debug.space();
}


#if defined(FORCE_UREF)
template <class T>
inline DStreamLogger & operator<<(DStreamLogger debug, const QList<T> & list)
#else
template <class T>
inline DStreamLogger operator<<(DStreamLogger debug, const QList<T> & list)
#endif
{
#ifdef ENABLE_DSTREAMLOGGER
    debug.nospace() << "(";
    for (Q_TYPENAME QList<T>::size_type i = 0; i < list.count(); ++i) {
        if (i>0)
            debug << ", ";
        debug << list.at(i);
    }
    debug << ")";
#else
	Q_UNUSED(list)
#endif
	return debug.space();
}

#if defined(FORCE_UREF)
template <typename T>
inline DStreamLogger & operator<<(DStreamLogger debug, const QVector<T> & vec)
#else
template <typename T>
inline DStreamLogger operator<<(DStreamLogger debug, const QVector<T> & vec)
#endif
{
    debug.nospace() << "QVector";
    return operator<<(debug, vec.toList());
}

#if defined(FORCE_UREF)
template <class aKey, class aT>
inline DStreamLogger & operator<<(DStreamLogger debug, const QMap<aKey, aT> & map)
#else
template <class aKey, class aT>
inline DStreamLogger operator<<(DStreamLogger debug, const QMap<aKey, aT> & map)
#endif
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QMap(";
	aKey i = 0;
    for (typename QMap<aKey, aT>::const_iterator it = map.constBegin();
         it != map.constEnd(); ++it)
	{
		if (i>0)
			debug << ", \n";
		i+=1;
        debug << "(" << it.key() << ", " << it.value() << ")";
    }
    debug << ")";
#else
	Q_UNUSED(map)
#endif
    return debug.space();
}

#if defined(FORCE_UREF)
template <class aKey, class aT>
inline DStreamLogger & operator<<(DStreamLogger debug, const QHash<aKey, aT> & hash)
#else
template <class aKey, class aT>
inline DStreamLogger operator<<(DStreamLogger debug, const QHash<aKey, aT> & hash)
#endif
{
#ifdef ENABLE_DSTREAMLOGGER
	//-- i think a sorted hash is more helpful, when debuging.
	//-- Even when this produces a higher overhead.
	QList<aKey> vKeys( hash.keys() );
	qSort(vKeys);
	
	debug.nospace() << "QHash(";
	for (Q_TYPENAME QList<aKey>::size_type i = 0; i < vKeys.count(); ++i) {
        if (i>0)
            debug << ", \n";
		aKey k( vKeys.at(i) );
        debug << "(" << k << ", " << hash.value(k) << ")";
	}

	//--- Unsorted hash output ---
/*	debug.nospace() << "QHash(";
    for (typename QHash<aKey, aT>::const_iterator it = hash.constBegin();
            it != hash.constEnd(); ++it)
        debug << "(" << it.key() << ", " << it.value() << ")";
*/
	debug << ")";
#else
	Q_UNUSED(hash)
#endif
	return debug.space();
}

#if defined(FORCE_UREF)
template <class T1, class T2>
inline DStreamLogger & operator<<(DStreamLogger debug, const QPair<T1, T2> & pair)
#else
template <class T1, class T2>
inline DStreamLogger operator<<(DStreamLogger debug, const QPair<T1, T2> & pair)
#endif
{
#ifdef ENABLE_DSTREAMLOGGER
    debug.nospace() << "QPair(" << pair.first << "," << pair.second << ")";
#else
	Q_UNUSED(pair)
#endif
    return debug.space();
}

template <typename T>
inline DStreamLogger operator<<(DStreamLogger debug, const QSet<T> &set)
{
#ifdef ENABLE_DSTREAMLOGGER
	debug.nospace() << "QSet";
    return operator<<(debug, set.toList());
#else
	Q_UNUSED(set)
	return debug;
#endif
}

inline
DStreamLogger operator<<(DStreamLogger debug, const QVariant & v)
{
#ifdef ENABLE_DSTREAMLOGGER
    debug.nospace() << "QVariant(" << v.typeName() << ", ";
	switch (v.type())
	{
		case QVariant::Invalid:    debug << "Invalid"; break;
		case QVariant::Int:        debug << v.toInt(); break;
		case QVariant::UInt:       debug << v.toUInt(); break;
		case QVariant::LongLong:   debug << v.toLongLong(); break;
		case QVariant::ULongLong:  debug << v.toULongLong(); break;
		case QVariant::Double:     debug << v.toDouble(); break;
		case QVariant::Bool:       debug << v.toBool(); break;
		case QVariant::String:     debug << v.toString(); break;
		case QVariant::Char:       debug << v.toChar(); break;
		case QVariant::StringList: debug << v.toStringList(); break;
		case QVariant::Hash:       debug << v.toHash(); break;
		case QVariant::Map:        debug << v.toMap(); break;
		case QVariant::List:       debug << v.toList(); break;
		case QVariant::Date:       debug << v.toDate(); break;
		case QVariant::Time:       debug << v.toTime(); break;
		case QVariant::DateTime:   debug << v.toDateTime(); break;
		case QVariant::ByteArray:  debug << v.toByteArray(); break;
		case QVariant::Url:        debug << v.toUrl(); break;
		case QVariant::Point:      debug << v.toPoint(); break;
		case QVariant::PointF:     debug << v.toPointF(); break;
		case QVariant::Rect:       debug << v.toRect(); break;
		case QVariant::RectF:      debug << v.toRectF(); break;
		case QVariant::Size:       debug << v.toSize(); break;
		case QVariant::SizeF:      debug << v.toSizeF(); break;
		default:
			break;
	}
    debug.nospace() << ')';
#else
    Q_UNUSED(v);
#endif
    return debug.space();
}



/***  THIS IS THE PUBLIC SCOPE OF OPERATION, THAT THIS LOGSYSTEM IS OFFERING FOR DEBUG  ***
 */
#define dTracing()    DStreamLogger(DStreamLogger::Tracing,  __FILE__, __LINE__, Q_FUNC_INFO)
#define dDebug()      DStreamLogger(DStreamLogger::Debug,    __FILE__, __LINE__, Q_FUNC_INFO)
#define dInfo()       DStreamLogger(DStreamLogger::Info,     __FILE__, __LINE__, Q_FUNC_INFO)
#define dWarning()    DStreamLogger(DStreamLogger::Warning,  __FILE__, __LINE__, Q_FUNC_INFO)
#define dCritical()   DStreamLogger(DStreamLogger::Critical, __FILE__, __LINE__, Q_FUNC_INFO)
#define dError()      DStreamLogger(DStreamLogger::Error,    __FILE__, __LINE__, Q_FUNC_INFO)

#define dStartFunct() DStreamLogger(DStreamLogger::Tracing,  __FILE__, __LINE__, Q_FUNC_INFO,  1) << "[function starts]"
#define dEndFunct()   DStreamLogger(DStreamLogger::Tracing,  __FILE__, __LINE__, Q_FUNC_INFO, -1) << "[function ends]"

/*#define D_ASSERT( cond, where, what ) \
	if(!(cond)) {DStreamLogger(DStreamLogger::Error,  LOGOUTPUT, __FILE__, __LINE__, Q_FUNC_INFO).nospace() << "ASSERT failure in " << where << ": " << what; exit(-1);}
*/
#endif // STREAMLOGGER_H
