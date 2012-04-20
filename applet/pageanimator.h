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

/*************************************************************************\
*   The PageAnimator is a stripped down version                           *
*   from Bespin's (a QT4/KDE4 Style) Animator/TabInfo class.              *
*                                                                         *
*   Thanks to Thomas Luebking for this algorithms.                        *
*                                                                         *
\*************************************************************************/

/* Bespin widget style for Qt4
Copyright (C) 2007 Thomas Luebking <thomas.luebking@web.de>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License version 2 as published by the Free Software Foundation.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/


#ifndef PAGEANIMATOR_H
#define PAGEANIMATOR_H

//--- QT4 CLASSES ---
#include <QPainter>
#include <QPixmap>

class PageAnimator
{
public:
	enum Transition { Jump = 0, // jumb is default, no animation at all
	                  CrossFade,
	                  ScanlineBlend,
	                  FlipVertically,
	                  FlipHorizontally,
	
			//--- Looks like a camera moves from one page to the other ---
	                  RollInVertically,
	                  RollOutVertically,
	                  RollInHorizontally,
	                  RollOutHorizontally,

			//--- Slides the new page over the old one from top or left---
	                  SlideInVertically,
	                  SlideOutVertically,
	                  SlideInHorizontally,
	                  SlideOutHorizontally,

			//--- Slides the new page over the old one top and bottom or left and right
	                  OpenVertically,
	                  CloseVertically,
	                  OpenHorizontally,
	                  CloseHorizontally,
	                };
	PageAnimator();
	~PageAnimator();

	void resetPages( int width, int height );
	QPixmap vPages[2];

	void paint( QPainter * painter, const QRectF & rect, const int step );
	inline Transition transition() const		{ return m_tTransition; }
	inline void setTransition( const Transition t )	{ m_tTransition = t; }
	inline int duration() const			{ return m_iDuration; }
	inline void setDuration( const uint ms)		{ m_iDuration = ms; }

private:
	Transition	m_tTransition;
	int		m_iDuration;
};

#endif
