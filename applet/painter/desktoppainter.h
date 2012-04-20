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

#ifndef YAWP_DESKTOP_PAINTER_H
#define YAWP_DESKTOP_PAINTER_H

//--- LOCAL CLASSES ---
#include "abstractpainter.h"
#include "yawpday.h"
#include "yawpdefines.h"

//--- QT4 CLASSES ---
#include <QPixmap>

class DesktopPainter : public AbstractPainter
{
public:
	DesktopPainter (
		QGraphicsWidget * widget,
		const Yawp::ConfigData * configData,
		StateMachine * stateMachine);
	virtual ~DesktopPainter();
	
	Plasma::FormFactor formFactor();
	QSize getSize(const QSizeF & currentSize) const;
	
	void mousePressEvent (QGraphicsSceneMouseEvent * event);
	
	void initWeatherIconAnimation(int dayIndex);
	
	QPixmap createSimpleToolTip(const QSize & size) const;
	QPixmap createExtendedToolTip(const QSize & size) const;

protected:
	void handleLayoutChanges();

	void drawPage(QPainter * painter, const QRect & contentsRect) const;
	void drawBackground (QPainter * painter, const QRect & contentsRect) const;

private:
	void drawPlainPage (QPainter * painter, const QRect & contentsRect) const;
	void drawTopWeatherInfo(QPainter * painter, int dayIndex, const QRect & contentsRect, bool fancyTemperature = false) const;
	
	void drawForecastHeader(QPainter * painter, const QRect & contentsRect) const;
	void drawForecast(QPainter * painter, const QRect & contentsRect, bool fancyTemperature = false) const;
	
	void drawDetailsHeader(QPainter * painter, int dayIndex, const QRect & detailsHeaderRect) const;
	void drawDetails(QPainter * painter, int dayIndex, const QRect & detailsContentsRect) const;
	
	void drawSatelliteImage(QPainter * painter, const QRect & contentsRect) const;

	void drawTemperature(QPainter * painter, const YawpWeather * weather, const QRect & tempRect) const;
	void drawFancyTemperature(QPainter * painter, const YawpDay * day, const QRect & tempRect) const;
	
	QRect getContentsRect(const QRect & contentsRect) const;

	/**
	 *  Get one of the buttons on the left top page (Preview, Details or Satellite).
	 *  This buttons will change the page.
	 */
	QRect getPageButtonRect (const QRect & contentsRect, Yawp::PageType pageType) const;
	
	/**
	 *  This button are on the left and right side of the city name and will change
	 *  the currently shown city.
	 */
	QRect getCityButtonRect(const QRect & contentsRect, bool previous) const;
	
	QRect getDetailsDayButtonRect(const QRect & detailsHeaderRect, bool previous) const;
	
	QRect getTodaysWeatherIconRect(const QRect & contentsRect) const;
	QRect getForecastWeatherIconRect(const QRect & detailsContentsRect, int forecastIndex) const;
	
	/** The header of the details rectangle is the highlighted part in the YaWP-Applet
	 * that contains the dayname or date or text "Forecast" right above the details contents.
	 */
	QRect getDetailsHeaderRect(const QRect & contentsRect) const;
	QRect getDetailsContentsRect(const QRect & contentsRect) const;
	
	void setButtonNames();
	
	QString createVisualCityName( const CityWeather * pCity ) const;
	
	void initPartChange(AbstractPainter::AnimationType animationType);
	
private:
	QStringList m_vButtonNames;
	QString m_sVisualCityName;
	
	bool m_bDayNames;
};

#endif // YAWP_DESKTOP_PAINTER_H
 
