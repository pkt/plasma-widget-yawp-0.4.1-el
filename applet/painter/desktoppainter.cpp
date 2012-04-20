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
#include "desktoppainter.h"
#include "pageanimator.h"
#include "weatherservice.h"
#include "yawpday.h"
#include "yawpdefines.h"

#include "logger/streamlogger.h"

//--- QT4 CLASSES ---
#include <QGraphicsSceneMouseEvent>
#include <QFontMetrics>

//--- KDE4 CLASSES ---
#include <KGlobalSettings>
#include <Plasma/Svg>
#include <Plasma/Theme>

//--- Desktop Layout Constants ---
#define YAWP_ORIG_SIZEX			273.0f
#define YAWP_ORIG_SIZEY			255.0f


DesktopPainter::DesktopPainter (
	QGraphicsWidget * widget,
	const Yawp::ConfigData * configData,
	StateMachine * stateMachine)
    : AbstractPainter(widget, configData, stateMachine),
      m_bDayNames(true)
{
}

DesktopPainter::~DesktopPainter()
{
}

Plasma::FormFactor
DesktopPainter::formFactor() { return Plasma::Planar; }

QSize
DesktopPainter::getSize(const QSizeF & currentSize) const
{
	if (currentSize.width() > 0)
	{
		qreal dScale = currentSize.width() / YAWP_ORIG_SIZEX;
		return QSize(currentSize.width(), dScale * YAWP_ORIG_SIZEY);
	}
	return QSize(YAWP_ORIG_SIZEX, YAWP_ORIG_SIZEY);
}

QRect
DesktopPainter::getContentsRect(const QRect & contentsRect) const
{
	qreal dScale = contentsRect.width() / YAWP_ORIG_SIZEX;
	qreal spacer = 0;
	if (!configData()->bUseCustomThemeBackground &&
	    configData()->sBackgroundName.compare("default") != 0 &&
	    configData()->sBackgroundName.compare("naked") != 0)
	{
		spacer = qRound(dScale * 8.0f);
	}
	else
		spacer = qRound(dScale * 4.0f);
	return contentsRect.adjusted(spacer, spacer, -spacer, -spacer);
}

void
DesktopPainter::drawPage(QPainter * painter, const QRect & contentsRect) const
{
	dStartFunct();
	
	drawPlainPage (painter, contentsRect);
	
	switch (stateMachine()->currentPage())
	{
		case Yawp::PreviewPage:
		{
			drawTopWeatherInfo(painter, 0, contentsRect);
			      
			// when we got a valid weather, than we have a valid city as well
			// so we do not have to check it anymore
			drawForecastHeader(painter, getDetailsHeaderRect(contentsRect));
			drawForecast(painter, getDetailsContentsRect(contentsRect));
		}
		break;
		
		case Yawp::DetailsPage:
		{
			int dayIndex = stateMachine()->detailsDayIndex();

			drawTopWeatherInfo(painter, dayIndex, contentsRect);
			drawDetailsHeader(painter, dayIndex, getDetailsHeaderRect(contentsRect));
			drawDetails(painter, dayIndex, getDetailsContentsRect(contentsRect));
		}
		break;
		
		case Yawp::SatellitePage:
			if (stateMachine()->hasPage(Yawp::SatellitePage))
				drawSatelliteImage(painter, contentsRect);
		break;
		
		default:
			break;
	}
	dEndFunct();
}

void
DesktopPainter::drawBackground (QPainter * painter, const QRect & contentsRect) const
{
	if (configData() == 0)
		return;

	QString backgroundName;
	Plasma::Svg * currentSvg = 0;
	
	if (configData()->bUseCustomTheme && configData()->bUseCustomThemeBackground)
	{
		backgroundName = QLatin1String("back-default");
		currentSvg = customSvg();
	}
	else
	{
		backgroundName = QString("back-%1").arg(configData()->sBackgroundName);
		currentSvg = svg();
	}
	
	if (currentSvg != 0 &&
	    currentSvg->isValid() &&
	    currentSvg->hasElement(backgroundName))
	{
		currentSvg->paint(painter, contentsRect, backgroundName);
	}
}

