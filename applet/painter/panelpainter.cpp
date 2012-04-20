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

//--- LOCAL ---
#include "panelpainter.h"
#include "logger/streamlogger.h"

//--- QT4 ---
#include <QLatin1String>

//--- KDE4 ----
#include <KGlobalSettings>
#include <Plasma/Theme>

#define HORIZONTAL_COMPACT_ICON_SIZE             0.65f
#define HORIZONTAL_TODAYS_COMPACT_TEMPERATURE    0.35f
#define HORIZONTAL_FORECAST_COMPACT_TEMPERATURE  0.20f

#define HORIZONTAL_TODAYS_TEMPERATURE            0.90f
#define HORIZONTAL_FORECAST_TEMPERATURE          0.40f


#define VERTICAL_COMPACT_ICON_SIZE               0.45f
#define VERTICAL_TODAYS_COMPACT_TEMPERATURE      0.35f
#define VERTICAL_FORECAST_COMPACT_TEMPERATURE    0.20f

#define VERTICAL_TODAYS_TEMPERATURE              0.60f
#define VERTICAL_FORECAST_TEMPERATURE            0.40f


PanelPainter::PanelPainter(
	Plasma::Applet * applet,
	const Yawp::ConfigData * configData,
	StateMachine * stateMachine,
	Plasma::FormFactor formFactor)
    : AbstractPainter(applet, configData, stateMachine),
      m_formFactor(formFactor),
      m_popupPainter(0)
{
}

PanelPainter::~PanelPainter()
{
}

Plasma::FormFactor
PanelPainter::formFactor()
{
	return m_formFactor;
}

