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

//	This Logsystem based on source code of QDebug - thanks a lot for open source !!!
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


#include "streamlogger.h"

#include <QCoreApplication>
#include <QIODevice>
#include <QMutex>
#include <QString>
#include <QTextStream>
#include <QTime>

#include <iostream>

#define HEADER_MAXLENGTH        60
#define TABS_WIDTH               3

/*  We do not use threads in yawp, so this might only wast some space in the logmessages.
 *  But this might be helpfull later, or if someone will use it in another application.
 */
#define MULTITHREAD_SUPPORT


#ifdef ENABLE_DSTREAMLOGGER
#include "streamlogger_definitions.h"

DStreamLogger::MessageType DStreamLogger::m_logLevel(DEFAULT_LOGLEVEL);
QCache<quintptr, DStreamLogger::TabInfo> DStreamLogger::m_vTabInfo(100);
#endif

DStreamLogger::DStreamLogger( MessageType type,
                              const QString & logfile,
                              const char * filename,
                              int iLine,
                              const char * funcinfo,
                              signed short iTabSwitch )
	: QObject(NULL)
#ifdef ENABLE_DSTREAMLOGGER
	, stream(NULL)
#endif
{
#ifdef ENABLE_DSTREAMLOGGER
	if (iTabSwitch < 0 && m_logLevel == Tracing)
		tabLeft();
	if (type >= m_logLevel)
	{
		stream = createStream(type, logfile);
		stream->createHeader(this, filename, iLine, funcinfo);
	}
	if (iTabSwitch > 0 && m_logLevel == Tracing)
		  tabRight();
#else
	Q_UNUSED(type)
	Q_UNUSED(logfile)
	Q_UNUSED(filename)
	Q_UNUSED(iLine)
	Q_UNUSED(funcinfo)
	Q_UNUSED(iTabSwitch)
#endif
}

DStreamLogger::DStreamLogger( MessageType type,
                              QIODevice * device,
                              const char * filename,
                              int iLine,
                              const char * funcinfo,
                              signed short iTabSwitch )
	: QObject(NULL)
#ifdef ENABLE_DSTREAMLOGGER
	  , stream(NULL)
#endif
{
#ifdef ENABLE_DSTREAMLOGGER
	if (iTabSwitch < 0 && m_logLevel == Tracing)
		tabLeft();
	if (type >= m_logLevel)
	{
		stream = new Stream(type, device);
		stream->createHeader(this, filename, iLine, funcinfo);
	}
	if (iTabSwitch > 0 && m_logLevel == Tracing)
		tabRight();
#else
	Q_UNUSED(type)
	Q_UNUSED(device)
	Q_UNUSED(filename)
	Q_UNUSED(iLine)
	Q_UNUSED(funcinfo)
	Q_UNUSED(iTabSwitch)
#endif
}

DStreamLogger::DStreamLogger( MessageType type,
                              const char * filename,
                              int iLine,
                              const char * funcinfo,
                              signed short iTabSwitch )
	: QObject(NULL)
#ifdef ENABLE_DSTREAMLOGGER
	  , stream(NULL)
#endif
{
#ifdef ENABLE_DSTREAMLOGGER
	if (iTabSwitch < 0 && m_logLevel == Tracing)
		tabLeft();
	if (type >= m_logLevel)
	{
		stream = createStream(type, LOGOUTPUT);
		stream->createHeader(this, filename, iLine, funcinfo);
	}
	if (iTabSwitch > 0 && m_logLevel == Tracing)
		tabRight();
#else
	Q_UNUSED(type)
	Q_UNUSED(filename)
	Q_UNUSED(iLine)
	Q_UNUSED(funcinfo)
	Q_UNUSED(iTabSwitch)
#endif
}

DStreamLogger::~DStreamLogger()
{
#ifdef ENABLE_DSTREAMLOGGER
	QMutexLocker locker(&mutex);
	
	if( !stream )
		return;
	stream->iRef -= 1;
		
	if( stream->iRef <= 0 )
	{
		if( !stream->file.fileName().isEmpty() && !stream->buffer.isEmpty() )
		{
			stream->ts << "\n";
			if( stream->file.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text) )
			{
				stream->file.write( stream->buffer.toLatin1() );
				stream->file.close();
			}
			else
				std::cout << stream->buffer.toLatin1(). constData();
		}
		else if( stream->outputType == StandardOutput )
			std::cout << stream->buffer.toLatin1(). constData() << std::endl;
		else if( stream->outputType == StandardError )
			std::cerr << stream->buffer.toLatin1(). constData() << std::endl;
		delete stream;
	 }
#endif
}

#ifdef ENABLE_DSTREAMLOGGER
DStreamLogger::Stream *
DStreamLogger::createStream(MessageType type, const QString & logfile)
{
	Stream * stream = 0;
	
	if( logfile.isEmpty() || logfile.toLower().compare(QLatin1String("stdout")) == 0 )
		stream = new Stream(type, StandardOutput);
	else if( logfile.toLower().compare(QLatin1String("stderr")) == 0 )
		stream = new Stream(type, StandardError);
	else
		stream = new Stream(type, logfile);

	return stream;
}