void
DesktopPainter::drawPlainPage (QPainter * painter, const QRect & contentsRect) const
{
	const CityWeather * city = stateMachine()->currentCity();
	if (!city)
		return;

	const qreal dOpacity = painter->opacity();
	Yawp::PageType currPage = stateMachine()->currentPage();
	const Yawp::PageType vPages[3] = {Yawp::PreviewPage, Yawp::DetailsPage, Yawp::SatellitePage};
	bool vAvailables[3] = {false, false, false};
	int iPageCounter = 0;

	//--- get the state off all pages for the current city ---
	for( int i = 0; i < 3; ++i )
	{
		if (stateMachine()->hasPage(vPages[i]))
		{
			vAvailables[i] = true;
			iPageCounter += 1;
		}
	}

	/*  draw all available buttons on top of widget (Preview Page, Details Page and Satellite Page),
	 *  when there are at least two of them,
	 *  otherwise the user has no choice, which page he/she wants to see.
	 */
	if (iPageCounter > 1)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (vAvailables[i])
			{
				painter->setOpacity( currPage == vPages[i] ? 1.0f : 0.5f );
				drawImage(painter, getPageButtonRect(contentsRect, vPages[i]), m_vButtonNames.at(i));
			}
		}
	}
	painter->setOpacity( dOpacity );
	const qreal scalingFactor = contentsRect.width() / YAWP_ORIG_SIZEX;
	
	/*** DRAW CITY AND LOCATION NAME ***
	*/
	QFont font = painter->font();
	font.setBold(false);
	font.setPixelSize(17.0f * scalingFactor);
	QFontMetrics fontMetrics(font);
	painter->setFont(font);

	QRect rectArrowPrev = getCityButtonRect(contentsRect, true);
	QRect rectArrowNext = getCityButtonRect(contentsRect, false);
	QRect rectCity( qRound(rectArrowPrev.right() + 2.0f*scalingFactor),
	                qRound(rectArrowPrev.top()   - 2.0f*scalingFactor),
	                qRound(rectArrowNext.left() - rectArrowPrev.right()
	                    - 4.0f*scalingFactor),
	                qRound(fontMetrics.height()) );
	
	if (isBusy())
		drawText(painter, rectCity, Qt::AlignHCenter | Qt::AlignVCenter, tr2i18n("Connecting..."));
	else
		drawText(painter, rectCity, Qt::AlignHCenter | Qt::AlignVCenter, m_sVisualCityName);


	/*** DRAW BUTTONS TO SELECT PREVIOUS AND NEXT CITY ***
	 */
	int cityCounter = stateMachine()->serviceModel()->rowCount();
	if( cityCounter > 1 )
	{
		painter->setOpacity( configData()->iCityIndex == 0 ? 0.5 : 1.0 );
		drawImage( painter, rectArrowPrev, QLatin1String("arrow-left") );
		painter->setOpacity( configData()->iCityIndex+1 == cityCounter ? 0.5 : 1.0 );
		drawImage( painter, rectArrowNext, QLatin1String("arrow-right") );
	}
	painter->setOpacity( dOpacity );
}

void
DesktopPainter::drawTopWeatherInfo(QPainter * painter, int dayIndex, const QRect & contentsRect, bool fancyTemperature) const
{
	dStartFunct();
	
	const qreal dScale = contentsRect.width() / YAWP_ORIG_SIZEX;
	const YawpWeather * weather = stateMachine()->weather(dayIndex);
	if (!weather)
	{
		dWarning() << "No weather data available";
		dEndFunct();
		return;
	}
	QFont font = painter->font();

	// paint weather icon when this icon is not animated right now
	if ( !(animationType() == AbstractPainter::IconAnimation && animationIndex() == dayIndex) )
	{
		QRect iconRect = getTodaysWeatherIconRect(contentsRect);
		drawWeatherIcon(painter, weather, iconRect);
	}
	// paint high and low temperature of weather (on the left side of weather icon and below the page buttons)
	if (fancyTemperature)
	{
		QRect temperatureRect(
			contentsRect.left(),
			contentsRect.top(),
			80.0f * dScale,
			95.0f * dScale);
		
		drawFancyTemperature(painter, stateMachine()->day(dayIndex), temperatureRect);
	}
	else
	{
		QRect temperatureRect(
			contentsRect.left(),
			qRound(contentsRect.top() + 38.0f * dScale),
			qRound(68.0f * dScale),
			qRound(45.0 * dScale) );
		
		font.setBold(true);
		font.setPixelSize(18.0f * dScale);
		painter->setFont(font);

		drawTemperature(painter, weather, temperatureRect);
	}
	
	// paint current temperature
	if (weather->currentTemperature() < SHRT_MAX)
	{
		font.setPixelSize(40.0f * dScale);
		font.setBold(false);
		painter->setFont(font);
		
		QRect temperatureRect(
			qRound(contentsRect.left() + 170.0f * dScale),
			contentsRect.top(),
			qRound(100.0f * dScale),
			qRound( 45.0f * dScale) );
		
		QString currentTemp = QString::number( weather->currentTemperature() ) + QChar(0xB0);
		drawText( painter, temperatureRect, Qt::AlignCenter, currentTemp);
	}
	
	// Create the string that contains all conditions about wind
	QString sText = weather->windShortText();
	
	if (weather->humidity() < SHRT_MAX && weather->humidity() > 0 )
		sText.append( QString("\n%1%").arg(weather->humidity()) );
	
	if (weather->pressure() < SHRT_MAX )
		sText.append( weather->pressureShortText() );

	// paint wind conditions below current temperature
	if( !sText.isEmpty() )
	{
		font.setBold(false);
		font.setPixelSize(13.0f * dScale);
		painter->setFont(font);
		QFontMetrics fontMetrics = QFontMetrics(font);

		QRect conditionRect = QRect(
			qRound(contentsRect.left() + 170.0f * dScale),
			qRound(contentsRect.top() + 44.0f * dScale),
			qRound(100.0f * dScale),
			3 * fontMetrics.height() );
		
		drawText(painter, conditionRect, Qt::AlignHCenter | Qt::AlignTop, sText );
	}
	dEndFunct();
}

