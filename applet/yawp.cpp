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

#define PANEL_DESKTOP_INTERFACE_NAME	QLatin1String("panel-desktop-interface")


//--- LOCAL CLASSES ---
#include "../config.h"
#include "painter/abstractpainter.h"
#include "painter/desktoppainter.h"
#include "painter/panelpainter.h"
#include "countrymap.h"
#include "ionlistmodel.h"
#include "paneldesktopinterface.h"
#include "yawp.h"
#include "yawpday.h"
#include "weatherdataprocessor.h"
#include "weatherservice.h"
#include "utils.h"

#include "logger/streamlogger.h"

//--- QT4 CLASSES ---
#include <QActionGroup>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QTimer>

//--- KDE4 CLASSES ---
#include <KAboutData>
#include <KAboutApplicationDialog>
#include <KActionMenu>
#include <KColorScheme>
#include <KConfigDialog>
#include <KDateTime>
#include <KIcon>
#include <KIO/DeleteJob>
#include <KMenu>
#include <KTimeZone>
#include <KSystemTimeZone>
#include <KUrl>

#include <Plasma/Extender>
#include <Plasma/ToolTipManager>
#include <Plasma/Theme>

#if KDE_IS_VERSION(4,3,70)
	#include <KUnitConversion/Value>
#endif

#include <limits.h>
#include <math.h>


//	cp lib/plasma_yawp3.so /usr/lib/kde4/
//	cp plasma_yawp3.desktop /usr/share/kde4/services/plasma-yawp3.desktop
//
//	plasmoidviewer -c desktop -f horizontal yaWP3


