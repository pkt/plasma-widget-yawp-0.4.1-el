/*************************************************************************\
*   Copyright (C) 2009 - 2011 by Ulf Kreissig                             *
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

#ifndef YET_ANOTHER_WEATHER_PLASMOID_H
#define YET_ANOTHER_WEATHER_PLASMOID_H

//--- LOCAL CLASSES ---
#include "painter/abstractpainter.h"
#include "statemachine.h"
#include "pageanimator.h"
#include "weatherservice.h"
#include "yawpday.h"
#include "configdialog/yawpconfig.h"

//--- QT4 CLASSES ---
#include <QBasicTimer>
#include <QGraphicsSceneMouseEvent>
#include <QSvgRenderer>
#include <QTimerEvent>
#include <QTimeLine>

//--- KDE4 CLASSES ---
#include <Plasma/PopupApplet>
#include <Plasma/ExtenderItem>
#include <Plasma/Svg>


class QActionGroup;
class QFont;
class QPainter;
class QRect;
class QTimer;

class KActionMenu;


class YaWP : public Plasma::PopupApplet
{
	Q_OBJECT

public:
	YaWP( QObject * parent, const QVariantList & args );
	virtual ~YaWP();

	void constraintsEvent( Plasma::Constraints constraints );

	QList<QAction *> contextualActions() { return m_actions; }

	/**
	 * This method is called once the applet is loaded and added to a Corona.
	 * If the applet requires a QGraphicsScene or has an particularly intensive
	 * set of initialization routines to go through, consider implementing it
	 * in this method instead of the constructor.
	 *
	 * Note: paintInterface may get called before init() depending on initialization
	 * order. Painting is managed by the canvas (QGraphisScene), and may schedule a
	 * paint event prior to init() being called.
	 **/
	virtual void init();

	// The paintInterface procedure paints the applet to screen
	void paintInterface( QPainter * painter,
	                     const QStyleOptionGraphicsItem * option,
	                     const QRect & contentsRect );

	/**
	 * Reimplement this method so provide a configuration interface,
	 * parented to the supplied widget. Ownership of the widgets is passed
	 * to the parent widget.
	 *
	 * @param parent the dialog which is the parent of the configuration
	 *               widgets
	 */
	virtual void createConfigurationInterface( KConfigDialog * parent );

protected:
	virtual void mousePressEvent( QGraphicsSceneMouseEvent * event );
	virtual void timerEvent ( QTimerEvent * event );
	virtual void initExtenderItem(Plasma::ExtenderItem * item);

private slots:
	void about() const;
	
	void slotCityUpdate( WeatherServiceModel::ServiceUpdate updateType );
	void changeCity(QAction * action);
	
	void openForecastUrl();

	void animationFinished();
	void configAccepted();	//--- Called when applet configuration values has been changed. ---

	void slotThemeChanged();
	void setCityIndex( int iIndex );
	void slotToggleWeatherIcon(int dayIndex);
	
	// Slot to clean up cache directory
	void slotStartCacheCleanUp();

private:
	void updateSize();
	
	void loadConfig();
	void loadCustomTheme();
	void saveConfig();

	void setDefaultFontColors();

	void updateCitySubMenu();
	
	void initAppletPainter();
	
	void createExtenderItem();
	void destroyExtenderItem();
	
	void createPanelTooltip();

	void setupWeatherServiceModel();
	void stopPendingEngineConnection();
	void startTraverseLocationTimeout();
	void stopTraverseLocationTimeout();
	
private:
	Plasma::Svg                  m_svg;
	Plasma::Svg                  m_customSvg;

	QPointer<YawpConfigDialog>   m_pConfigDlg;
	Yawp::ConfigData             m_configData;
	Yawp::Storage                m_storage;

	AbstractPainter * m_pAppletPainter;

	/**   Model which holds and controls the weatherdata of all cities */
	WeatherServiceModel        * m_pWeatherModel;

	/**   State machine that controls the current state of yawp and incloses the logic in one class. */
	StateMachine                 m_stateMachine;

	KAboutData                 * m_aboutData;

	 /**   Contextual actions - this list holds all actions we want to show in a popup menu */
	QList<QAction *>             m_actions;

	/**   We need this private member variable to disable the menu-entry, while we are updating the data.
	 */
	QAction                    * m_pManualUpdate;

	/**   This list holds a action for all currently configured city, so that the user can change
	 *    the currently shown city using the popup-menu.
	 */
	KActionMenu                * m_pCitySubMenu;
	
	/** This action will open the credit url of current location in a webbrowser.
	 */
	QAction                    * m_pOpenForecastUrl;

	/**   Controlls the actions in the m_citySubMenu to make them exclusive-checkable */
	QActionGroup               * m_pGrpActionCities;
	
	// timer-id for a pending connecion to the weather engine
	int                          m_iIdPendingEngineConnection;
	
	// timer-id for changing city periodically
	int                          m_iIdTraverseLocations;
};

#endif