void
DesktopPainter::drawForecastHeader(QPainter * painter, const QRect & forecastRect) const
{
	dStartFunct();
	if (animationType() == AbstractPainter::SlidingDayNames ||
	    animationType() == AbstractPainter::FullDetailsChange)
	{
		dTracing() << "We do not paint the forecast header during forecast header animation";
		dEndFunct();
		return;
	}
	
	const CityWeather * city = stateMachine()->currentCity();
	if (!city || city->days().count() <= 1)
	{
		dWarning() << "No weather data available";
		dEndFunct();
		return;
	}
	
	const int forecastCount = qMin(4, city->days().count()-1);
	const QString dayNamesFormat = (m_bDayNames ? QLatin1String("ddd") : QLatin1String("M/d"));

	QRect rectDay(forecastRect);
	rectDay.setWidth( qRound(rectDay.width() / forecastCount) );
	rectDay.setHeight( qRound(rectDay.height() * 0.95f) );
	
	QFont font = painter->font();
	font.setBold(true);
	font.setPixelSize( qRound(forecastRect.height() * 0.60f) );
	painter->setFont(font);

	for (int dayIndex = 1; dayIndex <= forecastCount; ++dayIndex)
	{
		const YawpDay * day = city->days().at(dayIndex);
		QString date = i18n(day->date().toString(dayNamesFormat).toUtf8().constData());
		drawText( painter, rectDay, Qt::AlignHCenter | Qt::AlignBottom, date );
		
		rectDay.moveLeft(rectDay.right());
	}
	dEndFunct();
}

void
DesktopPainter::drawForecast(QPainter * painter, const QRect & detailsContentsRect, bool fancyTemperature) const
{
	dStartFunct();
	if (animationType() == AbstractPainter::FullDetailsChange)
	{
		dTracing() << "We do not paint forecast info during full detail change";
		dEndFunct();
		return;
	}
	
	const CityWeather * city = stateMachine()->currentCity();
	
	QFont font = painter->font();
	font.setBold(false);
	font.setPixelSize( qRound(detailsContentsRect.height() * 0.15f) );
	painter->setFont(font);
	
	if (!city || city->days().count() <= 1)
	{
		drawGreyText(painter, detailsContentsRect, Qt::AlignCenter, i18n("No forecast information available"));
		dEndFunct();
		return;
	}
	
	const int forecastCount = qMin(4, city->days().count()-1);
	const qreal dScale = detailsContentsRect.width() / YAWP_ORIG_SIZEY;
	
	for (int dayIndex = 1; dayIndex <= forecastCount; ++dayIndex)
	{
		QRect rect = getForecastWeatherIconRect(detailsContentsRect, dayIndex);

		if (fancyTemperature)
		{
			rect.setTop( detailsContentsRect.top() + 3.0f * dScale);
			rect.setHeight( detailsContentsRect.height() * 0.95f );
			rect.setLeft(rect.left() - 5.0f * dScale);
			rect.setRight(rect.right() + 5.0f * dScale);
			
			drawFancyTemperature(painter, stateMachine()->day(dayIndex), rect);
		}
		else
		{
			const YawpWeather * weather = stateMachine()->weather(dayIndex);
			
			if ( !(animationType() == IconAnimation && animationIndex() == dayIndex) )
				drawWeatherIcon(painter, weather, rect);
		  
			rect.moveTop(rect.bottom() + rect.height() * 0.14f);
			rect.setHeight(rect.height() * 0.8f);
			rect.setLeft(rect.left() - 8.0f * dScale);
			rect.setRight(rect.right() + 8.0f * dScale);

			drawTemperature(painter, weather, rect);
		}
	}
	dEndFunct();
}

void
DesktopPainter::drawDetailsHeader(QPainter * painter, int dayIndex, const QRect & detailsHeaderRect) const
{
	dStartFunct();
	
	const YawpDay * day = stateMachine()->day(dayIndex);
	if (!day)
	{
		dWarning() << "No weather data available";
		dEndFunct();
		return;
	}
	if (animationType() == AbstractPainter::FullDetailsChange)
	{
		dTracing() << "We do not paint details during full detail change";
		dEndFunct();
		return;
	}
	
	const QRect previousDayRect = getDetailsDayButtonRect(detailsHeaderRect, true);
	const QRect nextDayRect = getDetailsDayButtonRect(detailsHeaderRect, false);
	const qreal dScale = detailsHeaderRect.width() / YAWP_ORIG_SIZEX;
	
	int maxDayIndex = stateMachine()->currentCity()->days().count();
	
	painter->save();
	painter->setOpacity(dayIndex > 0 ? 1.0f : 0.5f);
	drawImage(painter, previousDayRect, "arrow-left");
	painter->setOpacity(dayIndex + 1 < maxDayIndex ? 1.0f : 0.5f);
	drawImage(painter, nextDayRect, "arrow-right");
	painter->restore();
	
	QFont font = painter->font();
	font.setBold(true);
	font.setPixelSize( qRound(17.0f * dScale) );
	painter->setFont(font);
	
	QRect textRect = QRect(
		qRound(detailsHeaderRect.left() + 30.0f * dScale),
		detailsHeaderRect.top(),
		qRound(detailsHeaderRect.width() - 60.0f * dScale),
		qRound(detailsHeaderRect.height() * 0.95f) );

	drawText(painter, textRect, Qt::AlignHCenter | Qt::AlignBottom, tr2i18n("Forecast details"));
	
	dEndFunct();
}