YaWP::YaWP( QObject * parent, const QVariantList & args )
	: Plasma::PopupApplet(parent, args),
	  m_svg(this),
	  m_customSvg(this),
	  m_pConfigDlg(NULL),
	  m_pAppletPainter(0),
	  m_iIdPendingEngineConnection(-1),
	  m_iIdTraverseLocations(-1)
{
	Q_INIT_RESOURCE(yawpresource);
	dStartFunct() << "============================================";
	dInfo();
	dInfo() << "yaWP" << YAWP_VERSION_STRING << "compiled at" << __DATE__ << __TIME__ << "for KDE" << KDE_VERSION_STRING;
	dInfo() << "yaWP" << YAWP_VERSION_STRING << "is running on KDE" << KDE::versionString();
	dInfo();

	Plasma::DataEngine * pEngine = dataEngine("weather");
	m_storage.setEngine( pEngine );
;
	WeatherDataProcessor * pDataProcessor = new WeatherDataProcessor( &m_storage );
	m_pWeatherModel = new WeatherServiceModel( &m_storage, this, pDataProcessor );
	m_pWeatherModel->setObjectName("EngineModel");

	m_stateMachine.setServiceModel( m_pWeatherModel );

	//--- Set Default Configuration Values ---
	m_configData.iCityIndex = 0;
	m_configData.bUseCustomTheme = false;
	m_configData.bUseCustomThemeBackground = false;
	m_configData.bUseCustomFontColor = false;
	m_configData.bUseInteractivePanelWeatherIcons = true;
	m_configData.bDisableTextShadows = false;
	m_configData.iUpdateInterval = 45; // in minutes
	m_configData.iStartDelay = 0; // in minutes
	m_configData.iAnimationDuration = 1000;
	m_configData.sBackgroundName = QLatin1String("default");

	if( KGlobal::locale()->measureSystem() == KLocale::Metric )
	{
	#if KDE_IS_VERSION(4,3,70)
			m_configData.distanceSystem = KUnitConversion::Kilometer;
			m_configData.pressureSystem = KUnitConversion::Kilopascal;
			m_configData.temperatureSystem	= KUnitConversion::Celsius;
			m_configData.speedSystem	= KUnitConversion::KilometerPerHour;
	#else
			m_configData.distanceSystem = WeatherUtils::Kilometers;
			m_configData.pressureSystem = WeatherUtils::Kilopascals;
			m_configData.temperatureSystem	= WeatherUtils::Celsius;
		#if KDE_VERSION_MINOR >= 3
			m_configData.speedSystem = WeatherUtils::KilometersPerHour;
		#else
			m_configData.speedSystem = WeatherUtils::KilometersAnHour;
		#endif
	#endif
	}
	else
	{
	#if KDE_IS_VERSION(4,3,70)
		m_configData.distanceSystem = KUnitConversion::Mile;
		m_configData.pressureSystem = KUnitConversion::InchesOfMercury;
		m_configData.temperatureSystem = KUnitConversion::Fahrenheit;
		m_configData.speedSystem = KUnitConversion::MilePerHour;
	#else
		m_configData.distanceSystem = WeatherUtils::Miles;
		m_configData.pressureSystem = WeatherUtils::InchesHG;
		m_configData.temperatureSystem = WeatherUtils::Fahrenheit;
		#if KDE_VERSION_MINOR >= 3
			m_configData.speedSystem = WeatherUtils::MilesPerHour;
		#else
			m_configData.speedSystem = WeatherUtils::MilesAnHour;
		#endif
	#endif
	}

	m_configData.todaysWeatherPanelFormat	= Yawp::PanelTemperature | Yawp::PanelIcon;
	m_configData.forecastWeatherPanelFormat	= Yawp::PanelTemperature | Yawp::PanelIcon;
	m_configData.iPanelForecastDays = 3;

	m_configData.pageAnimation = PageAnimator::RollOutHorizontally;
	m_configData.daynamesAnimation = PageAnimator::RollOutHorizontally;
	m_configData.detailsAnimation = PageAnimator::CrossFade;
	m_configData.iconAnimation = PageAnimator::FlipVertically;

	m_configData.fontColor = QColor(Qt::white);
	m_configData.lowFontColor = QColor(Qt::gray);
	m_configData.shadowsFontColor = QColor(0,0,0,100);

	m_configData.bUseCompactPanelLayout = false;
	m_configData.bUseExtendedTooltip = true;
	m_configData.extendedTooltipOptions = Yawp::PreviewPage | Yawp::SatellitePage;

	m_configData.bTraverseLocationsPeriodically = false;
	m_configData.iTraverseLocationTimeout = 30;

	m_configData.vDetailsPropertyRankingList
		<< Yawp::WeatherDescription
		<< Yawp::SunriseSunset
		<< Yawp::RealfeelTemperature
		<< Yawp::Pressure
		<< Yawp::Visibility
		<< Yawp::Dewpoint
		<< Yawp::UV;

	// Contextual actions - thanks to Ezequiel, ported from stdin plasmoid :-)
	m_pManualUpdate = new QAction( i18n ("&Refresh"), this );
	m_pManualUpdate->setIcon ( KIcon ( "view-refresh" ) );
	addAction( "refresh", m_pManualUpdate );
	connect ( m_pManualUpdate, SIGNAL(triggered()), m_pWeatherModel, SLOT(reconnectEngine()) );

	QAction * aboutAction = new QAction(i18n("&About"), this);
	aboutAction->setIcon( KIcon("help-about") );
	addAction( "about", aboutAction );
	connect( aboutAction, SIGNAL(triggered()), this, SLOT(about()) );

	QAction * separator1 = new QAction ( this );
	separator1->setSeparator ( true );

	m_pCitySubMenu = new KActionMenu(KIcon("preferences-desktop-locale"), i18n("Show city"), this);
	m_pCitySubMenu->setEnabled( false );
	m_pGrpActionCities = new QActionGroup( this );
	m_pGrpActionCities->setExclusive( true );
	connect( m_pGrpActionCities, SIGNAL(triggered(QAction *)), this, SLOT(changeCity(QAction *)) );
	
	m_pOpenForecastUrl = new QAction( i18n("Open Forecast URL"), this);
	m_pOpenForecastUrl->setIcon( KIcon("text-html") );
	//addAction( "open_url", m_pOpenForecastUrl);
	connect( m_pOpenForecastUrl, SIGNAL(triggered()), this, SLOT(openForecastUrl()) );
	m_pOpenForecastUrl->setEnabled(false);

	QAction * separator2 = new QAction ( this );
	separator2->setSeparator ( true );

	m_actions.append( m_pManualUpdate );
	m_actions.append( aboutAction );
	m_actions.append( separator1 );
	m_actions.append( m_pCitySubMenu );
	m_actions.append( m_pOpenForecastUrl );
	m_actions.append( separator2 );

	/*** About data ***/
	m_aboutData = new KAboutData ( "plasma_yawp",
		QByteArray (), ki18n( YAWP_NAME ), YAWP_VERSION_STRING,
		ki18n( "Yet Another Weather Applet" ),
		KAboutData::License_GPL,
		ki18n( "Copyright (C) 2008-2010 Ruan Strydom\n"
		       "Copyright (C) 2008-2010 Ezequiel R. Aguerre\n"
		       "Copyright (C) 2008-2010 Pierpaolo Vittorini\n"
		       "Copyright (C) 2008-2011 Marián Kyral\n"
		       "Copyright (C) 2009-2011 Ulf Kreißig" ),
		ki18n( "This plasmoid shows for the selected place "
		       " current weather and forecast for 4 days." ),
		"http://www.kde-look.org/content/show.php?content=94106",
		"plasmafactory@jcell.co.za" );

	//--- Authors ---
	m_aboutData->addAuthor( ki18n("Ulf Kreißig"),
		ki18n("Developer"), "udev@gmx.net" );
	m_aboutData->addAuthor( ki18n("Marián Kyral" ),
		ki18n("Developer"), "mkyral@email.cz" );
	m_aboutData->addAuthor( ki18n("Ezequiel R. Aguerre"),
		ki18n("Developer"), "ezeaguerre@gmail.com" );
	m_aboutData->addAuthor( ki18n("Pierpaolo Vittorini"),
		ki18n("Developer"), "pierpaolo.vittorini@gmail.com" );
	m_aboutData->addAuthor( ki18n("Ruan Strydom"),
		ki18n("Developer"), "ruans@kr8.co.za" );

/*
      Copyright 2007 Thomas Luebking <thomas.luebking@web.de>
      Copyright 2008 Aleix Pol <aleixpol@gmail.com>
      Copyright 2007 Frerich Raabe <raabe@kde.org>
      Copyright 2007 Aaron Seigo <aseigo@kde.org>
*/

	//--- Credits ---
	m_aboutData->addCredit ( ki18n("Ulf Kreißig"),
		ki18n("Complete rewrite!" ), "udev@gmx.net" );
	m_aboutData->addCredit ( ki18n("Ruan Strydom"),
		ki18n("Idea, graphics" ), "ruans@kr8.co.za" );
	m_aboutData->addCredit( ki18n("AccuWeather.com"),
		ki18n("For the weather data"), "", "http://www.accuweather.com"  );
	m_aboutData->addCredit( ki18n("Thomas Luebking"),
		ki18n("Algorithm for animated page-changes is a slightly modified version from Bespin. (Qt4 Style)"),
		"thomas.luebking@web.de" );
	m_aboutData->addCredit( ki18n("das Gnu"),
		ki18n("German translation"), "dasgnu@gmail.com" );
	m_aboutData->addCredit( ki18n("Jesús Vidal Panalés"),
		ki18n("Spanish translation"));
	m_aboutData->addCredit( ki18n("Bribanick Dominique"),
		ki18n("French translation"), "chepioq@gmail.com" );
	m_aboutData->addCredit( ki18n("Mihail Pisanov"),
		ki18n("Russian translation"), "miha@52.ru" );
	m_aboutData->addCredit( ki18n("Richard Frič"),
		ki18n("Slovak translation"), "Richard.Fric@kdemail.net" );
	m_aboutData->addCredit( ki18n("Mihael Simonič"),
		ki18n("Slovenian translation"), "smihael@gmail.com" );
	m_aboutData->addCredit( ki18n("Maciej Bulik"),
		ki18n("Polish translation"), "Maciej.Bulik@post.pl" );
        m_aboutData->addCredit( ki18n("Hasan Kiran"),
                ki18n("Turkish translation"), "under67@hotmail.com" );
	m_aboutData->addCredit( ki18n("All people reporting bugs, send feedback and new feature requests") );

	m_aboutData->setProgramIconName("weather-clear");

	//--- Translators ---
	m_aboutData->setTranslator( ki18nc("NAME OF THE TRANSLATORS", "Your names"),
		ki18nc("EMAIL OF THE TRANSLATORS", "Your emails") );
	/*** About data end ***/

	connect( m_pWeatherModel,   SIGNAL(isBusy(bool)),       this,   SLOT(setBusy(bool)) );
	connect( m_pWeatherModel,   SIGNAL(cityUpdated(WeatherServiceModel::ServiceUpdate)),
	         this,              SLOT(slotCityUpdate(WeatherServiceModel::ServiceUpdate)) );
	connect( Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()),	this,	SLOT(slotThemeChanged()) );

	setHasConfigurationInterface(true);
	setAspectRatioMode(Plasma::KeepAspectRatio);
	setPassivePopup(true);
	
	resize(273, 255);	// set a fixed initial size that fits the desktop mode. We simply suppose we are always in desktop-mode.

	dEndFunct();
}

