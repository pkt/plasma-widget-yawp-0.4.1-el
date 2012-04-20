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

#ifndef YAWP_PANEL_PAINTER_H
#define YAWP_PANEL_PAINTER_H

//--- LOCAL CLASSES ---
#include "abstractpainter.h"
#include "desktoppainter.h"
#include "yawpday.h"
#include "yawpdefines.h"

//--- QT4 CLASSES --- 
#include <QGraphicsSceneMouseEvent>
#include <QFrame>
#include <QPointer>

//--- KDE4 CLASSES --- 
#include <Plasma/Applet>

class PanelPainter : public AbstractPainter
{
	Q_OBJECT

public:
	PanelPainter (
		Plasma::Applet * applet,
		const Yawp::ConfigData * configData,
		StateMachine * stateMachine,
		Plasma::FormFactor formFactor);
	virtual ~PanelPainter();
	
	Plasma::FormFactor formFactor();
	QSize getSize(const QSizeF & currentSize) const;
	
	void mousePressEvent (QGraphicsSceneMouseEvent * event);

	// sets the flag that layout has been changed and needs to recalculated
	void update();
	void setupAnimationTimeLine();
	void initCityChangeAnimation(int cityIndex);
	
	DesktopPainter * getPopupPainter() const;
	void setPopupPainter(DesktopPainter * painter);
	
	void setBusy(bool busy);
	
	void initWeatherIconAnimation(int dayIndex);

protected:
	QRect getContentsRect(const QRect & contentsRect) const;
	void handleLayoutChanges();
	void drawPage(QPainter * painter, const QRect & contentsRect) const;
	
private:
	void drawHorizontalApplet(QPainter * painter, const QRect & contentsRect) const;
	void drawVerticalApplet(QPainter * painter, const QRect & contentsRect) const;
	
	QRect getHorizontalTodaysIconRect(const QRect & contentsRect) const;
	QRect getHorizontalTodaysTempRect(const QRect & contentsRect) const;
	QRect getHorizontalForecastIconRect(const QRect & contentsRect, int dayIndex) const;
	QRect getHorizontalForecastTempRect(const QRect & contentsRect, int dayIndex) const;

	QRect getVerticalTodaysIconRect(const QRect & contentsRect) const;
	QRect getVerticalTodaysTempRect(const QRect & contentsRect) const;
	QRect getVerticalForecastIconRect(const QRect & contentsRect, int dayIndex) const;
	QRect getVerticalForecastTempRect(const QRect & contentsRect, int dayIndex) const;

	void drawTemperature(QPainter * painter, const YawpWeather * weather, int alignmentFlag, const QRect & tempRect) const;
	
	void getHorizontalTemperatureSize(
		const int contentsHeight,
		int & todaysTemperatureWidth,
		int & forecastTemperatureWidth) const;
	void getVerticalTemperatureSize(
		const int contentsHeight,
		int & todaysTemperatureWidth,
		int & forecastTemperatureWidth) const;

	int getTemperatureWidth( int pixelSize, const QString & text ) const;

private slots:
	void slotToggleWeatherIcon(int dayIndex);
	void slotChangeCity(int cityIndex);
	
private:
	Plasma::FormFactor m_formFactor;
	
	int m_todaysTempSize;
	int m_forecastTempSize;
	
	QPointer<DesktopPainter> m_popupPainter;
};

#endif // YAWP_PANEL_PAINTER_H