void
DesktopPainter::drawDetails(QPainter * painter, int dayIndex, const QRect & detailsContentsRect) const
{
	dStartFunct();
	
	const qreal dScale = detailsContentsRect.width() / YAWP_ORIG_SIZEX;
	if (animationType() == AbstractPainter::DetailsChange ||
	    animationType() == AbstractPainter::FullDetailsChange)
	{
		dTracing() << "We do not paint details during detail change";
		dEndFunct();
		return;
	}
	
	QFont font = painter->font();
	font.setPixelSize(16.0f * dScale);
	font.setBold(false);
	painter->setFont(font);

	const YawpDay * day = stateMachine()->day(dayIndex);
	if (!day)
	{
		drawGreyText(painter, detailsContentsRect, Qt::AlignCenter, i18n("No detail information available"));
		dEndFunct();
		return;
	}
	
	painter->save();
	painter->setBrush( QColor(Qt::black) );
	painter->setPen( Qt::NoPen );
	painter->setOpacity( 0.4 );
	painter->drawRect( detailsContentsRect );
	painter->restore();

	QRect textRect = detailsContentsRect.adjusted(
		qRound(dScale * 4.0f), 
		qRound(dScale * 6.0f),
		-qRound(dScale * 4.0f),
		-qRound(dScale * 6.0f) );

	int propertyPageIndex = stateMachine()->currentPropertyPage();
	
	if (propertyPageIndex + 1 == stateMachine()->maxPropertyPage())
	{
		const CityWeather * city = stateMachine()->currentCity();
		QFontMetrics fm(font);
		
		QRect creditRect = detailsContentsRect;
		creditRect.setHeight( qRound(fm.height() * 3.5f) );
		

		QString creditText = city->credit();
//		if( !city->creditURL().isEmpty() && fm.width(city->creditURL()) < creditRect.width() )
//			creditText += "\n" + city->creditURL();

		drawText( painter, creditRect, Qt::AlignVCenter | Qt::AlignHCenter | Qt::TextWordWrap, creditText );
		
		QString updateText;
		if (city->lastUpdate().isValid())
		{
			updateText.append( i18n("Last update:") + " " +
				KGlobal::locale()->formatDateTime(city->lastUpdate(), KLocale::FancyShortDate, false));
		}
		else
			updateText.append(i18n("*** Not updated yet ***"));
		
		if (city->observationPeriode().isValid())
		{
			QDateTime obsTime;
			obsTime = city->fromLocalTime(city->observationPeriode());

			updateText.append( QString("\n%1 %2")
				.arg(i18n("Observation Periode:"))
				.arg(KGlobal::locale()->formatDateTime(obsTime, KLocale::FancyShortDate, false)) );
		}
		drawGreyText(painter, textRect, Qt::AlignBottom | Qt::AlignHCenter, updateText);

	}
	else if (day != 0)
	{
		const YawpWeather * weather = stateMachine()->weather(dayIndex);
		QString text;
		
		if (weather->dayTime())
			text = i18n(day->date().toString("dddd").toUtf8().constData()) + "\n";
		else
			text = i18n(day->date().toString("dddd").toUtf8().constData()) + QString(" ") + i18n("night") + "\n";
		
		int lineIndex = propertyPageIndex * 3;
		int maxLineIndex = (lineIndex + 3 <= weather->propertyTextLines().count() ? lineIndex + 3 : weather->propertyTextLines().count());

		for( ; lineIndex < maxLineIndex; ++lineIndex )
			text += "\n" + weather->propertyTextLines().at(lineIndex);
		
		drawText(painter, textRect, Qt::AlignLeft | Qt::AlignTop, text);
		
		drawGreyText(painter, textRect, Qt::AlignRight | Qt::AlignTop,
			QString("%1/%2").arg(propertyPageIndex + 1).arg(stateMachine()->maxPropertyPage()));
	}
	else
	{
		drawGreyText(painter, detailsContentsRect, Qt::AlignCenter, i18n("No detail information available"));
	}
	dEndFunct();
}

void
DesktopPainter::drawSatelliteImage(QPainter * painter, const QRect & contentsRect) const
{
	const float dScale = contentsRect.width() / YAWP_ORIG_SIZEX;
	const CityWeather * city = stateMachine()->currentCity();
	
	int offset = 55.0f * dScale;
	QRect satelliteRect(
		contentsRect.left(),
		qRound(contentsRect.top() + offset),
		contentsRect.width(),
		qRound(contentsRect.height() - offset) );
	
	// we draw a black background to hide the details header rectangle,
	// that contains the daynames in preview mode and "Forecast details" in detail mode
	painter->save();
	painter->setBrush(QColor(Qt::black));
	painter->setPen(QColor(Qt::black));
	painter->drawRect(satelliteRect);
	painter->restore();
	
	QImage satelliteImage = city->satelliteImage();
	
	/** Try to scale the satellite image properly
	 */
	const float imageAspectRatio = ((float)satelliteImage.width()) / ((float)satelliteImage.height());
	int heightForMaxWidth = satelliteRect.width() / imageAspectRatio;
	int widthForMaxHeight = satelliteRect.height() * imageAspectRatio;
	
	if (heightForMaxWidth <= satelliteRect.height())
	{
		int heightOffset = qRound( ((float)(satelliteRect.height() - heightForMaxWidth)) / 2.0f );
		satelliteRect.adjust(0, heightOffset, 0, -heightOffset);
	}
	else
	{
		int widthOffset = qRound( ((float)(satelliteRect.width() - widthForMaxHeight)) / 2.0f );
		satelliteRect.adjust(widthOffset, 0, -widthOffset, 0);
	}
	
	//--- Draw the satellite well sized ---
	painter->drawImage( satelliteRect, satelliteImage, QRect(QPoint(0, 0), satelliteImage.size()) );
}