YaWP::~YaWP()
{
	dStartFunct();
	Plasma::ToolTipManager::self()->clearContent(this);

	destroyExtenderItem();

	if (hasFailedToLaunch())
	{
		// Do some cleanup here
	}
	else
	{
		// Save settings
		saveConfig();
	}

	if (m_pAppletPainter != 0)
		delete m_pAppletPainter;

	delete m_pWeatherModel;
	delete m_aboutData;

	dEndFunct();
}

void
YaWP::init()
{
	dStartFunct();
	m_svg.setImagePath("widgets/yawp_theme15");
	m_svg.setContainsMultipleImages( true );

	loadConfig();
	loadCustomTheme();
	initAppletPainter();

	//--- just 4 debug ---
/*	CityWeather dummyCity1;
	dummyCity1.setProvider(QString("accuweather"));
	dummyCity1.setCity("Dresden, Germany (Saxony)");
	dummyCity1.setCountry("Germany");
	dummyCity1.setCountryCode("de");
	dummyCity1.setExtraData("EUR.DE.GM014.DRESDEN");
	dummyCity1.setTimeZone("Europe/Berlin");
	m_pWeatherModel->addCity(dummyCity1);

	CityWeather dummyCity2;
	dummyCity2.setProvider(QString("accuweather"));
	dummyCity2.setCity("Shanghai, China(Shanghai)");
	dummyCity2.setCountry("China");
	dummyCity2.setCountryCode("cn");
	dummyCity2.setExtraData("ASI.CN.CH024.SHANGHAI");
	dummyCity2.setTimeZone("Asia/Shanghai");
	m_pWeatherModel->addCity(dummyCity2);
	//--- end of debug section ---
*/
	updateCitySubMenu();
	setCityIndex( m_configData.iCityIndex );

	const bool bHasCities = (m_pWeatherModel->rowCount() > 0);

	if (bHasCities)
		m_iIdPendingEngineConnection = startTimer( m_configData.iStartDelay*60*1000 );
	else
		setConfigurationRequired( true );

	m_pManualUpdate->setEnabled( bHasCities );
	m_pCitySubMenu->setEnabled( bHasCities);

	startTraverseLocationTimeout();

	// Every 5 days, 15 minutes after start of applet, cache directory will be checked to remove old files.
	if (QDate::currentDate().dayOfYear() % 5 == 0)
		QTimer::singleShot(15*60*1000, this, SLOT(slotStartCacheCleanUp()) );

	PopupApplet::init();
	
	dEndFunct();
}

void
YaWP::loadConfig()
{
	dStartFunct();
	KConfigGroup cfg = config();

	/*** SETTINGS PAGE ***/
	m_configData.iUpdateInterval
		= cfg.readEntry( "update interval", QVariant(m_configData.iUpdateInterval) ).toInt();
	m_configData.iStartDelay
		= cfg.readEntry( "start delay", QVariant(m_configData.iStartDelay) ).toInt();
	m_configData.bTraverseLocationsPeriodically
		= cfg.readEntry( "traverse locations", m_configData.bTraverseLocationsPeriodically );
	m_configData.iTraverseLocationTimeout
		= cfg.readEntry( "traverse locations timeout", QVariant(m_configData.iTraverseLocationTimeout) ).toInt();

	m_configData.distanceSystem
		= (YAWP_DISTANCE_UNIT)cfg.readEntry( "system.distance",
			(int)m_configData.distanceSystem );
	m_configData.pressureSystem
		= (YAWP_PRESSURE_UNIT)cfg.readEntry( "system.pressure",
		 	(int)m_configData.pressureSystem );
	m_configData.temperatureSystem
		= (YAWP_TEMPERATURE_UNIT)cfg.readEntry( "system.temperature",
			(int)m_configData.temperatureSystem );
	m_configData.speedSystem
		= (YAWP_SPEED_UNIT)cfg.readEntry( "system.speed",
			(int)m_configData.speedSystem );

	m_configData.daynamesAnimation
		= (PageAnimator::Transition)cfg.readEntry( "animation.daysnames",
			(int)m_configData.daynamesAnimation );
	m_configData.detailsAnimation
		= (PageAnimator::Transition)cfg.readEntry( "animation.details",
			(int)m_configData.detailsAnimation );
	m_configData.pageAnimation
		= (PageAnimator::Transition)cfg.readEntry( "animation.page",
			(int)m_configData.pageAnimation );
	m_configData.iconAnimation
		= (PageAnimator::Transition)cfg.readEntry( "animation.icon",
			(int)m_configData.iconAnimation );
	m_configData.iAnimationDuration
		= cfg.readEntry( "animation.duration",
			(int)m_configData.iAnimationDuration );

	/*** PANEL PAGE ***/
	m_configData.todaysWeatherPanelFormat
		= (Yawp::PanelDayFormat)cfg.readEntry( "panel.today.format",
			(int)m_configData.todaysWeatherPanelFormat );
	m_configData.forecastWeatherPanelFormat
		= (Yawp::PanelDayFormat)cfg.readEntry( "panel.forecast.format",
			(int)m_configData.forecastWeatherPanelFormat );
	m_configData.iPanelForecastDays
		= cfg.readEntry( "panel.forecast.days",	m_configData.iPanelForecastDays );
	m_configData.bUseCompactPanelLayout
		= cfg.readEntry( "panel.layout.compact", m_configData.bUseCompactPanelLayout );
	m_configData.bUseInteractivePanelWeatherIcons
		= cfg.readEntry( "panel.weathericon.interactive", m_configData.bUseInteractivePanelWeatherIcons );

	/*** PANEL TOOLTIP ***/
	m_configData.bUseExtendedTooltip
		= cfg.readEntry( "panel.tooltip.extended.enabled", m_configData.bUseExtendedTooltip );
	m_configData.extendedTooltipOptions
		= (Yawp::ExtendedTooltipOptions)cfg.readEntry( "panel.tooltip.extended.format",
			(int)m_configData.extendedTooltipOptions );

	/*** THEME PAGE ***/
	m_configData.sBackgroundName
		= cfg.readEntry( "theme", m_configData.sBackgroundName );
	m_configData.sCustomThemeFile
		= cfg.readEntry( "custom.theme.file", m_configData.sCustomThemeFile );
	m_configData.bUseCustomTheme
		= cfg.readEntry( "custom.theme.enabled", m_configData.bUseCustomTheme );
	m_configData.bUseCustomThemeBackground
		= cfg.readEntry( "custom.theme.background.enabled",
			QVariant( m_configData.bUseCustomThemeBackground ) ).toBool();

	m_configData.fontColor
		= cfg.readEntry( "custom.fontcolor.normal", m_configData.fontColor );
	m_configData.lowFontColor
		= cfg.readEntry( "custom.fontcolor.lowtemp", m_configData.lowFontColor );
	m_configData.shadowsFontColor
		= cfg.readEntry( "custom.fontcolor.shadows", m_configData.shadowsFontColor );
	m_configData.bUseCustomFontColor
		= cfg.readEntry( "custom.fontcolor.usage", m_configData.bUseCustomFontColor );
	m_configData.bDisableTextShadows
		= cfg.readEntry( "text.shadows", m_configData.bDisableTextShadows );
	if( !m_configData.bUseCustomFontColor )
		setDefaultFontColors();

	setupWeatherServiceModel();

	/*** LOCATION PAGE ***/
	if( cfg.hasGroup("locations") )
	{
		KConfigGroup	cityCfg = cfg.group("locations");
		QString		sKey;
		QStringList	lst;
		int		iCityIndex = 0;
		CityWeather	cityInfo;

		while( true )
		{
			sKey = QString("city%1").arg(iCityIndex+1, 2, 10, QChar('0'));
			if( !cityCfg.hasKey( sKey ) )
				break;
			lst = cityCfg.readEntry(sKey, QStringList());
			if( lst.count() < 5 )
				break;

			cityInfo.clear();
			cityInfo.setProvider(    QUrl::fromPercentEncoding(lst.at(0).toUtf8()) );
			cityInfo.setCity(        QUrl::fromPercentEncoding(lst.at(1).toUtf8()) );
			cityInfo.setCountry(     QUrl::fromPercentEncoding(lst.at(2).toUtf8()) );
			cityInfo.setCountryCode( QUrl::fromPercentEncoding(lst.at(3).toUtf8()) );
			cityInfo.setExtraData(   QUrl::fromPercentEncoding(lst.at(4).toUtf8()) );

			if( lst.count() > 5 )
				cityInfo.setTimeZone( QUrl::fromPercentEncoding(lst.at(5).toUtf8()) );
			if( !cityInfo.timeZone().isValid() )
			{
				QStringList vTimeZones( Utils::GetTimeZones( cityInfo, &m_storage ) );
				if( vTimeZones.count() == 1 )
					cityInfo.setTimeZone( vTimeZones.at(0) );
			}
			if( cityInfo.countryCode().isEmpty() && cityInfo.timeZone().isValid() )
				cityInfo.setCountryCode( cityInfo.timeZone().countryCode() );

			dDebug() << cityInfo.provider() << cityInfo.city() << cityInfo.country() << cityInfo.countryCode()
			         << cityInfo.extraData() << cityInfo.timeZone().name() << cityInfo.timeZone().countryCode();

			m_pWeatherModel->addCity( cityInfo );

			iCityIndex += 1;
		}
		m_configData.iCityIndex = cityCfg.readEntry("selected", 0);	// currently shown city in the applet
	}
	else
		m_configData.iCityIndex = 0;
	dEndFunct();
}