void
DStreamLogger::Stream::createHeader( const QObject * pParent, const char * fileName, int line, const char * funcinfo )
{
	Q_UNUSED(fileName)
	
	ts << QTime::currentTime().toString( QLatin1String("hh:mm:ss.zzz") ) << "  ";
	if( outputType == StandardOutput || outputType == StandardError )	// we write to standard output or standard error
	{
		switch (msgType)
		{
		case Tracing:  ts <<         "Tracing   "; break;
		case Debug:    ts <<         "Debug     "; break;
		case Info:     ts << "\033[36mInfo      \033[m"; break;
		case Warning:  ts << "\033[33mWarning   \033[m"; break;
		case Critical: ts << "\033[35mCritical  \033[m"; break;
		case Error:    ts << "\033[31mError     \033[m"; break;
		}
	}
	else
	{
		switch (msgType)
		{
		case Tracing:  ts << "Tracing    "; break;
		case Debug:	   ts << "Debug      "; break;
		case Info:     ts << "Info       "; break;
		case Warning:  ts << "Warning    "; break;
		case Critical: ts << "Critical   "; break;
		case Error:    ts << "Error      "; break;
		}
	}

	QString sHeader;
#ifdef MULTITHREAD_SUPPORT
	if( QCoreApplication::instance() && pParent->thread() == QCoreApplication::instance()->thread() )
		sHeader += "[Appl. Thread] ";	// Application Thread
	else
		sHeader += QString("[Thread %1] ").arg((quintptr)pParent->thread());
#endif

	// strip the function info down to the base function name
	// note that this throws away the template definitions,
	// the parameter types (overloads) and any const/volatile qualifiers
	QString sFunc;
	if (funcinfo)
	{
# ifdef Q_CC_GNU
		// strip the function info down to the base function name
		// note that this throws away the template definitions,
		// the parameter types (overloads) and any const/volatile qualifiers
		QByteArray info = funcinfo;
		int pos = info.indexOf('(');
		Q_ASSERT_X(pos != -1, "dDebug",
                  "Bug in dDebug(): I don't know how to parse this function name");
		while (info.at(pos - 1) == ' ')
			// that '(' we matched was actually the opening of a function-pointer
			pos = info.indexOf('(', pos + 1);

		info.truncate(pos);
		// gcc 4.1.2 don't put a space between the return type and
		// the function name if the function is in an anonymous namespace
		int index = 1;
		forever {
			index = info.indexOf("<unnamed>::", index);
			if ( index == -1 )
				break;

			if ( info.at(index-1) != ':' )
				info.insert(index, ' ');

			index += strlen("<unnamed>::");
		}
		pos = info.lastIndexOf(' ');
		if (pos != -1)
		{
			int startoftemplate = info.lastIndexOf('<');
			if( startoftemplate != -1 && pos > startoftemplate &&
				pos < info.lastIndexOf(">::") )
					// we matched a space inside this function's template definition
					pos = info.lastIndexOf(' ', startoftemplate);
		}

		if (pos + 1 == info.length())
			// something went wrong, so gracefully bail out
			sFunc = funcinfo;
		else
			sFunc = info.constData() + pos + 1;
#else	// Q_CC_GNU
            sFunc = funcinfo;
#endif	// Q_CC_GNU
	}
	QString sLineNumb;
	if( line >= 0 )
		sLineNumb = QString( " (Line %1): ").arg(line);
	else
		sLineNumb = QLatin1String(": ");
	sHeader += sFunc.left(HEADER_MAXLENGTH - sHeader.length() - sLineNumb.length());
	ts << sHeader << sLineNumb;
	if( sHeader.length()+sLineNumb.length() < HEADER_MAXLENGTH )
		ts << QString(HEADER_MAXLENGTH - (sHeader.length()+sLineNumb.length()), QChar(' '));
	
	TabInfo * pTab = m_vTabInfo.object( (quintptr)pParent->thread() );
	unsigned short iMax = (pTab ? pTab->iSpacer*TABS_WIDTH : 0);
	for( unsigned short i = 0; i < iMax; ++i )
		ts << (i % TABS_WIDTH == 0 ? '.' : ' ');
}

void
DStreamLogger::tabRight()
{
	TabInfo * pTabInfo = m_vTabInfo.object( (quintptr)thread() );
	if( !pTabInfo )
	{
		pTabInfo = new TabInfo;
		m_vTabInfo.insert( (quintptr)thread(), pTabInfo );
	}
	(pTabInfo)->iSpacer++;
}

void
DStreamLogger::tabLeft()
{
	TabInfo * pTabInfo = m_vTabInfo.object( (quintptr)thread() );
	if( pTabInfo )
	{
		if( pTabInfo->iSpacer > 0 )
		  pTabInfo->iSpacer -= 1;
		if( pTabInfo->iSpacer == 0 )
			m_vTabInfo.remove( (quintptr)thread() );
	}
}
#endif	// ENABLE_DSTREAMLOGGER