void
DesktopPainter::drawTemperature(QPainter * painter, const YawpWeather * weather, const QRect & tempRect) const
{
	QRect rect(tempRect.x(), tempRect.y(), tempRect.width(), qRound(tempRect.height() / 2.0));
	
	if (weather->highTemperature() < SHRT_MAX)
	{
		QString text = i18nc("High degrees", "H: %1", weather->highTemperature()) + QChar(0xB0);
		drawText( painter, rect, Qt::AlignCenter, text );
	}
	if (weather->lowTemperature() < SHRT_MAX)
	{
		rect.moveTop( rect.bottom() );
		QString text  = i18nc("Low degrees",  "L: %1", weather->lowTemperature())  + QChar(0xB0);

		drawGreyText( painter, rect, Qt::AlignCenter, text );
	}
}
void
DesktopPainter::drawFancyTemperature(QPainter * painter, const YawpDay * day, const QRect & tempRect) const
{
	QFont font = painter->font();
	font.setPixelSize(0.2 * tempRect.height());
	QFontMetrics fm( font );
	painter->setFont( font );

	short highTemp(SHRT_MAX), lowTemp(SHRT_MAX);
	
	if (day->hasNightValues())
	{
		int iconSize = 0.4 * tempRect.height();
		if (iconSize < tempRect.width() * 0.7)
			iconSize = tempRect.width() * 0.8;
		
		const qreal rectCenter = tempRect.top() + tempRect.height() * 0.5;
		const qreal iconOffset = 0.25 * iconSize;
		const YawpWeather * weather = NULL;
		
		//--- draw night icon ---
		painter->save();
		weather = &day->nightWeather();
		lowTemp = weather->lowTemperature();
		painter->setOpacity( 0.5 );
		
		QRect iconRect(tempRect.right() - iconSize, rectCenter - iconOffset, iconSize, iconSize);
		drawImage( painter, iconRect, weather->iconName() );
		painter->restore();

		//--- draw day icon ---
		weather = &day->weather();
		highTemp = weather->highTemperature();
		
		iconRect = QRect(tempRect.left(), rectCenter + iconOffset - iconSize, iconSize, iconSize);
		drawImage( painter, iconRect, weather->iconName() );
	}
	else
	{
		const qreal iconSize = 0.60 * tempRect.height();
		const YawpWeather * weather = &day->weather();
		highTemp = weather->highTemperature();
		lowTemp = weather->lowTemperature();
		
		QRect iconRect( tempRect.left() + (tempRect.width() - iconSize) / 2.0,
		                tempRect.top() + (tempRect.height() - iconSize) / 2.0,
		                iconSize,
		                iconSize );
		drawImage( painter, iconRect, weather->iconName() );
	}
	
	//--- draw the high/low temperature ---
	if (highTemp < SHRT_MAX)
		drawText( painter, tempRect, Qt::AlignRight|Qt::AlignTop, QString::number(highTemp) + QChar(0xB0) );
	if (lowTemp < SHRT_MAX)
		drawGreyText( painter, tempRect, Qt::AlignLeft|Qt::AlignBottom, QString::number(lowTemp) + QChar(0xB0) );
}

