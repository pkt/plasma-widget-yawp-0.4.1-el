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
#include "paneldesktopinterface.h"

//--- QT4 CLASSES ---
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsLinearLayout>


PanelDesktopInterface::PanelDesktopInterface(
	Yawp::ConfigData * configData,
	StateMachine * stateMachine,
	QGraphicsItem * parent,
	Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags)
{
	m_painter = new DesktopPainter(this, configData, stateMachine);
	
	m_busyWidget = new Plasma::BusyWidget();
	m_busyWidget->setAcceptHoverEvents(false);
	m_busyWidget->setAcceptedMouseButtons(Qt::NoButton);

	QGraphicsLinearLayout * mainLayout = new QGraphicsLinearLayout(this);
	mainLayout->addStretch();
	mainLayout->addItem(m_busyWidget);
	mainLayout->addStretch();

	setBusy(false);
}

PanelDesktopInterface::~PanelDesktopInterface()
{
	delete m_painter;
	m_busyWidget->deleteLater();
}

DesktopPainter *
PanelDesktopInterface::desktopPainter() const
{
	return (DesktopPainter *)m_painter;
}

void
PanelDesktopInterface::setSvg(Plasma::Svg * svg)
{
	m_painter->setSvg(svg);
}

void
PanelDesktopInterface::setCustomSvg(Plasma::Svg * svg)
{
	m_painter->setCustomSvg(svg);
}

void
PanelDesktopInterface::setupAnimationTimeLine()
{
	m_painter->setupAnimationTimeLine();
}

void
PanelDesktopInterface::resizeEvent(QGraphicsSceneResizeEvent * event)
{
	QSizeF contentsSize = size();
	QSize painterSize = m_painter->getSize(contentsSize);
	
	QRect painterRect;
	if (painterSize.height() > contentsSize.height())
	{
		qreal scale = contentsSize.height() / painterSize.height();
		painterRect = QRect(0,0, contentsSize.width() * scale, contentsSize.height());
	}
	else
		painterRect = QRect(0,0, painterSize.width(), painterSize.height());
	
	if (painterRect != m_painterRect)
	{
		m_painterRect = painterRect;
		m_painter->update();
	}
	
	event->accept();
}

void
PanelDesktopInterface::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
	const CityWeather * const pCity = m_painter->stateMachine()->currentCity();

	if (!pCity ||
	    m_painter->timeLine()->state() == QTimeLine::Running ||
	    event->button() != Qt::LeftButton )
	{
		return;
	}

	m_painter->mousePressEvent(event);
}

void
PanelDesktopInterface::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
	Q_UNUSED(widget);

	m_painter->paintApplet(painter, option, m_painterRect);
}

void
PanelDesktopInterface::setBusy(bool busy)
{
	m_busyWidget->setRunning(busy);
	m_busyWidget->setVisible(busy);
}