QSize
PanelPainter::getSize(const QSizeF & currentSize) const
{
	if (m_formFactor == Plasma::Horizontal)
	{
		int todaysTemperatureWidth = 0;
		int forecastTemperatureWidth = 0;
		int contentsHeight = qMax((int)qRound(currentSize.height()), 30);
		
		getHorizontalTemperatureSize(contentsHeight, todaysTemperatureWidth, forecastTemperatureWidth);
		
		int width = 0;
		if (configData()->bUseCompactPanelLayout)
		{
			width = todaysTemperatureWidth + forecastTemperatureWidth * configData()->iPanelForecastDays;
		}
		else
		{
			if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
				width += todaysTemperatureWidth;
			if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
				width += currentSize.height();
			if (configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
				width += forecastTemperatureWidth * configData()->iPanelForecastDays;
			if (configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
				width += currentSize.height() * configData()->iPanelForecastDays;
		}
		return QSize(width, currentSize.height());
	}
	else
	{
		int todaysTemperatureHeight = 0;
		int forecastTemperatureHeight = 0;
		int contentsWidth = qMax((int)qRound(currentSize.width()), 30);
		
		getVerticalTemperatureSize(contentsWidth, todaysTemperatureHeight, forecastTemperatureHeight);
		
		int height = 0;
		if (configData()->bUseCompactPanelLayout)
		{
			height += VERTICAL_COMPACT_ICON_SIZE * contentsWidth;
			height += forecastTemperatureHeight * configData()->iPanelForecastDays;
		}
		else
		{
			if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
				height += todaysTemperatureHeight;
			if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
				height += currentSize.width();
			if (configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
				height += forecastTemperatureHeight * configData()->iPanelForecastDays;
			if (configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
				height += currentSize.width() * configData()->iPanelForecastDays;
		}
		return QSize(currentSize.width(), height);
	}
}

void
PanelPainter::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
	const CityWeather * city = stateMachine()->currentCity();
	if (city == 0 ||
	    city->days().count() == 0 ||
	    !configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
	{
		return;
	}

	const int maxDays = qMin(city->days().count(), configData()->iPanelForecastDays);
	QRect iconRect;

	for (int dayIndex = 0; dayIndex < maxDays; ++dayIndex)
	{
		if (m_formFactor == Plasma::Horizontal)
			iconRect = getHorizontalForecastIconRect(contentsRect(), dayIndex);
		else
			iconRect = getVerticalForecastIconRect(contentsRect(), dayIndex);
		
		if (iconRect.contains(event->pos().x(), event->pos().y()))
		{
			if (city->days().at(dayIndex)->hasNightValues())
			{
			      if (m_popupPainter)
				      m_popupPainter->initWeatherIconAnimation(dayIndex);
			      else
				      initWeatherIconChange(iconRect, dayIndex, !(dayIndex == 0));
			}
			event->accept();
			return;
		}
	}
}

QRect
PanelPainter::getContentsRect(const QRect & contentsRect) const
{
	return contentsRect;
}

void
PanelPainter::update()
{
	AbstractPainter::update();
	
	if (m_popupPainter)
		m_popupPainter->update();
}

void
PanelPainter::setupAnimationTimeLine()
{
	AbstractPainter::setupAnimationTimeLine();
	
	if (m_popupPainter)
		m_popupPainter->setupAnimationTimeLine();
}

void
PanelPainter::initCityChangeAnimation(int cityIndex)
{
	if (m_popupPainter)
		// When we have a popup painter than we will receive a signal
		// to change the city from this popup painter, so we do not initialize
		// city change animation here!!!
		m_popupPainter->initCityChangeAnimation(cityIndex);
	else
		AbstractPainter::initCityChangeAnimation(cityIndex);
}


void
PanelPainter::initWeatherIconAnimation(int dayIndex)
{
	if (m_popupPainter)
		m_popupPainter->initWeatherIconAnimation(dayIndex);
	else
		slotToggleWeatherIcon(dayIndex);
}

void
PanelPainter::setBusy(bool busy)
{
	AbstractPainter::setBusy(busy);
	
	if (m_popupPainter)
		m_popupPainter->setBusy(busy);
}

void
PanelPainter::handleLayoutChanges()
{
	if (m_formFactor == Plasma::Horizontal)
		getHorizontalTemperatureSize(contentsRect().height(), m_todaysTempSize, m_forecastTempSize);
	else
		getVerticalTemperatureSize(contentsRect().width(), m_todaysTempSize, m_forecastTempSize);
}

DesktopPainter *
PanelPainter::getPopupPainter() const
{
	return (DesktopPainter *)m_popupPainter;
}

void
PanelPainter::setPopupPainter(DesktopPainter * painter)
{
	if (m_popupPainter)
		m_popupPainter->disconnect(this);
	
	m_popupPainter = painter;

	connect(m_popupPainter, SIGNAL(signalCityChanged(int)), this, SLOT(slotChangeCity(int)), Qt::DirectConnection);
	connect(m_popupPainter, SIGNAL(signalToggleWeatherIcon(int)), this, SLOT(slotToggleWeatherIcon(int)), Qt::DirectConnection);
}

void
PanelPainter::slotChangeCity(int cityIndex)
{
	initPageChange(AbstractPainter::CityChangeAnimation, cityIndex, configData()->iCityIndex < cityIndex);
}

void
PanelPainter::slotToggleWeatherIcon(int dayIndex)
{
	QRect iconRect;
	
	if (dayIndex < configData()->iPanelForecastDays)
	{
		if (m_formFactor == Plasma::Horizontal)
			iconRect = getHorizontalForecastIconRect(contentsRect(), dayIndex);
		else
			iconRect = getVerticalForecastIconRect(contentsRect(), dayIndex);

		initWeatherIconChange(iconRect, dayIndex, !(dayIndex == 0));
	}
	else
	{
		setAnimationType(DummyAnimation);
		timeLine()->start();

		emit signalToggleWeatherIcon(dayIndex);
	}
}

void
PanelPainter::getHorizontalTemperatureSize(
	const int contentsHeight,
	int & todaysTemperatureWidth,
	int & forecastTemperatureWidth) const
{
	int todaysPixelSize = 0;
	int forecastPixelSize = 0;
	
	if (configData()->bUseCompactPanelLayout)
	{
		todaysPixelSize = qRound(contentsHeight * HORIZONTAL_TODAYS_COMPACT_TEMPERATURE);
		forecastPixelSize = qRound(contentsHeight * HORIZONTAL_FORECAST_COMPACT_TEMPERATURE);
	}
	else
	{
		todaysPixelSize = qRound(contentsHeight * HORIZONTAL_TODAYS_TEMPERATURE);
		forecastPixelSize = qRound(contentsHeight * HORIZONTAL_FORECAST_TEMPERATURE);
	}
	todaysTemperatureWidth = getTemperatureWidth(todaysPixelSize, QLatin1String("-99") + QChar(0xB0));
	forecastTemperatureWidth = getTemperatureWidth(forecastPixelSize, QLatin1String("-99") + QChar(0xB0));
	
	
	// todays temperature width will be set to maximum size of icon size an temperature size
	if (configData()->bUseCompactPanelLayout)
	{
		int iconSize = qRound(contentsHeight * HORIZONTAL_COMPACT_ICON_SIZE);
		todaysTemperatureWidth = qMax(todaysTemperatureWidth, iconSize);
		forecastTemperatureWidth = qMax(forecastTemperatureWidth, iconSize);
	}
	else
	{
		// leave a little space to the weather icon of the previous day
		forecastTemperatureWidth = qRound(1.05 * forecastTemperatureWidth);
	}
}

void
PanelPainter::getVerticalTemperatureSize(
	const int contentsWidth,
	int & todaysTemperatureHeight,
	int & forecastTemperatureHeight) const
{
	if (configData()->bUseCompactPanelLayout)
	{
		todaysTemperatureHeight = qRound(contentsWidth * VERTICAL_TODAYS_COMPACT_TEMPERATURE);
		forecastTemperatureHeight = 2 * qRound(contentsWidth * VERTICAL_FORECAST_COMPACT_TEMPERATURE);
	}
	else
	{
		todaysTemperatureHeight = qRound(contentsWidth * VERTICAL_TODAYS_TEMPERATURE);
		forecastTemperatureHeight = 2 * qRound(contentsWidth * VERTICAL_FORECAST_TEMPERATURE);
	}
}

void
PanelPainter::drawPage(QPainter * painter, const QRect & contentsRect) const
{
	dStartFunct();
	const CityWeather * city = stateMachine()->currentCity();
	
	if (!city || city->days().count() == 0)
	{
		dWarning() << "No city available";
		dEndFunct();
		return;
	}
	if (m_formFactor == Plasma::Horizontal)
		drawHorizontalApplet(painter, contentsRect);
	else
		drawVerticalApplet(painter, contentsRect);
	dEndFunct();
}

void
PanelPainter::drawHorizontalApplet(QPainter * painter, const QRect & contentsRect) const
{
	dStartFunct();

	const YawpWeather * weather = stateMachine()->weather(0, true);
	if (weather == 0)
	{
		dWarning() << "No weather information available";
		dEndFunct();
		return;
	}
	
	/***  PAINT TODAYS ICON AND TEMPERATURE ***
	 */
	if (configData()->bUseCompactPanelLayout ||
	    configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelIcon ) )
	{
		drawWeatherIcon(painter, weather, getHorizontalTodaysIconRect(contentsRect));
	}
	if (configData()->bUseCompactPanelLayout ||
	    configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ) )
	{
		if (weather->currentTemperature() < SHRT_MAX)
		{
			int pixelSize = 0;
			int alignmentFlag = Qt::AlignCenter;
			
			if (configData()->bUseCompactPanelLayout)
			{
				pixelSize = qRound(contentsRect.height() * HORIZONTAL_TODAYS_COMPACT_TEMPERATURE);
			}
			else
			{
				pixelSize = qRound(contentsRect.height() * HORIZONTAL_TODAYS_TEMPERATURE);
				alignmentFlag = Qt::AlignVCenter | Qt::AlignRight;
			}
			
			QFont font = painter->font();
			font.setBold(false);
			font.setPixelSize(pixelSize);
			painter->setFont(font);
			
			drawText(painter, getHorizontalTodaysTempRect(contentsRect), alignmentFlag,
				QString::number(weather->currentTemperature()) + QChar(0xB0) );
		}
	}
	
	QFont font = painter->font();
	font.setBold(false);
	
	if (configData()->bUseCompactPanelLayout)
		font.setPixelSize(contentsRect.height() * HORIZONTAL_FORECAST_COMPACT_TEMPERATURE);
	else
		font.setPixelSize(contentsRect.height() * HORIZONTAL_FORECAST_TEMPERATURE);

	painter->setFont(font);
	
	const int maxDays = qMin(stateMachine()->currentCity()->days().count(), configData()->iPanelForecastDays);
	const int alignmentFlag = configData()->bUseCompactPanelLayout ?  Qt::AlignCenter : Qt::AlignRight | Qt::AlignHCenter;
	
	const bool drawForecastIcon = configData()->bUseCompactPanelLayout ||
		configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelIcon );
	const bool drawForecastTemp = configData()->bUseCompactPanelLayout ||
		configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelTemperature );
	
	for (int forecastDay = 0; forecastDay < maxDays; ++forecastDay)
	{
		weather = stateMachine()->weather(forecastDay);
		
		if (animationType() == AbstractPainter::IconAnimation &&
		    animationIndex() == forecastDay)
		{
			// we do not draw animated weather icon here
		}
		else if (drawForecastIcon)
		{
			QRect iconRect = getHorizontalForecastIconRect(contentsRect, forecastDay);
			drawWeatherIcon(painter, weather, iconRect, !(forecastDay == 0));
		}
		
		if (drawForecastTemp)
		{
			QRect tempRect = getHorizontalForecastTempRect(contentsRect, forecastDay);
			drawTemperature(painter, weather, alignmentFlag, tempRect);
		}
	}
	dEndFunct();
}

