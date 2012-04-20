/*************************************************************************\
*   Copyright (C) 2011 by Ulf Kreissig                                    *
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

//--- LOCAL CLASSES ---
#include "abstractpainter.h"
#include "yawpday.h"
#include "logger/streamlogger.h"

//--- QT4 CLASSES ---
#include <QTimer>

//--- KDE4 CLASSES ---
#include <Plasma/Theme>

AbstractPainter::AbstractPainter (
	QGraphicsWidget * widget,
	const Yawp::ConfigData * configData,
	StateMachine * stateMachine)
    : m_timeLine(500, this),
      m_bIsBusy(false),
      m_lastFrame(0)
{
	m_widget = widget;
	m_stateMachine = stateMachine;
	m_configData = configData;
	m_svg = m_customSvg = 0;
	
	m_animationType = NoAnimation;
	m_updateRequired = false;
	
	m_appletContent = QPixmap();
	
	connect( &m_timeLine, SIGNAL(frameChanged(int)),  this, SLOT(animationTimeout(int)) );
	connect( &m_timeLine, SIGNAL(finished()),         this, SLOT(animationFinished()) );
}

AbstractPainter::~AbstractPainter()
{
}

bool
AbstractPainter::isBusy() const
{
	return m_bIsBusy;
}

QTimeLine *
AbstractPainter::timeLine() const
{
	return (QTimeLine *)&m_timeLine;
}

QGraphicsWidget *
AbstractPainter::widget() const
{
	return m_widget;
}


void
AbstractPainter::setBusy(bool busy)
{
	if (busy != m_bIsBusy)
	{
		m_bIsBusy = busy;
		update();
	}
}

void
AbstractPainter::setSvg (Plasma::Svg * svg)
{
	if (svg != 0)
	{
		m_svg = svg;
		update();
	}
}

void
AbstractPainter::setCustomSvg (Plasma::Svg * svg)
{
	if (svg != 0)
	{
		m_customSvg = svg;
		update();
	}
}

void
AbstractPainter::update()
{
	m_updateRequired = true;
	
	if (m_widget)
		m_widget->update();
}

void
AbstractPainter::animationTimeout(int frame)
{
	Q_UNUSED(frame);
	
	// QPainter will crash when painting rectangel is invalid or a null-rect (QRect(0,0,0,0)
	if (!m_contentsRect.isValid())
		return;
	
	// During icon animation, we update the pixmap in the middle of animation
	// to change the other values (that depends on the wheather state (daytime / midnight)
	if (m_lastFrame < frame && m_animationType == AbstractPainter::IconAnimation)
	{
		int halfDuration = m_timeLine.duration() / 2;
		if (m_lastFrame < halfDuration && halfDuration < frame)
		{
			updatePixmap(false);
		}
	}
	m_lastFrame = frame;

	m_widget->update();
}

void
AbstractPainter::animationFinished()
{
	m_pageAnimator.vPages[0] = m_pageAnimator.vPages[1] = QPixmap();
	m_animationType = NoAnimation;

	// When animation was a full page animation (e.g.: city changed, day changed, preview page changed to satellite page
	// than we can copy the final page from the page animator otherwise we have to repaint applet content to pixmap
	if (m_animationType == PageChange)
		m_appletContent = m_pageAnimator.vPages[1];
	else
		updatePixmap();
}

void
AbstractPainter::setupAnimationTimeLine()
{
	m_timeLine.stop();
	m_timeLine.setDuration( m_configData->iAnimationDuration );
	m_timeLine.setFrameRange( 0, m_configData->iAnimationDuration );
	m_pageAnimator.setDuration( m_configData->iAnimationDuration );
}


void
AbstractPainter::paintApplet (
	QPainter * painter,
	const QStyleOptionGraphicsItem * option,
	const QRect & contentsRect )
{
	Q_UNUSED(option);

	QRect newContentsRect = getContentsRect(contentsRect);
	if (newContentsRect.isValid())
		m_contentsRect = newContentsRect;

	Plasma::Theme * theme = Plasma::Theme::defaultTheme();
	QFont font = theme->font(Plasma::Theme::DefaultFont);
	painter->setFont(font);

	painter->save();
	painter->translate(0, 0);
	drawBackground(painter, contentsRect);
	painter->restore();

	if( m_animationType == PageChange )
	{	
		m_pageAnimator.paint(painter, m_contentsRect, m_timeLine.currentFrame());
	}
	else
	{
		// When second page is empty or something has been changed,
		// we will paint applet content in second page of page animator
		// Therefore when animation has been finished we are NOT allowed to truncate the second
		// page of pageAnimatior when animation has been finished, because we reuse this page.
		// start a time(0) to paint the backup pixmap - this way we will not change the
		// pixmap during paint-event - which is not allowed nor paint in a pixmap from a background thread
		if (m_updateRequired || m_appletContent.isNull())
			QTimer::singleShot(0, this, SLOT(updatePixmap()));
	
		// we will paint the image we have in cache even if it is old
		// when we would not paint anything, applet content might flicker
		if (!m_appletContent.isNull())
		{
			painter->save();
			painter->translate(m_contentsRect.left(), m_contentsRect.top());
			painter->drawPixmap(0, 0, m_appletContent);
			painter->restore();
		}
		
		if (m_animationType != NoAnimation &&
		    m_animationType != DummyAnimation)
		{
			m_pageAnimator.paint(painter, m_animationRect, m_timeLine.currentFrame());
		}
	}
}

void
AbstractPainter::updatePixmap(bool updateWidget)
{
	if (!m_widget || !m_contentsRect.isValid())
		return;
	
	dWarning() << "Updating applet pixmap";
	
	if (m_updateRequired)
	{
		handleLayoutChanges();
		m_updateRequired = false;
	}      

	m_appletContent = QPixmap(m_contentsRect.width(), m_contentsRect.height());
	m_appletContent.fill(Qt::transparent);
	
	QPainter p( &m_appletContent );
	p.translate( -m_contentsRect.left(), -m_contentsRect.top() );
	drawPage( &p, m_contentsRect );
	
	if (updateWidget)
		m_widget->update();
}

void
AbstractPainter::initCityChangeAnimation(int cityIndex)
{
	initPageChange( AbstractPainter::CityChangeAnimation, cityIndex, configData()->iCityIndex < cityIndex);
}

void
AbstractPainter::initPageChange(
	PageAnimationType pageAnimationType,
	int pageValue,
	bool forward)
{
	dStartFunct();
	
	// check if we have a valid contents rectangle, otherwise do not paint
	bool hasValidContentRect = !m_contentsRect.isNull();

	/***  when the user selected an animation than draw current page
	*/
	if (hasValidContentRect &&
	    m_configData->pageAnimation != PageAnimator::Jump)
	{
		if (!m_updateRequired && m_appletContent.size() == m_contentsRect.size())
		{
			m_pageAnimator.vPages[0] = m_appletContent;
			m_pageAnimator.vPages[1] = QPixmap(m_contentsRect.size());
			m_pageAnimator.vPages[1].fill(Qt::transparent);
		}
		else
		{
			m_pageAnimator.resetPages(m_contentsRect.width(), m_contentsRect.height());
			QPainter painter(&m_pageAnimator.vPages[0]);
			painter.translate(-m_contentsRect.left(), -m_contentsRect.top());
			drawPage(&painter, m_contentsRect);
		}
	}
	
	/*** change the page according to the selectd type
	*/
	switch (pageAnimationType)
	{
	case CityChangeAnimation:
		emit signalCityChanged(pageValue);
		handleLayoutChanges();
		break;
		
	case DayChangeAnimation:
		m_stateMachine->setDetailsDayIndex(pageValue);
		break;
		
	case PageChangeAnimation:
		m_stateMachine->setCurrentPage( (Yawp::PageType)pageValue );
		if (m_stateMachine->currentPage() == Yawp::PreviewPage ||
		    m_stateMachine->currentPage() == Yawp::SatellitePage)
		{
 			m_stateMachine->setDetailsDayIndex(0);
			m_stateMachine->setCurrentPropertyPage(0);
		}
		break;
	}

	if (!hasValidContentRect)
	{
		m_animationType = DummyAnimation;
		m_timeLine.start();
	}
	/***  when the user selected an animation than draw new page and start the page animation
	*/
	else if (m_configData->pageAnimation != PageAnimator::Jump)
	{
		QPainter painter( &m_pageAnimator.vPages[1] );
		painter.translate( -m_contentsRect.left(), -m_contentsRect.top() );
		drawPage(&painter, m_contentsRect);

		//--- setup animation ---
		m_pageAnimator.setTransition( getSlidingTransition(m_configData->pageAnimation, forward) );
		m_animationType = PageChange;
		
		m_timeLine.start();
	}
	else
		//-- When no animation has been configured than just print the new applet state ---
		updatePixmap();
	dEndFunct();
}

