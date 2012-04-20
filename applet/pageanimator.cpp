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
*   from Bespin's (a QT4/KDE4 Stype) Animator/TabInfo class.              *
*                                                                         *
*   Thanks to Thomas Luebking for this algorithms                         *
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

#include "pageanimator.h"
#include <cmath>


PageAnimator::PageAnimator()
{
	m_tTransition = ScanlineBlend;
	vPages[0] = vPages[1] = QPixmap();
}

PageAnimator::~PageAnimator()
{
}

void
PageAnimator::resetPages( int width, int height )
{
	vPages[0] = QPixmap(width, height);

	if( !vPages[0].isNull() )
		vPages[0].fill(Qt::transparent);
	vPages[1] = vPages[0];
}

void
PageAnimator::paint( QPainter * painter, const QRectF & rect, const int step )
{
	// Save current state of painter.
	// After painting the animation, we role back all painter settings,
	// we did in this function using QPainter::restore();
	painter->save();

	if( m_tTransition == CrossFade )
	{
		qreal opac = (qreal)step / (qreal)m_iDuration;
		painter->setOpacity( opac );
		painter->drawPixmap( rect.left(), rect.top(), vPages[1] );
		painter->setOpacity( 1.0-opac );
		painter->drawPixmap( rect.left(), rect.top(), vPages[0] );
	}
	else if( m_tTransition == ScanlineBlend )
	{
		const qreal lineHeight = 20.0;
		const int height = qRound(((qreal)(step*lineHeight)) / ((qreal)(m_iDuration)));
		
		for( int posY = 0; posY < vPages[0].height(); posY += (int)lineHeight )
		{
			if (height > 0)
			{
				painter->drawPixmap( rect.left(), rect.top() + posY,
					vPages[1], 0, posY, vPages[1].width(), height);
			}
			if (lineHeight-height > 0)
			{
				painter->drawPixmap( rect.left(), rect.top()+posY+height,
					vPages[0], 0, posY+height, vPages[0].width(), lineHeight-height);
			}
		}
	}
	else if( m_tTransition == FlipVertically )
	{
		
		double dHeight(rect.height());
		double dAnimation = 90.0f - (180.0f*(float)step) / (float)m_iDuration;
		dHeight *= sin( ((double)(qAbs(dAnimation)))*0.01745f ); // PI / 180 = 0.01745
		QRectF targetRect( rect.left(), qRound(rect.top()+(rect.height()-dHeight)/2.0f), rect.width(), dHeight );
	
		if( dAnimation > 0.0 )
			painter->drawPixmap( targetRect, vPages[0], QRectF(0,0,vPages[0].width(), vPages[0].height()) );
		else
			painter->drawPixmap( targetRect, vPages[1], QRectF(0,0,vPages[1].width(), vPages[1].height()) );
	}
	else if( m_tTransition == FlipHorizontally )
	{
		double dWidth(rect.width());
		double dAnimation = 90.0f - (180.0f*(float)step) / (float)m_iDuration;
		dWidth *= sin( ((double)(qAbs(dAnimation)))*0.01745f ); // PI / 180 = 0.01745
		QRectF targetRect( qRound(rect.left()+(rect.width()-dWidth)/2.0f), rect.top(), dWidth, rect.height() );
	
		if( dAnimation > 0.0 )
			painter->drawPixmap( targetRect, vPages[0], QRectF(0,0,vPages[0].width(), vPages[0].height()) );
		else
			painter->drawPixmap( targetRect, vPages[1], QRectF(0,0,vPages[1].width(), vPages[1].height()) );
	}
	else if( m_tTransition == RollInVertically )
	{
		const int h = step*vPages[1].height()/m_iDuration;
		if( h > 0 )
			painter->drawPixmap(rect.left(), rect.top(),
				vPages[1], 0, vPages[1].height() - h, vPages[1].width(), h);
		if( h < vPages[0].height() )
			painter->drawPixmap(rect.left(), rect.top()+h,
				vPages[0], 0, 0, vPages[0].width(), vPages[0].height() - h);
	}
	else if( m_tTransition == RollOutVertically )
	{
		const int h = step*vPages[1].height()/m_iDuration;
		if( h < vPages[0].height() )
			painter->drawPixmap(rect.left(), rect.top(),
				vPages[0], 0, h, vPages[0].width(), vPages[1].height() - h);
		if( h > 0 )
			painter->drawPixmap(rect.left(), rect.top() + vPages[1].height() - h,
				vPages[1], 0, 0, vPages[1].width(), h);
	}
	else if( m_tTransition == RollInHorizontally )
	{
		const int w = (int)qRound( (qreal)(step*vPages[1].width()) / (qreal)m_iDuration );
		if( w > 0 )
			painter->drawPixmap(rect.left(), rect.top(),
				vPages[1], vPages[1].width() - w, 0, w, vPages[1].height());
		if( w < vPages[0].width() )
			painter->drawPixmap(rect.left() + w, rect.top(),
				vPages[0], 0, 0, vPages[0].width() - w, vPages[0].height());
	}
	else if( m_tTransition == RollOutHorizontally )
	{
		int w = (int)qRound( (qreal)(step*vPages[1].width()) / (qreal)m_iDuration );
		if( w < vPages[0].width() )
			painter->drawPixmap(rect.left(), rect.top(),
				vPages[0], w, 0, vPages[0].width()-w, vPages[0].height());
		if( w > 0 )
			painter->drawPixmap(rect.left() + vPages[1].width() - w, rect.top(),
				vPages[1], 0, 0, w, vPages[1].height());
	}
	else if( m_tTransition == SlideInVertically )
	{
		const int h = step*vPages[1].height()/m_iDuration;
		painter->drawPixmap(rect.left(), rect.top(),
			vPages[1], 0, vPages[1].height() - h, vPages[1].width(), h);
		painter->drawPixmap(rect.left(), rect.top() + h,
			vPages[0], 0, h, vPages[0].width(), vPages[0].height() - h);
	}
	else if( m_tTransition == SlideOutVertically )
	{
		const int off = step*vPages[0].height()/m_iDuration;
		painter->drawPixmap(rect.left(), rect.top(),
			vPages[0], 0, off, vPages[0].width(), vPages[0].height() - off);
		painter->drawPixmap(rect.left(), rect.top() + vPages[1].height() - off,
			vPages[1], 0, vPages[1].height() - off, vPages[1].width(), off);
	}
	else if( m_tTransition == SlideInHorizontally )
	{
		const int w = step*vPages[1].width()/m_iDuration;
		painter->drawPixmap(rect.left(), rect.top(),
			vPages[1], vPages[1].width() - w, 0, w, vPages[1].height());
		painter->drawPixmap(rect.left()+w, rect.top(),
			vPages[0], w, 0, vPages[0].width()-w, vPages[0].height());
	}
	else if( m_tTransition == SlideOutHorizontally )
	{
		const int w = vPages[1].width() - step*vPages[1].width()/m_iDuration;
		painter->drawPixmap(rect.left(), rect.top(),
			vPages[0], vPages[0].width() - w, 0, w, vPages[0].height());
		painter->drawPixmap(rect.left()+w, rect.top(),
			vPages[1], w, 0, vPages[1].width()-w, vPages[1].height());
	}
	else if( m_tTransition == CloseVertically )
	{
		int h = step*vPages[1].height()/(2*m_iDuration);
		if( h > 0 )
		{
			painter->drawPixmap(rect.left(), rect.top(),
				vPages[1], 0, vPages[1].height()/2 - h, vPages[1].width(), h);
			painter->drawPixmap(rect.left(), rect.top() + vPages[1].height()-h,
				vPages[1], 0, vPages[1].height()/2, vPages[1].width(), h);
		}
		if( 2*h < vPages[0].height() )
		{
			painter->drawPixmap(rect.left(), rect.top()+h,
				vPages[0], 0, h, vPages[0].width(), vPages[0].height()-2*h);
		}
	}
	else if( m_tTransition == OpenVertically )
	{
		int h = qRound( ( ((qreal)vPages[1].height())
			- ((qreal)(step*vPages[1].height()))/((qreal)(m_iDuration)) ) / 2.0 );
		if( h > 0 )
		{
			painter->drawPixmap(rect.left(), rect.top(),
				vPages[0], 0, vPages[0].height()/2 - h, vPages[0].width(), h);
			painter->drawPixmap(rect.left(), rect.top() + vPages[0].height()-h,
				vPages[0], 0, vPages[0].height()/2, vPages[0].width(), h);
		}
		if( 2*h < vPages[0].height() )
		{
			painter->drawPixmap(rect.left(), rect.top()+h,
				vPages[1], 0, h, vPages[1].width(), vPages[0].height()-2*h);
		}
	}
	else if( m_tTransition == CloseHorizontally )
	{
		int w = step*vPages[1].width()/(2*m_iDuration);
		if( w > 0 )
		{
			painter->drawPixmap(rect.left(), rect.top(),
				vPages[1], vPages[1].width()/2 - w, 0, w, vPages[1].height());
			painter->drawPixmap(rect.left() + vPages[1].width()-w, rect.top(),
				vPages[1], vPages[1].width()/2, 0, w, vPages[1].height());
		}
		if( 2*w < vPages[0].width() )
		{
			painter->drawPixmap(rect.left()+w, rect.top(),
				vPages[0], w, 0, vPages[0].width()-2*w, vPages[0].height());
		}
	}
	else if( m_tTransition == OpenHorizontally )
	{
		int w = (int)qRound( ( ((qreal)vPages[1].width())
			- ((qreal)(step*vPages[1].width()))/((qreal)m_iDuration)) / 2.0);
		if( w > 0 )
		{
			painter->drawPixmap(rect.left(), rect.top(),
				vPages[0], vPages[0].width()/2 - w, 0, w, vPages[0].height());
			painter->drawPixmap(rect.left() + vPages[1].width()-w, rect.top(),
				vPages[0], vPages[0].width()/2, 0, w, vPages[0].height());
		}
		if( 2*w < vPages[0].width() )
		{
			painter->drawPixmap(rect.left()+w, rect.top(),
				vPages[1], w, 0, vPages[1].width()-2*w, vPages[1].height());
		}
	}
	
	//--- restore all painter settings ---
	painter->restore();
}