void
DesktopPainter::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
	if (stateMachine()->serviceModel()->rowCount() == 0)
		return;
	
	const bool hasPreviewPage( stateMachine()->hasPage( Yawp::PreviewPage) );
	const bool hasDetailsPage( stateMachine()->hasPage( Yawp::DetailsPage) );
	const bool hasSatellitePage( stateMachine()->hasPage( Yawp::SatellitePage) );

	const QRectF rectPreviewPageButton = getPageButtonRect( contentsRect(),  Yawp::PreviewPage );
	const QRectF rectDetailsPageButton = getPageButtonRect( contentsRect(), Yawp::DetailsPage );
	const QRectF rectSatellitePageButton = getPageButtonRect( contentsRect(), Yawp::SatellitePage );
	
	const Yawp::PageType currentPage( stateMachine()->currentPage() );
	const int detailsDayIndex = stateMachine()->detailsDayIndex();
	const QRect detailsHeaderRect = getDetailsHeaderRect(contentsRect());
	
	const CityWeather * city = stateMachine()->currentCity();


	/***  Check if one of the page buttons (Preview, Details or Satellite Page) has been pressed ***
	 */
	//--- one of the first two buttons has been pressed ---
	if( (currentPage != Yawp::DetailsPage && hasDetailsPage && rectDetailsPageButton.contains(event->pos())) ||
	    (currentPage != Yawp::PreviewPage && hasPreviewPage && rectPreviewPageButton.contains(event->pos())) )
	{
		if( detailsDayIndex > 0 || currentPage == Yawp::SatellitePage )
		{
			Yawp::PageType pageType = (rectDetailsPageButton.contains(event->pos()) ? Yawp::DetailsPage : Yawp::PreviewPage);
			initPageChange( AbstractPainter::PageChangeAnimation, pageType, currentPage < pageType );
		}
		else
		{
			/*  just update the details rectangle since we see the first day
		         *  and the current page is either PreviewPage or DetailsPage.
		         */
			initPartChange(AbstractPainter::FullDetailsChange);
		}
		event->accept();
		return;

	}
	else if (hasSatellitePage && currentPage != Yawp::SatellitePage && rectSatellitePageButton.contains(event->pos()))
	{
		initPageChange( AbstractPainter::PageChangeAnimation, Yawp::SatellitePage, true );
		event->accept();
		return;
	}
	
	/*** Check if button to select previous or next city has been clicked ***
	 */
	const QRectF rectPrevCityButton = getCityButtonRect(contentsRect(), true);
	const QRectF rectNextCityButton = getCityButtonRect(contentsRect(), false);
	
	if (configData()->iCityIndex > 0 && rectPrevCityButton.contains(event->pos()))
	{
		initPageChange( AbstractPainter::CityChangeAnimation, configData()->iCityIndex - 1, false );
		event->accept();
		return;
	}
	else if (configData()->iCityIndex+1 < stateMachine()->serviceModel()->rowCount() && rectNextCityButton.contains(event->pos()))
	{
		initPageChange( AbstractPainter::CityChangeAnimation, configData()->iCityIndex + 1, true );
		event->accept();
		return;
	}

	/*** Check whether one of the weather icons has been clicked
	*/
	QRect iconRect = getTodaysWeatherIconRect(contentsRect());
	if ( (currentPage == Yawp::PreviewPage || currentPage == Yawp::DetailsPage) &&
	     iconRect.contains(event->pos().x(), event->pos().y()) )
	{
		int dayIndex = currentPage == Yawp::PreviewPage ? 0 : stateMachine()->detailsDayIndex();

		if (city->days().at(dayIndex)->hasNightValues())
			initWeatherIconChange(iconRect, dayIndex);
		event->accept();
		return;
	}
	
	if (currentPage == Yawp::PreviewPage)
	{
		/*** Check whether forecast header names has been clicked
		*/
		if (detailsHeaderRect.contains(event->pos().x(), event->pos().y()))
		{
			initPartChange(AbstractPainter::SlidingDayNames);
			event->accept();
			return;
		}
		
		int forecastCount = qMin(4, city->days().count());
		const QRect detailsContentsRect = getDetailsContentsRect(contentsRect());
		
		for (int dayIndex = 1; dayIndex <= forecastCount; ++dayIndex)
		{
			iconRect = getForecastWeatherIconRect(detailsContentsRect, dayIndex);
			if (iconRect.contains(event->pos().x(), event->pos().y()))
			{
				if (city->days().at(dayIndex)->hasNightValues())
					initWeatherIconChange(iconRect, dayIndex);
				event->accept();
				return;
			}
		}
	}
	else if (currentPage == Yawp::DetailsPage)
	{
		const QRect rectPreviousDayButton = getDetailsDayButtonRect(detailsHeaderRect, true);
		const QRect rectNextDayButton = getDetailsDayButtonRect(detailsHeaderRect, false);
		
		if (detailsDayIndex > 0 &&
		    rectPreviousDayButton.contains(event->pos().x(), event->pos().y()))
		{
			initPageChange(AbstractPainter::DayChangeAnimation, detailsDayIndex-1, false);
			event->accept();
			return;
		}
		else if (detailsDayIndex + 1 < stateMachine()->currentCity()->days().count() &&
		    rectNextDayButton.contains(event->pos().x(), event->pos().y()))
		{
			initPageChange(AbstractPainter::DayChangeAnimation, detailsDayIndex+1, true);
			event->accept();
			return;
		}
		else if (stateMachine()->maxPropertyPage() > 1 &&
		    getDetailsContentsRect(contentsRect()).contains(event->pos().x(), event->pos().y()))
		{
			initPartChange(AbstractPainter::DetailsChange);
			event->accept();
			return;
		}
	}
}


void
DesktopPainter::handleLayoutChanges()
{
	setButtonNames();
	m_sVisualCityName = createVisualCityName(stateMachine()->currentCity());
}

void
DesktopPainter::setButtonNames()
{
	m_vButtonNames.clear();
	const Plasma::Svg * pSvg = (configData()->bUseCustomTheme && customSvg() != 0 && customSvg()->isValid()) ? customSvg() : svg();
		
	if (!pSvg)
		return;
	
	if (pSvg->hasElement("actual"))
		m_vButtonNames << "actual";
	else
		m_vButtonNames << "map";
	
	if (pSvg->hasElement("info"))
		m_vButtonNames << "info";
	else
		m_vButtonNames << "map";
	
	m_vButtonNames << "map";
}

QString
DesktopPainter::createVisualCityName( const CityWeather * pCity ) const
{
	if (!pCity)
		return QString();

	//--- determine the cityname for visualisation, just make sure the name fits on the applet ---
	Plasma::Theme* theme = Plasma::Theme::defaultTheme();
	
	QFont font = theme->font(Plasma::Theme::DesktopFont);
	font.setPixelSize(17.0f);
	QFontMetrics fm (font);
	QString sCached;
	int iCacheWidth(0);
	QString sVisualName;

	
	/* we check if the update intervall is still valid, i added 1 minute to give the
	 * update job some time to update. Because the job will be normaly started when the update time
	 * has been past.
	 */
	if (pCity->days().count() > 0)
	{
		int iUpdateInterval = configData()->iUpdateInterval;

		if (!pCity->lastUpdate().isValid() ||
			(iUpdateInterval > 0 && pCity->lastUpdate().addMSecs((iUpdateInterval+1)*60*1000) < QDateTime::currentDateTime()) )
		{
			sCached = " (" + i18n("Cached") + ")";
			iCacheWidth = fm.width(sCached);
		}
	}
	sVisualName = fm.elidedText( pCity->localizedCityString(), Qt::ElideRight, 220 - iCacheWidth );
	sVisualName.append(sCached);
	return sVisualName;
}