void
YaWP::saveConfig()
{
	dStartFunct();
	KConfigGroup cfg = config();

	/*** SETTINGS PAGE ***/
	cfg.writeEntry( "update interval",			(int)m_configData.iUpdateInterval );
	cfg.writeEntry( "start delay",				(int)m_configData.iStartDelay );
	cfg.writeEntry( "traverse locations",		m_configData.bTraverseLocationsPeriodically );
	cfg.writeEntry( "traverse locations timeout",	m_configData.iTraverseLocationTimeout );

	cfg.writeEntry( "system.distance",          (int)m_configData.distanceSystem );
	cfg.writeEntry( "system.pressure",          (int)m_configData.pressureSystem );
	cfg.writeEntry( "system.temperature",		(int)m_configData.temperatureSystem );
	cfg.writeEntry( "system.speed",			(int)m_configData.speedSystem );

	cfg.writeEntry( "animation.daysnames",		(int)m_configData.daynamesAnimation );
	cfg.writeEntry( "animation.details",		(int)m_configData.detailsAnimation );
	cfg.writeEntry( "animation.page",		(int)m_configData.pageAnimation );
	cfg.writeEntry( "animation.icon",		(int)m_configData.iconAnimation );
	cfg.writeEntry( "animation.duration",		(int)m_configData.iAnimationDuration );

	/*** PANEL PAGE ***/
	cfg.writeEntry( "panel.today.format",		(int)m_configData.todaysWeatherPanelFormat );
	cfg.writeEntry( "panel.forecast.format",	(int)m_configData.forecastWeatherPanelFormat );
	cfg.writeEntry( "panel.forecast.days",		m_configData.iPanelForecastDays );
	cfg.writeEntry( "panel.layout.compact", 	m_configData.bUseCompactPanelLayout );

	cfg.writeEntry( "panel.tooltip.extended.enabled",	m_configData.bUseExtendedTooltip );
	cfg.writeEntry( "panel.tooltip.extended.format",	(int)m_configData.extendedTooltipOptions );
	cfg.writeEntry( "panel.weathericon.interactive",	m_configData.bUseInteractivePanelWeatherIcons );

	/***  THEME PAGE ***/
	cfg.writeEntry( "theme",				m_configData.sBackgroundName );
	cfg.writeEntry( "custom.theme.file",			m_configData.sCustomThemeFile );
	cfg.writeEntry( "custom.theme.enabled",			m_configData.bUseCustomTheme );
	cfg.writeEntry( "custom.theme.background.enabled",	m_configData.bUseCustomThemeBackground );

	cfg.writeEntry( "custom.fontcolor.normal",	m_configData.fontColor );
	cfg.writeEntry( "custom.fontcolor.lowtemp",	m_configData.lowFontColor );
	cfg.writeEntry( "custom.fontcolor.shadows",	m_configData.shadowsFontColor );
	cfg.writeEntry( "custom.fontcolor.usage",	m_configData.bUseCustomFontColor );
	cfg.writeEntry( "text.shadows",	m_configData.bDisableTextShadows );

	/*** LOCATION PAGE ***/
	if( cfg.hasGroup("locations") )
		cfg.group("locations").deleteGroup();	// delete all old entries in this group
	if( m_pWeatherModel->rowCount() > 0 )
	{
		KConfigGroup cityCfg = cfg.group("locations");
		for( int iCityIndex = 0; iCityIndex < m_pWeatherModel->rowCount(); ++iCityIndex )
		{
			QStringList lst;
			const CityWeather * pCity = m_pWeatherModel->getCityInfo( iCityIndex );
			lst << pCity->provider()
			    << pCity->city()
			    << pCity->country()
			    << pCity->countryCode()
			    << pCity->extraData()
				<< pCity->timeZone().name();
			QString sKey = QString("city%1").arg(iCityIndex+1, 2, 10, QChar('0'));
			cityCfg.writeEntry( sKey, lst );
		}
		cityCfg.writeEntry("selected", m_configData.iCityIndex);
	}
	dEndFunct();
}