void
PanelPainter::drawVerticalApplet(QPainter * painter, const QRect & contentsRect) const
{
	dStartFunct();

	const YawpWeather * weather = stateMachine()->weather(0, true);
	if (weather == 0)
	{
		dWarning() << "No weather information available";
		dEndFunct();
		return;
	}
	
	/***  PAINT TODAYS ICON AND TEMPERATURE ***
	 */
	if (configData()->bUseCompactPanelLayout ||
	    configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelIcon ) )
	{
		drawWeatherIcon(painter, weather, getVerticalTodaysIconRect(contentsRect));
	}
	if (configData()->bUseCompactPanelLayout ||
	    configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ) )
	{
		if (weather->currentTemperature() < SHRT_MAX )
		{
			int pixelSize = 0;
			int alignmentFlag = Qt::AlignCenter;
			
			if (configData()->bUseCompactPanelLayout)
				pixelSize = qRound(contentsRect.width() * VERTICAL_TODAYS_COMPACT_TEMPERATURE);
			else
				pixelSize = qRound(contentsRect.width() * VERTICAL_TODAYS_TEMPERATURE);

			QFont font = painter->font();
			font.setBold(false);
			font.setPixelSize(pixelSize);
			painter->setFont(font);
			
			drawText(painter, getVerticalTodaysTempRect(contentsRect), alignmentFlag,
				QString::number(weather->currentTemperature()) + QChar(0xB0) );
		}
	}

	QFont font = painter->font();
	font.setBold(false);
	
	if (configData()->bUseCompactPanelLayout)
		font.setPixelSize(contentsRect.width() * VERTICAL_FORECAST_COMPACT_TEMPERATURE);
	else
		font.setPixelSize(contentsRect.width() * VERTICAL_FORECAST_TEMPERATURE);

	painter->setFont(font);
	
	const int maxDays = qMin(stateMachine()->currentCity()->days().count(), configData()->iPanelForecastDays);
	int alignmentFlag = Qt::AlignCenter;
	
	const bool drawForecastIcon = configData()->bUseCompactPanelLayout ||
		configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelIcon );
	const bool drawForecastTemp = configData()->bUseCompactPanelLayout ||
		configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelTemperature );

	for (int forecastDay = 0; forecastDay < maxDays; ++forecastDay)
	{
		weather = stateMachine()->weather(forecastDay);
		
		if (animationType() == AbstractPainter::IconAnimation &&
		    animationIndex() == forecastDay)
		{
			// we do not draw animated weather icon here
		}
		else if (drawForecastIcon)
		{
			QRect iconRect = getVerticalForecastIconRect(contentsRect, forecastDay);
			drawWeatherIcon(painter, weather, iconRect, !(forecastDay == 0));
		}
		
		if (drawForecastTemp)
		{
			QRect tempRect = getVerticalForecastTempRect(contentsRect, forecastDay);
			drawTemperature(painter, weather, alignmentFlag, tempRect);
		}
	}
	dEndFunct();
}