QRect
DesktopPainter::getPageButtonRect (const QRect & contentsRect, Yawp::PageType pageType) const
{
	const qreal dScale = contentsRect.width() / YAWP_ORIG_SIZEX;
	int button = 0;
	
	if (pageType == Yawp::DetailsPage)
		button = 1;
	else if (pageType == Yawp::SatellitePage)
		button = 2;
	
	return QRect (qRound(contentsRect.left() + dScale + (27.0f * button * dScale)),
	              qRound(contentsRect.top() + dScale),
	              qRound(25.0f * dScale),
	              qRound(25.0f * dScale));
}

QRect
DesktopPainter::getCityButtonRect(const QRect & contentsRect, bool previous) const
{
	const qreal dScale = contentsRect.width() / YAWP_ORIG_SIZEX;
	const qreal yPos = stateMachine()->currentPage() == Yawp::SatellitePage ? 35.0f : 100.0f;
	const int buttonSize = qRound(16.0f * dScale);
	
	int xPos;
	if (previous)
		xPos = contentsRect.left();
	else
		xPos = contentsRect.right() - buttonSize;
	
	return QRect( xPos, qRound(contentsRect.top() + yPos * dScale), buttonSize, buttonSize );
}

QRect
DesktopPainter::getDetailsDayButtonRect(const QRect & detailsHeaderRect, bool previous) const
{
	const qreal dScale = detailsHeaderRect.width() / YAWP_ORIG_SIZEX;
	const qreal dButtonSize = qRound(20.0f * dScale);
	
	qreal dXPos;
	if (previous)
		dXPos = detailsHeaderRect.left() + (4.0f * dScale);
	else
		dXPos = detailsHeaderRect.right() - (4.0 * dScale + dButtonSize);
	
	return QRect( qRound(dXPos), qRound(detailsHeaderRect.top() + 9.0f * dScale), dButtonSize, dButtonSize );
}

QRect
DesktopPainter::getTodaysWeatherIconRect(const QRect & contentsRect) const
{
	const qreal dScale = contentsRect.width() / YAWP_ORIG_SIZEX;
	return QRect( qRound(contentsRect.left() + 85.0f * dScale),
		      qRound(contentsRect.top() + 3.0f * dScale),
		      qRound(88.0f * dScale),
		      qRound(88.0f * dScale) );
}

QRect
DesktopPainter::getForecastWeatherIconRect(const QRect & detailsContentsRect, int forecastIndex) const
{
	const CityWeather * city = stateMachine()->currentCity();
	if (city != 0 && forecastIndex >= 1 && forecastIndex <= 4)
	{
		const qreal dScale = detailsContentsRect.width() / YAWP_ORIG_SIZEY;
		
		const int forecastWeatherColumns = qMin(city->days().count() - 1, 4);
		const double dIconSize( 45.0f * dScale );
		const double dColumnWidth = detailsContentsRect.width() / forecastWeatherColumns;
		
		return QRect( qRound(detailsContentsRect.left() + float(forecastIndex-1) * dColumnWidth + (dColumnWidth-dIconSize)/2.0),
		              qRound(detailsContentsRect.top() + 7.0f * dScale),
		              qRound(dIconSize),
		              qRound(dIconSize) );
	}
	return QRect();
}

QRect
DesktopPainter::getDetailsHeaderRect(const QRect & contentsRect) const
{
	const qreal dScale = contentsRect.width() / YAWP_ORIG_SIZEX;
	return QRect( contentsRect.left(),
	              qRound(contentsRect.top() + 116 * dScale),
	              contentsRect.width(),
	              qRound(30.0f * dScale) );
}

QRect
DesktopPainter::getDetailsContentsRect(const QRect & contentsRect) const
{
	const qreal dScale = contentsRect.width() / YAWP_ORIG_SIZEX;
	return QRect( contentsRect.left(),
	              qRound(contentsRect.top() + 150.0f * dScale),
	              contentsRect.width(),
	              qRound(102.0f * dScale) );
}