void
YaWP::loadCustomTheme()
{
	dStartFunct();
	if( !m_configData.bUseCustomTheme )
	{
		dEndFunct();
		return;
	}
	if( !QFile(m_configData.sCustomThemeFile).exists() )
	{
		m_configData.bUseCustomTheme = false;
		dWarning() << "File does not exist: " << m_configData.sCustomThemeFile;
		return;
	}
	/*TODO get the following paramaters from theme:
		rect The rectangle to draw in.
		font The font to use.
		color The font color to use.
	*/
	m_customSvg.setImagePath( m_configData.sCustomThemeFile );
	m_customSvg.setContainsMultipleImages( true );
	dEndFunct();
}

void
YaWP::constraintsEvent(Plasma::Constraints constraints)
{
	dStartFunct();

	if (constraints.testFlag(Plasma::FormFactorConstraint))
	{
		initAppletPainter();

		if (m_pAppletPainter->formFactor() == Plasma::Planar)
		{
			if (m_configData.sBackgroundName == QLatin1String("default") &&
			    !(m_configData.bUseCustomTheme &&
			      m_configData.bUseCustomThemeBackground) )
			{
				setBackgroundHints(DefaultBackground);
			}
			else
			{
				setBackgroundHints(NoBackground);
			}

		}
		else if (m_pAppletPainter->formFactor() == Plasma::Horizontal)
		{
			// no backround in vertical panel mode
			setBackgroundHints(NoBackground);
		}
		else if (m_pAppletPainter->formFactor() == Plasma::Vertical)
		{
			// no backround in vertical panel mode
			setBackgroundHints(NoBackground);
		}

		updateSize();
	}
	if (constraints.testFlag(Plasma::SizeConstraint))
	{
		if (m_pAppletPainter->formFactor() != Plasma::Planar)
			updateSize();
	}

	m_pAppletPainter->update();

	dEndFunct();
}

void
YaWP::updateSize()
{
	QSize sizeHint = m_pAppletPainter->getSize(size());

	if (formFactor() == Plasma::Horizontal)
	{
		setMinimumHeight(0);
		setMaximumHeight(QWIDGETSIZE_MAX);

		setMinimumWidth(sizeHint.width());
		setMaximumWidth(sizeHint.width());
	}
	else if (formFactor() == Plasma::Vertical)
	{
		setMinimumWidth(0);
		setMaximumWidth(QWIDGETSIZE_MAX);

		setMinimumHeight(sizeHint.height());
		setMaximumHeight(sizeHint.height());
	}
	else
	{
		setMinimumWidth(30);
		setMaximumWidth(QWIDGETSIZE_MAX);
		setMinimumHeight(30);
		setMaximumHeight(QWIDGETSIZE_MAX);

		resize(sizeHint);
	}
}

// The paintInterface procedure paints the applet to screen
void
YaWP::paintInterface(QPainter * painter,
                     const QStyleOptionGraphicsItem * option,
                     const QRect & contentsRect)
{
	Q_UNUSED(option);

	if (m_pAppletPainter != 0)
		m_pAppletPainter->paintApplet(painter, option, contentsRect);
}

void
YaWP::createConfigurationInterface(KConfigDialog * parent)
{
	dStartFunct();
	if (m_pConfigDlg != NULL)
		delete m_pConfigDlg;
	m_pConfigDlg = new YawpConfigDialog( parent, &m_storage );
	m_pConfigDlg->copyCities( m_pWeatherModel );
	m_pConfigDlg->setData( &m_configData );

	connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
	connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
	dEndFunct();
}

void
YaWP::configAccepted()
{
	if( ! m_pConfigDlg )
		return;

	dStartFunct();
	m_pWeatherModel->disconnectEngine();
	stopPendingEngineConnection();

	stopTraverseLocationTimeout();

	dDebug() << "configuration changed..."
	         << "    model changed" << m_pConfigDlg->cityModelChanged();
	if (m_pConfigDlg->cityModelChanged())
	{
		dDebug() << "update model";
		m_pWeatherModel->copyCities( *m_pConfigDlg->weatherModel() );
		updateCitySubMenu();
	}
	else
		dDebug() << "model did not change...";

	m_pConfigDlg->getData( &m_configData );
	saveConfig();
	emit configNeedsSaving();

	m_pAppletPainter->setupAnimationTimeLine();
	setupWeatherServiceModel();

	setCityIndex( m_configData.iCityIndex );
	loadCustomTheme();

	if( !m_configData.bUseCustomFontColor )
		setDefaultFontColors();

	const bool bHasCities = (m_pWeatherModel->rowCount() > 0);
	setConfigurationRequired( !bHasCities );
	m_pManualUpdate->setEnabled( bHasCities);
	m_pCitySubMenu->setEnabled( bHasCities );

	//--- trigger the update-process ---
	if (bHasCities)
	{
		bool bUpdateRequired = (m_pConfigDlg->citySettingsChanged() || m_pConfigDlg->unitsChanged());
		m_iIdPendingEngineConnection = startTimer( bUpdateRequired ? 0 : 3 * 60 * 1000 );
		startTraverseLocationTimeout();
	}
	else if (m_pConfigDlg->cityModelChanged())
	{
		// When we have no city anymore, make sure popup will be hidden.
		// This can only happen, when extender is shown and we delete all cities.
		// In this case we do not accept nor pass any mouse events to the applet painter to
		// handle this events. Therefore there is no way for the user to hide the popup manual,
		// unless user is adding at least one city to the applet.
		hidePopup();

		// remove current tooltip
		Plasma::ToolTipManager::self()->clearContent(this);
	}
	
	m_pConfigDlg->resetChanges();

	// to make sure we will change the theme
	constraintsEvent( Plasma::FormFactorConstraint | Plasma::SizeConstraint );
	initAppletPainter();
	m_pAppletPainter->update();

	dEndFunct();
}

