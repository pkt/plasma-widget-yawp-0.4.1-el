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

#ifndef ABSTRACT_PAINTER_H
#define ABSTRACT_PAINTER_H

//--- LOCAL CLASSES ---
#include "pageanimator.h"
#include "statemachine.h"
#include "yawpdefines.h"

//--- QT4 CLASSES ---
#include <QGraphicsWidget>
#include <QObject>
#include <QPainter>
#include <QTimeLine>

//--- KDE4 CLASSES ---
#include <Plasma/Applet>

class AbstractPainter : public QObject
{
	 Q_OBJECT
public:
	enum PageAnimationTypeFlags { CityChangeAnimation, DayChangeAnimation, PageChangeAnimation };
	Q_DECLARE_FLAGS( PageAnimationType, PageAnimationTypeFlags );

	/*   we use the AnimationType as Index in m_vAnimationData,
	 *   since we need space for 5 days SlidingDayNames got index 6
	 *   we will waste one bit, but we need NoAnimation as well.
	 */
	enum AnimationType
	{
		NoAnimation=0,
		
		/* Desktop Interface (Desktop Painter) needs to set the animation type to dummy
		 * when public methods initCityChangeAnimation() and initWeatherIconAnimation()
		 * will be called by panelPainter, but the desktop painter does not show
		 * meet the requirements (does not has a size or does not show the specific weather icon).
		 * In this case we set the dummy animation and trigger the timeLine to stop interacting
		 * with the user to give the panelPainter its time to do the animation.
		 */
		DummyAnimation,
		SlidingDayNames,
		PageChange,
		DetailsChange,
		FullDetailsChange,
		IconAnimation
	};

	virtual ~AbstractPainter();

	virtual Plasma::FormFactor formFactor() = 0;
	
	// calculates proper size depending on current size
	virtual QSize getSize(const QSizeF & currentSize) const = 0;
	void paintApplet (
		QPainter * painter,
		const QStyleOptionGraphicsItem * option,
		const QRect & contentsRect);
	
	virtual void mousePressEvent (QGraphicsSceneMouseEvent * event) = 0;
	
	void setSvg (Plasma::Svg * svg);
	void setCustomSvg (Plasma::Svg * svg);
	
	// sets the flag that layout has been changed and needs to recalculated
	virtual void update();
	virtual void setupAnimationTimeLine();
	
	QTimeLine * timeLine() const;
	QGraphicsWidget * widget() const;
	
	bool isBusy() const;
	
	/** public method to initialize city change animation, current city will change to city index */
	virtual void initCityChangeAnimation(int cityIndex);
	
	/** public method to initialize weather icon change animation for given day index. */
	virtual void initWeatherIconAnimation(int dayIndex) = 0;

	inline StateMachine * stateMachine() const {return m_stateMachine;}
	inline Plasma::Svg * svg() const {return m_svg;}
	inline Plasma::Svg * customSvg() const {return m_customSvg;}
	inline const Yawp::ConfigData * configData() const {return m_configData;}
	inline const QRect contentsRect() const {return m_contentsRect;}
	inline AnimationType animationType() const {return m_animationType;}

public slots:
	virtual void setBusy(bool busy);

signals:
	void signalCityChanged(int cityIndex);
	void signalToggleWeatherIcon(int dayIndex);
	
protected slots:
	void updatePixmap(bool updateWidget = true);

protected:
	AbstractPainter (
		QGraphicsWidget * widget, // this is the widget we paint on
		const Yawp::ConfigData * configData,
		StateMachine * stateMachine);

	inline void setAnimationType(AnimationType type) {m_animationType = type;}
	
	inline int animationIndex() const {return m_animationIndex;}
	inline void setAnimationRect(const QRect & rect) {m_animationRect = rect;}
	
	inline PageAnimator * pageAnimator() {return &m_pageAnimator;}
	
	/** Returns the content rectangle and padding (space for frame) depending of given total content rectangle. */
	virtual QRect getContentsRect(const QRect & contentsRect) const = 0;
	
	void initPageChange(PageAnimationType pageAnimationType, int pageValue, bool forward);
	void initWeatherIconChange(const QRect & rect, int dayIndex, bool showCurrent = true);
	
	/** This method will be called to update internal state of derivative classes
	*   when background has been changed, SVG-File changed, or city name, ...
	*/
	virtual void handleLayoutChanges() = 0;
	
	/** Paint the background - this method will be called during paint event and can/should be used to
	 *  draw the background (desktop-painter) because the background is not part of animation sequence.
	 */
	virtual void drawBackground(QPainter * painter, const QRect & contentsRect) const;
	
	/** Method to paint current applet content (without background). */
	virtual void drawPage(QPainter * painter, const QRect & contentsRect) const = 0;
	
	/** Draws the weather icon for the given weather, using the given painter, at given position iconRect.
	 *  Each weather has two icons (current and forecast) when currentIcon = true, we will paint
	 * the current icon if available. Fallback solution is always to paint the forecast icon.
	 */
	void drawWeatherIcon(QPainter * painter, const YawpWeather * weather, const QRect & iconRect, bool currentIcon = true) const;
	
	void drawImage (QPainter * painter, const QRect & rect, const QString & name) const;
	void drawText( QPainter * painter, const QRect & rect, int align, const QString & text ) const;
	void drawGreyText( QPainter * painter, const QRect & rect, int align, const QString & text ) const;
	
	PageAnimator::Transition getSlidingTransition( const PageAnimator::Transition tr, const bool forward ) const;


private slots:
	void animationTimeout(int frame);
	void animationFinished();

private:
	QGraphicsWidget * m_widget;
	StateMachine * m_stateMachine;
	const Yawp::ConfigData * m_configData;
	
	Plasma::Svg * m_svg;
	Plasma::Svg * m_customSvg;
	
	bool m_updateRequired;
	QRect m_contentsRect;
	
	PageAnimator m_pageAnimator;

	QPixmap m_appletContent;
	QTimeLine m_timeLine;
	AnimationType m_animationType;	// current active animation, or NoAnimation
	int m_animationIndex;
	QRect m_animationRect;
	
	bool m_bIsBusy;
	
	int m_lastFrame;
};

#endif // ABSTRACT_PAINTER_H