QRect
PanelPainter::getHorizontalTodaysIconRect(const QRect & contentsRect) const
{
	QRect iconRect;
	if (configData()->bUseCompactPanelLayout)
	{
		int iconSize = qRound(contentsRect.height() * HORIZONTAL_COMPACT_ICON_SIZE);
		int offset = m_todaysTempSize > iconSize ? qRound((m_todaysTempSize - iconSize) / 2) : 0;
		iconRect = QRect( offset, 0, iconSize, iconSize );
	}
	else
	{
		int tempOffset = 0;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
			tempOffset = m_todaysTempSize;
		iconRect = QRect(tempOffset, 0, contentsRect.height(), contentsRect.height());
	}
	return iconRect;
}

QRect
PanelPainter::getVerticalTodaysIconRect(const QRect & contentsRect) const
{
	QRect iconRect;
	if (configData()->bUseCompactPanelLayout)
	{
		int iconSize = qRound(contentsRect.width() * VERTICAL_COMPACT_ICON_SIZE);
		iconRect = QRect( 0, 0, iconSize, iconSize );
	}
	else
	{
		int tempOffset = 0;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
			tempOffset = m_todaysTempSize;
		iconRect = QRect(0, tempOffset, contentsRect.width(), contentsRect.width());
	}
	return iconRect;
}