void
YaWP::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
	const CityWeather * const pCity = m_stateMachine.currentCity();

	if (!pCity ||
	    m_pAppletPainter->timeLine()->state() == QTimeLine::Running ||
	    event->button() != Qt::LeftButton )
	{
		return;
	}

	// every time user interacts with this plasmoid, we reset the timer
	// to automatically switch to next location
	stopTraverseLocationTimeout();

	event->setAccepted(false);

	if (m_pAppletPainter->formFactor() == Plasma::Planar ||
	    m_configData.bUseInteractivePanelWeatherIcons)
	{
		m_pAppletPainter->mousePressEvent(event);
	}

	if (!event->isAccepted() && m_pAppletPainter->formFactor() != Plasma::Planar)
		togglePopup();

	//   When user did no click on an animation area, than we have to restart timeout
	//   to traverse through all locations periodically
	startTraverseLocationTimeout();
}

void
YaWP::timerEvent(QTimerEvent * event)
{
	if (event->timerId() == m_iIdPendingEngineConnection)
	{
		stopPendingEngineConnection();
		m_pWeatherModel->reconnectEngine();
	}
	else if (event->timerId() == m_iIdTraverseLocations)
	{
		stopTraverseLocationTimeout();

		int iSelectedCityIndex = (m_configData.iCityIndex + 1) % m_pWeatherModel->rowCount();
		m_pAppletPainter->initCityChangeAnimation(iSelectedCityIndex);
	}
	Plasma::Applet::timerEvent(event);
}

void
YaWP::slotCityUpdate(WeatherServiceModel::ServiceUpdate updateType)
{
	dStartFunct();
	stopPendingEngineConnection();
	const CityWeather * const pCity = m_stateMachine.currentCity();

	if(!pCity)
	{
		dEndFunct();
		return;
	}

	if( updateType.testFlag(WeatherServiceModel::CityInfoUpdate) )
	{
		updateCitySubMenu();
		saveConfig();
		emit configNeedsSaving();
	}

	//--- update the paneltooltip when we are in panel mode only ---
	if (m_pAppletPainter->formFactor() != Plasma::Planar)
		createPanelTooltip();
	
	m_pOpenForecastUrl->setEnabled( !pCity->creditUrl().isNull() );

	m_pAppletPainter->update();
	dEndFunct();
}

/***************************************************************************\
*
\***************************************************************************/

void
YaWP::changeCity( QAction * action )
{
	int iSelectedCityIndex = action->data().toInt();
	if( iSelectedCityIndex >= 0 &&
	    iSelectedCityIndex < m_pWeatherModel->rowCount() &&
	    iSelectedCityIndex != m_configData.iCityIndex )
	{
		m_pAppletPainter->initCityChangeAnimation(iSelectedCityIndex);
	}
}

void
YaWP::about() const
{
	KAboutApplicationDialog * aboutDialog = new KAboutApplicationDialog ( m_aboutData );
	connect ( aboutDialog, SIGNAL (finished()), aboutDialog, SLOT(deleteLater()) );
	aboutDialog->show ();
}

void
YaWP::openForecastUrl()
{
	const CityWeather * city = m_stateMachine.currentCity();
	if (!city)
		return;
	
	if (!city->creditUrl().isNull())
		Utils::OpenUrl(city->creditUrl());
}

void
YaWP::animationFinished()
{
	startTraverseLocationTimeout();
}

/***********************************************************************************
*/

void
YaWP::setCityIndex( int cityIndex )
{
	dStartFunct();
	m_stateMachine.setCurrentCityIndex( cityIndex );
	m_configData.iCityIndex = m_stateMachine.currentCityIndex();
	const CityWeather * city = m_stateMachine.currentCity();

	if (city)
	{
		//--- select the right menuitem in the city-submenu ---
		QList<QAction *> list = m_pCitySubMenu->menu()->actions();
		int iCityIndex = m_stateMachine.currentCityIndex();
		if (list.count() > iCityIndex)
			list.at(iCityIndex )->setChecked( true );

		//--- update the paneltooltip when we are in panel mode only ---
		if (m_pAppletPainter->formFactor() != Plasma::Planar)
			createPanelTooltip();
		
		m_pOpenForecastUrl->setEnabled( !city->creditUrl().isNull() );
	}
	m_pAppletPainter->update();
	dEndFunct();
}

void
YaWP::slotToggleWeatherIcon(int dayIndex)
{
	m_stateMachine.toggleIconState(dayIndex);
}

void
YaWP::slotThemeChanged()
{
	setDefaultFontColors();

	if (m_pAppletPainter->formFactor() != Plasma::Planar)
		createPanelTooltip();
}

void
YaWP::slotStartCacheCleanUp()
{
	dTracing() << "Start cache clean up...";
	QDir cacheDir(
		WeatherDataProcessor::CacheDirectory,
		"*.dat",
		QDir::NoSort,
		QDir::Files | QDir::NoSymLinks | QDir::Writable);
	QFileInfoList cacheFiles = cacheDir.entryInfoList();

	const QDateTime currentDateTime = QDateTime::currentDateTime();
	KUrl::List selectedCacheFiles;

	foreach(QFileInfo info, cacheFiles)
	{
		// When file is older than five days, we add it to the list of files that will be removed.
		if (info.lastModified().daysTo(currentDateTime) > 5)
		{
			dTracing() << "Remove cache file" << info.absoluteFilePath();
			selectedCacheFiles.append(KUrl(info.absoluteFilePath()));
		}
		else
		{
			dTracing() << "Keeping file" << info.absoluteFilePath();
		}
	}

	// Start job to remove selected files. We hide the progress information,
	// because we do not want to bother the user with this stuff.
	if (selectedCacheFiles.count() > 0)
		KIO::del(selectedCacheFiles, KIO::HideProgressInfo);
}