void
DesktopPainter::initPartChange(AbstractPainter::AnimationType animationType)
{
	const QRect detailsHeaderRect = getDetailsHeaderRect(contentsRect());
	const QRect detailsContentsRect = getDetailsContentsRect(contentsRect());
	
	PageAnimator::Transition transition;
	Yawp::PageType currentPage = stateMachine()->currentPage();

	QRect animationRect;
	bool printForecastHeader = false;
	bool printForecastContent = false;
	bool printDetailsHeader = false;
	bool printDetailsContent = false;
	
	switch (animationType)
	{
	case AbstractPainter::SlidingDayNames:
		animationRect = detailsHeaderRect;
		printForecastHeader = true;
		transition = getSlidingTransition(configData()->daynamesAnimation, m_bDayNames);
		break;

	case AbstractPainter::FullDetailsChange:
		animationRect = detailsHeaderRect;
		animationRect.setBottom(detailsContentsRect.bottom());
		transition = getSlidingTransition(configData()->detailsAnimation, currentPage == Yawp::PreviewPage);
		
		printDetailsHeader = printDetailsContent = currentPage == Yawp::DetailsPage;
		printForecastHeader = printForecastContent = !printDetailsHeader;
		break;
		
	case AbstractPainter::DetailsChange:
		animationRect = detailsContentsRect;
		printDetailsContent = true;
		transition = configData()->detailsAnimation;
		break;
		
	default:
		dError() << "AnimationType: " << animationType << " is not supported!";
		return;
	}

	if (transition != PageAnimator::Jump)
	{
		int dayIndex = stateMachine()->detailsDayIndex();

		pageAnimator()->resetPages(animationRect.width(), animationRect.height());
		
		QPainter painter;
		painter.begin(&pageAnimator()->vPages[0]);
		painter.translate(-animationRect.left(), -animationRect.top());
		
		if (printForecastHeader)
			drawForecastHeader(&painter, detailsHeaderRect);
		else if (printDetailsHeader)
			drawDetailsHeader(&painter, dayIndex, detailsHeaderRect);

		if (printForecastContent)
			drawForecast(&painter, detailsContentsRect);
		else if (printDetailsContent)
			drawDetails(&painter, dayIndex, detailsContentsRect);
		painter.end();
	}
	
	//--- switch state ---
	switch (animationType)
	{
	case AbstractPainter::SlidingDayNames:
		m_bDayNames = !m_bDayNames;
		break;
	
	case AbstractPainter::FullDetailsChange:
		stateMachine()->setCurrentPage(currentPage == Yawp::DetailsPage ? Yawp::PreviewPage : Yawp::DetailsPage);
		printDetailsHeader = printDetailsContent = !printDetailsHeader;
		printForecastHeader = printForecastContent = !printDetailsHeader;
		break;
	
	case AbstractPainter::DetailsChange:
		stateMachine()->setCurrentPropertyPage(stateMachine()->currentPropertyPage() + 1, true);
		break;
		
	default:
		break;
	}
	
	if (transition != PageAnimator::Jump)
	{
		int dayIndex = stateMachine()->detailsDayIndex();

		QPainter painter;
		painter.begin(&pageAnimator()->vPages[1]);
		painter.translate(-animationRect.left(), -animationRect.top());
		
		if (printForecastHeader)
			drawForecastHeader(&painter, detailsHeaderRect);
		else if (printDetailsHeader)
			drawDetailsHeader(&painter, dayIndex, detailsHeaderRect);

		if (printForecastContent)
			drawForecast(&painter, detailsContentsRect);
		else if (printDetailsContent)
			drawDetails(&painter, dayIndex, detailsContentsRect);
		painter.end();
		
		//--- setup animation ---
		setAnimationType(animationType);
		pageAnimator()->setTransition(transition);
		setAnimationRect(animationRect);

		updatePixmap(false);

		timeLine()->start();
	}
	else
		//--- When no animation has been configured than just print the new applet state ---
		updatePixmap();
}

void
DesktopPainter::initWeatherIconAnimation(int dayIndex)
{
	Yawp::PageType currentPage = stateMachine()->currentPage();
	QRect iconRect;
	
	if (currentPage == Yawp::PreviewPage)
	{
		if (dayIndex == 0)
			iconRect = getTodaysWeatherIconRect(contentsRect());
		else
			iconRect = getForecastWeatherIconRect(getDetailsContentsRect(contentsRect()), dayIndex);
	}
	else if (currentPage == Yawp::DetailsPage && dayIndex == stateMachine()->detailsDayIndex())
	{
		iconRect = getTodaysWeatherIconRect(contentsRect());
	}

	if (iconRect.isValid())
	{
		initWeatherIconChange(iconRect, dayIndex);
	}
	else
	{
		setAnimationType(DummyAnimation);
		timeLine()->start();
		
		emit signalToggleWeatherIcon(dayIndex);
	}
}

QPixmap
DesktopPainter::createSimpleToolTip(const QSize & defaultSize) const
{
	QSize size = getSize(defaultSize);
	QPixmap pix(size.width(), size.height() * 0.34);
	pix.fill(Qt::transparent);
	
	QPainter painter(&pix);
	Plasma::Theme * theme = Plasma::Theme::defaultTheme();
	QFont font = theme->font(Plasma::Theme::DefaultFont);
	painter.setFont(font);

	drawTopWeatherInfo(&painter, 0, QRect(QPoint(0, 0), size), true);
	
	return pix;
}

QPixmap
DesktopPainter::createExtendedToolTip(const QSize & defaultSize) const
{
	QSize size = getSize(defaultSize);
	QRect contentsRect(0, 0, size.width(), size.height());
	
	QPixmap pix(size);
	pix.fill(Qt::transparent);
	
	QPainter painter(&pix);
	Plasma::Theme * theme = Plasma::Theme::defaultTheme();
	QFont font = theme->font(Plasma::Theme::DefaultFont);
	painter.setFont(font);
	
	if (configData()->extendedTooltipOptions.testFlag(Yawp::ThemeBackground))
	{
		drawBackground(&painter, contentsRect);
		contentsRect = getContentsRect(contentsRect);
	}
	else
	{
		QString backgroundName = QLatin1String("back-default");
		Plasma::Svg * currentSvg = svg();

		if (currentSvg != 0 &&
		    currentSvg->isValid() &&
		    currentSvg->hasElement(backgroundName))
		{
			currentSvg->paint(&painter, contentsRect, backgroundName);
		}
	}
	
	painter.save();
	// since we do not show the city name above the details rectangle,
	// we move the top a little bit lower to make it look nicer :)
	painter.translate(0, 0.05 * contentsRect.height());
	drawTopWeatherInfo(&painter, 0, contentsRect, true);
	painter.restore();

	// when we got a valid weather, than we have a valid city as well
	// so we do not have to check it anymore
	drawForecastHeader(&painter, getDetailsHeaderRect(contentsRect));
	drawForecast(&painter, getDetailsContentsRect(contentsRect), true);
	
	return pix;
}