void
AbstractPainter::initWeatherIconChange(const QRect & iconRect, int dayIndex, bool currentIcon)
{
	if (m_stateMachine->currentCity() == 0 ||
	    dayIndex >= m_stateMachine->currentCity()->days().count())
	{
		return;
	}
	
	// check if we have a valid contents rectangle, otherwise do not paint
	bool hasValidContentRect = !m_contentsRect.isNull();
	
	if (hasValidContentRect &&
	    m_configData->iconAnimation != PageAnimator::Jump)
	{
  		//--- setup animation ---
		m_animationIndex = dayIndex;
		m_animationType = IconAnimation;
		m_animationRect = iconRect;
		m_pageAnimator.setTransition( m_configData->iconAnimation );

		updatePixmap(false);

		m_pageAnimator.resetPages(iconRect.width(), iconRect.height());
		
		QPainter painter(&m_pageAnimator.vPages[0]);
		painter.translate(-iconRect.left(), -iconRect.top());
		
		const YawpWeather * weather = m_stateMachine->weather(dayIndex);
		drawWeatherIcon(&painter, weather, iconRect, currentIcon);
	}
	
	emit signalToggleWeatherIcon(dayIndex);
	
	if (!hasValidContentRect)
	{
		// When content rect is invalid, just start the timeline to stop
		// user interactions and give calling-object the time to do its
		// animation
		// this will happen when applet is in panel-mode, extender-item has never been shown
		// and panelpainter is triggering this animation.
		m_animationType = DummyAnimation;
		m_timeLine.start();
	}
	else if( m_configData->iconAnimation != PageAnimator::Jump )
	{
		QPainter painter(&m_pageAnimator.vPages[1]);
		painter.translate( -iconRect.left(), -iconRect.top() );
		
		const YawpWeather * weather = m_stateMachine->weather(dayIndex);
		drawWeatherIcon(&painter, weather, iconRect, currentIcon);
		
		m_timeLine.start();
	}
	else
		//-- When no animation has been configured than just print the new applet state ---
		updatePixmap();
}