void
YaWP::setDefaultFontColors()
{
	if( m_configData.bUseCustomFontColor )
		return;

	if( // m_pPanelLayout ||
	    m_configData.sBackgroundName.compare( QLatin1String("default") ) == 0 ||
	    m_configData.sBackgroundName.compare( QLatin1String("naked") ) == 0 )
	{
		m_configData.fontColor = KColorScheme(QPalette::Active, KColorScheme::View,
			Plasma::Theme::defaultTheme()->colorScheme()).foreground().color();
		m_configData.lowFontColor = KColorScheme(QPalette::Active, KColorScheme::View,
			Plasma::Theme::defaultTheme()->colorScheme()).foreground(KColorScheme::InactiveText).color();

		if( m_configData.fontColor.red() < 25 &&
		    m_configData.fontColor.green() < 25 &&
		    m_configData.fontColor.blue() < 25 )
		{
			m_configData.lowFontColor = m_configData.fontColor.lighter();
			m_configData.shadowsFontColor = QColor(255,255,255,100);
		}
		else
		{
			m_configData.lowFontColor = m_configData.fontColor.darker(125);
			m_configData.shadowsFontColor = QColor(0,0,0,100);
		}
	}
	else
	{
		m_configData.fontColor = QColor(Qt::white);
		m_configData.lowFontColor = QColor(Qt::gray);
		m_configData.shadowsFontColor = QColor(0,0,0,100);
	}
}

void
YaWP::updateCitySubMenu()
{
	//--- first of all we delete all old actions ---
	m_pCitySubMenu->menu()->clear();

	const int iMaxCities = m_pWeatherModel->rowCount();
	for( int i = 0; i < iMaxCities; ++i )
	{
		const CityWeather * cityInfo = m_pWeatherModel->getCityInfo( i );
		QIcon flag = m_storage.countryMap()->getPixmapForCountryCode( cityInfo->countryCode() );
		QAction * action = new QAction( flag, cityInfo->localizedCityString(), this );
		action->setCheckable( true );
		action->setData( QVariant(i) );
		m_pGrpActionCities->addAction( action );
		m_pCitySubMenu->addAction( action );

		if( i == m_configData.iCityIndex )
			action->setChecked( true );
	}
	m_pCitySubMenu->setEnabled( iMaxCities > 0 );
}

void
YaWP::initAppletPainter()
{
	const Plasma::FormFactor form = formFactor();

	if (m_pAppletPainter == 0 || m_pAppletPainter->formFactor() != form)
	{
		if (m_pAppletPainter != 0)
		{
			m_pAppletPainter->disconnect();
			this->disconnect(m_pAppletPainter);
			this->disconnect(m_pAppletPainter->timeLine());

			delete m_pAppletPainter;
		}

		if (form == Plasma::Horizontal || form == Plasma::Vertical)
		{
			m_pAppletPainter = new PanelPainter(this, &m_configData, &m_stateMachine, form);
			createExtenderItem();
			createPanelTooltip();
		}
		else
		{
			m_pAppletPainter = new DesktopPainter(this, &m_configData, &m_stateMachine);
			destroyExtenderItem();
			Plasma::ToolTipManager::self()->clearContent(this);
		}
		connect(m_pWeatherModel, SIGNAL(isBusy(bool)), m_pAppletPainter, SLOT(setBusy(bool)));
		connect(m_pAppletPainter, SIGNAL(signalCityChanged(int)), this, SLOT(setCityIndex(int)), Qt::DirectConnection);
		connect(m_pAppletPainter, SIGNAL(signalToggleWeatherIcon(int)), this, SLOT(slotToggleWeatherIcon(int)), Qt::DirectConnection);
		connect(m_pAppletPainter->timeLine(), SIGNAL(finished()),  this, SLOT(animationFinished()));
	}

	if (m_svg.isValid())
		m_pAppletPainter->setSvg(&m_svg);
	if (m_customSvg.isValid())
		m_pAppletPainter->setCustomSvg(&m_customSvg);

	m_pAppletPainter->setupAnimationTimeLine();
}


void
YaWP::createExtenderItem()
{
	bool itemFound = extender()->hasItem(PANEL_DESKTOP_INTERFACE_NAME);
	Plasma::ExtenderItem * item = 0;

	if (itemFound)
		item = extender()->item(PANEL_DESKTOP_INTERFACE_NAME);

	// When we already have an extender item, just connect the current painter with this extender item
	if (item != 0 && item->widget() != 0)
	{
		PanelDesktopInterface * popupDesktopInterface = dynamic_cast<PanelDesktopInterface *>(item->widget());
		PanelPainter * panelPainter = dynamic_cast<PanelPainter *>(m_pAppletPainter);

		if (panelPainter != 0 && popupDesktopInterface != 0)
		{
			panelPainter->setPopupPainter(popupDesktopInterface->desktopPainter());
			dInfo() << "Reusing existing desktop interface.";
		}
		else
			// should not happen !!! does it ???
			dError() << "Could not connect to existing desktop interface!";
	}
	else
	{
		dInfo() << "Creating new desktop interface.";
		Plasma::ExtenderItem * item = new Plasma::ExtenderItem(extender());
		item->setName(PANEL_DESKTOP_INTERFACE_NAME);
		initExtenderItem(item);
	}
}

void
YaWP::initExtenderItem(Plasma::ExtenderItem * item)
{
	PanelPainter * panelPainter = dynamic_cast<PanelPainter *>(m_pAppletPainter);

	/* We accept only one extender item with this name, because it does not make
	 * sence to have multiple items connected.
	 */
	if (item->name() == PANEL_DESKTOP_INTERFACE_NAME &&
	    panelPainter != 0 &&
	    panelPainter->getPopupPainter() == 0)
	{
		PanelDesktopInterface * popupDesktopInterface = new PanelDesktopInterface(&m_configData, &m_stateMachine, item);
		popupDesktopInterface->setSvg(&m_svg);
		popupDesktopInterface->setCustomSvg(&m_customSvg);
		popupDesktopInterface->setupAnimationTimeLine();

		popupDesktopInterface->setMinimumSize(QSizeF(100, 93));
		popupDesktopInterface->setPreferredSize(273, 255);

		panelPainter->setPopupPainter(popupDesktopInterface->desktopPainter());

		connect( m_pWeatherModel, SIGNAL(isBusy(bool)), popupDesktopInterface, SLOT(setBusy(bool)) );

		item->setTitle(i18n("yaWP Desktop Interface"));
		item->setIcon("weather-clear");
		item->setWidget(popupDesktopInterface);
	}
	else
		Plasma::PopupApplet::initExtenderItem(item);
}

