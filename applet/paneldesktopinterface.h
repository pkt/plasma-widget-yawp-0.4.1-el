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

#ifndef PANEL_DESKTOP_INTERFACE
#define PANEL_DESKTOP_INTERFACE

//--- LOCAL CLASSES ---
#include "painter/desktoppainter.h"
#include "yawpdefines.h"
#include "statemachine.h"

//--- QT4 CLASSES ---
#include <QGraphicsWidget>

//--- KDE4 CLASSES ---
#include <Plasma/Svg>
#include <Plasma/BusyWidget>
 
class PanelDesktopInterface : public QGraphicsWidget
{
	Q_OBJECT

public:
	PanelDesktopInterface(
		Yawp::ConfigData * configData,
		StateMachine * stateMachine,
		QGraphicsItem * parent = 0,
		Qt::WindowFlags wFlags = 0);
	virtual ~PanelDesktopInterface();

	DesktopPainter * desktopPainter() const;
	void paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
	
	void setSvg (Plasma::Svg * svg);
	void setCustomSvg (Plasma::Svg * svg);
	void setupAnimationTimeLine();
	
public slots:
	void setBusy(bool busy);
	
protected:
      void resizeEvent(QGraphicsSceneResizeEvent * event);
      void mousePressEvent(QGraphicsSceneMouseEvent * event);
	 
private:
	DesktopPainter * m_painter;
	QRect m_painterRect;
	Plasma::BusyWidget * m_busyWidget;
};

#endif // PANEL_DESKTOP_INTERFACE