void
AbstractPainter::drawBackground(QPainter * painter, const QRect & contentsRect) const
{
	Q_UNUSED(painter);
	Q_UNUSED(contentsRect);
}

void
AbstractPainter::drawWeatherIcon(QPainter * painter, const YawpWeather * weather, const QRect & iconRect, bool currentIcon) const
{
	if (!weather)
		return;
	
	//--- GET THE ICON NAME ---
	QString iconName;
	
	if (currentIcon)
		iconName = weather->currentIconName();
	if (iconName.isEmpty() || iconName.compare("unknown") == 0)
		iconName = weather->iconName();

	//--- PAINT THE ICON ---
	painter->save();
	painter->setOpacity( weather->dayTime() ? 1.0 : 0.5 );
	drawImage(painter, iconRect, iconName );
	painter->restore();
}

void
AbstractPainter::drawImage(QPainter * painter, const QRect & rect, const QString & name) const
{
	if (!m_configData)
		return;
	
	if( m_configData->bUseCustomTheme &&
	    m_customSvg != 0 &&
	    m_customSvg->isValid() &&
	    m_customSvg->hasElement(name))
	{
		m_customSvg->paint(painter, rect, name);
	}
	else if (m_svg != 0 &&
	         m_svg->isValid() &&
	         m_svg->hasElement(name) )
	{
		m_svg->paint(painter, rect, name);
	}
	else
	{
		QString backgroundName;
		int lastPos = name.lastIndexOf('-');
		
		if (lastPos > 0)
			backgroundName = name.left(lastPos);
		
		if (!backgroundName.isEmpty() &&
		    m_svg != 0 &&
		    m_svg->isValid() &&
		    m_svg->hasElement(backgroundName))
		{
			m_svg->paint(painter, rect, backgroundName);
		}
	}
}

void
AbstractPainter::drawText(QPainter * painter, const QRect & rect, int align, const QString & text) const
{
	if (! m_configData->bDisableTextShadows)
	{
		float dOffset = 1.0f; // * m_pCurrLayout->getScalingFactor();
		painter->setPen(m_configData->shadowsFontColor);
		painter->drawText(rect.translated(dOffset, dOffset), align, text);
	}
	painter->setPen(m_configData->fontColor);
	painter->drawText(rect, align, text);
}

void
AbstractPainter::drawGreyText(QPainter * painter, const QRect & rect, int align, const QString & text) const
{
	if (! m_configData->bDisableTextShadows)
	{
		float dOffset = 1.0f; //m_pCurrLayout->getScalingFactor();
		painter->setPen(m_configData->shadowsFontColor);
		painter->drawText(rect.translated(dOffset, dOffset), align, text);
	}
	painter->setPen(m_configData->lowFontColor);
	painter->drawText(rect, align, text);
}

/***  if the selected Transition is one of the Rolling, Sliding or Open/Close Effekt,
*     than get the opponet of this transition depending on forward == true/false
*/
PageAnimator::Transition
AbstractPainter::getSlidingTransition(const PageAnimator::Transition tr, const bool forward) const
{
	if( tr <= 4 || forward )
		return tr;
	return (PageAnimator::Transition)(tr % 2 == 1 ? tr+1 : tr-1 );
}