void
YaWP::destroyExtenderItem()
{
	bool itemFound = extender()->hasItem(PANEL_DESKTOP_INTERFACE_NAME);

	if (itemFound)
	{
		extender()->item(PANEL_DESKTOP_INTERFACE_NAME)->deleteLater();
	}
}

void
YaWP::createPanelTooltip()
{
	dStartFunct();
	Plasma::ToolTipManager::self()->clearContent(this);

	//--- just exit, when we have no city or no weather information ---
	const CityWeather * city = m_stateMachine.currentCity();
	if( !city || city->days().count() == 0 )
	{
		dEndFunct();
		return;
	}

	Plasma::ToolTipContent toolTipData;
	toolTipData.setMainText( city->localizedCityString() );

	//--- Create the tooltip pixmap for simple layout and extended tooltip ---
	if (!m_configData.bUseExtendedTooltip ||
	    m_configData.extendedTooltipOptions.testFlag(Yawp::PreviewPage))
	{
		DesktopPainter toolTipPainter(0, &m_configData, &m_stateMachine);
		toolTipPainter.setSvg(&m_svg);
		toolTipPainter.setCustomSvg(&m_customSvg);

		QPixmap pix;
		if (!m_configData.bUseExtendedTooltip || city->days().count() == 1)
			pix = toolTipPainter.createSimpleToolTip(QSize(218,0));
		else
			pix = toolTipPainter.createExtendedToolTip(QSize(218,0));

		if (!pix.isNull())
			toolTipData.setImage(pix);
	}

	//--- Create the content of the extended tooltip ---
	if (m_configData.bUseExtendedTooltip)
	{
		if (!m_configData.extendedTooltipOptions.testFlag(Yawp::PreviewPage))
		{
 			KIcon appIcon(icon());
			toolTipData.setImage( appIcon.pixmap(48,48) );
		}

		bool showSatellitePage
		      = m_configData.extendedTooltipOptions.testFlag(Yawp::SatellitePage) &&
		        m_stateMachine.hasPage(Yawp::SatellitePage);
		QString toolTipText;

		if (showSatellitePage)
			toolTipText += "<table><tr><td>";

		//--- Create the forecast information for today and the following two days ---
		const int maxDay = qMin(3, city->days().count());
		for (int dayIndex = 0; dayIndex < maxDay; ++dayIndex)
		{
			const YawpDay * day = city->days().at(dayIndex);

			// QDate returns translated nominative case of day
			// name which is grammatically wrong at least in
			// Russian. So fetch base English form and translate
			// into correct one with own catalog
			const KDateTime kDate(day->date());

			toolTipText += "<u>"
			            + i18n("Forecast for %1 (%2)",
			                   i18nc("Forecast for day",
			                       kDate.toString("%:A").toUtf8()),
			                       KGlobal::locale()->formatDate(day->date(), KLocale::ShortDate ))
			            + "</u><br />";
			toolTipText += i18n("Day: ")
			            + Utils::LocalizedWeatherString(day->weather().description())
			            + "<br />";

			if (day->hasNightValues())
			{
				toolTipText += i18n("Night: ")
				            + Utils::LocalizedWeatherString(day->nightWeather().description())
				            + "<br />";
			}

			//--- Sunset, sunrise and realfeal will be shown for today only, to limit tooltip size ---
			if (dayIndex == 0)
			{
				// Show the sunrise/sunset when we have it
				//    (Google does not have this information)
				// Format time according to KDE Local setting
				if (!day->sunrise().isNull() && !day->sunset().isNull())
				{
					toolTipText += i18n("Sunrise at %1, sunset at %2",
						KGlobal::locale()->formatTime(day->sunrise()),
						KGlobal::locale()->formatTime(day->sunset())) + "<br />";
				}

				//--- Show the realfeel temperature (when we have it) ---
				const YawpWeather & weather = day->weather();
				if (weather.temperatureRealFeelHigh() < SHRT_MAX &&
				    weather.temperatureRealFeelLow() < SHRT_MAX)
				{
					toolTipText += i18n("Real feel: %1 / %2",
						QString::number(weather.temperatureRealFeelHigh()) + QChar(0xB0),
						QString::number(weather.temperatureRealFeelLow()) + QChar(0xB0))
						+ "<br />";
				}
			}

			if (dayIndex+1 < maxDay)
				toolTipText += "<br />";
			else
				// removes the last <br />; because this is the end of text
				toolTipText.remove(toolTipText.length()-6, 6);
		}

		if (showSatellitePage)
		{
			toolTipData.addResource(
				Plasma::ToolTipContent::ImageResource,
				QUrl("wicon://satelite"),
				QVariant(city->satelliteImage().scaledToWidth(218, Qt::SmoothTransformation)));

			toolTipText += "</td><td valign=\"top\">";
			toolTipText += "<img src=\"wicon://satelite\"/>";
			toolTipText += "</td></tr></table>";
		}

		toolTipData.setSubText(toolTipText);
	}

	toolTipData.setAutohide(false);
	Plasma::ToolTipManager::self()->setContent(this, toolTipData);

	dEndFunct();
}

void
YaWP::setupWeatherServiceModel()
{
	m_pWeatherModel->setUpdateInterval( m_configData.iUpdateInterval );
	m_pWeatherModel->setDetailsPropertyList( m_configData.vDetailsPropertyRankingList );

	WeatherDataProcessor * pDataProcessor = m_pWeatherModel->dataProcessor();
	if (pDataProcessor)
	{
		pDataProcessor->setDistanceSystem( m_configData.distanceSystem );
		pDataProcessor->setPressureSystem( m_configData.pressureSystem );
		pDataProcessor->setTemperatureSystem( m_configData.temperatureSystem );
		pDataProcessor->setSpeedSystem( m_configData.speedSystem );
	}
}

void
YaWP::stopPendingEngineConnection()
{
	if (m_iIdPendingEngineConnection > 0)
	{
		killTimer(m_iIdPendingEngineConnection);
		m_iIdPendingEngineConnection = -1;
	}
}

void
YaWP::startTraverseLocationTimeout()
{
	if (m_configData.bTraverseLocationsPeriodically && m_pWeatherModel->rowCount() > 1)
		m_iIdTraverseLocations = startTimer(m_configData.iTraverseLocationTimeout * 1000);
}

void
YaWP::stopTraverseLocationTimeout()
{
	if (m_iIdTraverseLocations > 0)
		killTimer(m_iIdTraverseLocations);
}

// This is the command that links your applet to the .desktop file
K_EXPORT_PLASMA_APPLET(yawp, YaWP);