QRect
PanelPainter::getHorizontalTodaysTempRect(const QRect & contentsRect) const
{
	QRect tempRect;
	if (configData()->bUseCompactPanelLayout)
	{
		int iconSize = qRound(contentsRect.height() * (HORIZONTAL_COMPACT_ICON_SIZE - 0.05f));
		tempRect = QRect( 0, iconSize, m_todaysTempSize, contentsRect.height()-iconSize );
	}
	else
	{
		tempRect = QRect(0, 0, m_todaysTempSize, contentsRect.height());
	}
	return tempRect;
}

QRect
PanelPainter::getVerticalTodaysTempRect(const QRect & contentsRect) const
{
	QRect tempRect;
	if (configData()->bUseCompactPanelLayout)
	{
		int iconSize = qRound(contentsRect.width() * VERTICAL_COMPACT_ICON_SIZE);
		tempRect = QRect(iconSize, 0, contentsRect.width() - iconSize, iconSize );
	}
	else
	{
		tempRect = QRect(0, 0, contentsRect.width(), m_todaysTempSize);
	}
	return tempRect;
}

QRect
PanelPainter::getHorizontalForecastIconRect(const QRect & contentsRect, int dayIndex) const
{
	QRect iconRect;
	if (configData()->bUseCompactPanelLayout)
	{
		int iconSize = qRound(contentsRect.height() * (HORIZONTAL_COMPACT_ICON_SIZE - 0.10f));
		int offset = m_forecastTempSize > iconSize ? qRound((m_forecastTempSize - iconSize) / 2) : 0;
		iconRect = QRect( m_todaysTempSize + m_forecastTempSize * dayIndex + offset, 0, iconSize, iconSize);
	}
	else
	{
		int offset = 0;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
			offset += m_todaysTempSize;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
			offset += contentsRect.height();
		
		if (configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
			offset += m_forecastTempSize * (dayIndex + 1);
		offset += contentsRect.height() * dayIndex;
		
		iconRect = QRect(offset, 0, contentsRect.height(), contentsRect.height());
	}
	return iconRect;
}

QRect
PanelPainter::getVerticalForecastIconRect(const QRect & contentsRect, int dayIndex) const
{
	QRect iconRect;
	if (configData()->bUseCompactPanelLayout)
	{
		int todayIconSize = qRound(contentsRect.width() * VERTICAL_COMPACT_ICON_SIZE);
		int iconSize = qRound(contentsRect.width() * (VERTICAL_COMPACT_ICON_SIZE - 0.10f));
		int offset = m_forecastTempSize > iconSize ? qRound((m_forecastTempSize - iconSize) / 2) : 0;
		iconRect = QRect(0, todayIconSize + m_forecastTempSize * dayIndex + offset, iconSize, iconSize);
	}
	else
	{
		int offset = 0;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
			offset += m_todaysTempSize;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
			offset += contentsRect.width();
		
		if (configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
			offset += m_forecastTempSize * (dayIndex + 1);
		
		offset += contentsRect.width() * dayIndex;
		
		iconRect = QRect(0, offset, contentsRect.width(), contentsRect.width());
	}
	return iconRect;
}

QRect
PanelPainter::getHorizontalForecastTempRect(const QRect & contentsRect, int dayIndex) const
{
	QRect iconRect;
	if (configData()->bUseCompactPanelLayout)
	{
		int iconSize = qRound(contentsRect.height() * (HORIZONTAL_COMPACT_ICON_SIZE - 0.10f));
		int offset = m_todaysTempSize > iconSize ? qRound((m_todaysTempSize - iconSize) / 2) : 0;
		iconRect = QRect( m_todaysTempSize + m_forecastTempSize * dayIndex + offset, iconSize,
		                  m_forecastTempSize, contentsRect.height()-iconSize );
	}
	else
	{
		int offset = 0;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
			offset += m_todaysTempSize;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
			offset += contentsRect.height();
		
		offset += m_forecastTempSize * dayIndex;
		
		if (configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
			offset += contentsRect.height() * dayIndex;
		
		iconRect = QRect(offset, 0, m_forecastTempSize, contentsRect.height());
	}
	return iconRect;
}

QRect
PanelPainter::getVerticalForecastTempRect(const QRect & contentsRect, int dayIndex) const
{
	QRect iconRect;
	if (configData()->bUseCompactPanelLayout)
	{
		int todayIconSize = qRound(contentsRect.width() * VERTICAL_COMPACT_ICON_SIZE);
		
		int iconSize = qRound(contentsRect.width() * (VERTICAL_COMPACT_ICON_SIZE - 0.10f));
		int offset = m_todaysTempSize > iconSize ? qRound((m_todaysTempSize - iconSize) / 2) : 0;
		iconRect = QRect( iconSize, todayIconSize + m_forecastTempSize * dayIndex + offset, 
				  contentsRect.width() - iconSize, m_forecastTempSize);
	}
	else
	{
		int offset = 0;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelTemperature ))
			offset += m_todaysTempSize;
		if (configData()->todaysWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
			offset += contentsRect.width();
		
		offset += m_forecastTempSize * dayIndex + contentsRect.width() * 0.05;
		
		if (configData()->forecastWeatherPanelFormat.testFlag( Yawp::PanelIcon ))
			offset += contentsRect.width() * dayIndex;
		
		iconRect = QRect(0, offset, contentsRect.width(), m_forecastTempSize);
	}
	return iconRect;
}

void
PanelPainter::drawTemperature(QPainter * painter, const YawpWeather * weather, int alignmentFlag, const QRect & tempRect) const
{
	QRect rect(tempRect.x(), tempRect.y(), tempRect.width(), qRound(tempRect.height() / 2.0));
	
	if (weather->highTemperature() < SHRT_MAX)
	{
		QString text = QString::number(weather->highTemperature()) + QChar(0xB0);
		drawText( painter, rect, alignmentFlag, text );
	}
	if (weather->lowTemperature() < SHRT_MAX)
	{
		rect.moveTop( rect.bottom() );
		QString text  = QString::number(weather->lowTemperature()) + QChar(0xB0);

		drawGreyText( painter, rect, alignmentFlag, text );
	}
}

int
PanelPainter::getTemperatureWidth( int pixelSize, const QString & text ) const
{
	Plasma::Theme * theme = Plasma::Theme::defaultTheme();
	QFont font = theme->font(Plasma::Theme::DefaultFont);
	
	font.setBold(false);
	font.setPixelSize( pixelSize );
	QFontMetrics fm(font);
	return fm.width( text );
}
