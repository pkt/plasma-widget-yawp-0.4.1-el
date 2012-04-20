/*************************************************************************\
*   Copyright (C) 2009 by Ulf Kreissig                                    *
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
#include "../config.h"
#include "ionlistmodel.h"
#include "dlgaddcity.h"
#include "dlgtimezone.h"
#include "ionlistmodel.h"
#include "pageanimator.h"
#include "weatherservice.h"
#include "yawpconfig.h"
#include "yawpdefines.h"
#include "utils.h"

#include "logger/streamlogger.h"


//--- QT4 ---
#include <QDir>
#include <QFileInfo>
#include <QIcon>

//--- KDE4 ---
#include <KConfigDialog>
#include <KColorScheme>
#include <KFileDialog>
#include <KIcon>
#include <KMessageBox>
#include <Plasma/Theme>

//--- number of maximum allowed cities ---
#define MAXCITIES	50

struct YawpConfigDialog::Private
{
	KConfigDialog              * pConfigDlg;
	WeatherServiceModel        * pWeatherModel;
	bool                         bCityModelChanged;
	bool                         bCitySettingsChanged;
	bool                         bUnitsChanged;

// This is used to map localized theme names to the english names
	QMap <int, QString>          theme_map;
	
	Yawp::Storage              * pStorage;
};

YawpConfigDialog::YawpConfigDialog( KConfigDialog * parent, Yawp::Storage * pStorage )
       : QObject(parent),
         d(new Private)
{
	d->pStorage = pStorage;
	
	d->bCityModelChanged = false;
	d->bCitySettingsChanged = false;

	d->pConfigDlg = parent;
	d->pWeatherModel = new WeatherServiceModel(pStorage, this);
	d->pWeatherModel->setObjectName("ConfigModel");

// Fill theme map
	d->theme_map.insert(0, "default");
	d->theme_map.insert(1, "purple");
	d->theme_map.insert(2, "green");
	d->theme_map.insert(3, "black");
	d->theme_map.insert(4, "blue");
	d->theme_map.insert(5, "red");
	d->theme_map.insert(6, "yellow");
	d->theme_map.insert(7, "funky");
	d->theme_map.insert(8, "naked");

	QWidget * locationsPage	= new QWidget( parent );
	QWidget * settingsPage	= new QWidget( parent );
	QWidget * panelPage	= new QWidget( parent );
	QWidget * themePage	= new QWidget( parent );

	Ui::LocationsPage::setupUi(locationsPage);
	Ui::SettingsPage::setupUi(settingsPage);
	Ui::PanelPage::setupUi(panelPage);
	Ui::ThemePage::setupUi(themePage);

	parent->setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);
	parent->addPage(locationsPage, i18n("Locations"), QLatin1String("preferences-desktop-locale"));
	parent->addPage(settingsPage,  i18n("Settings"),  QLatin1String("preferences-system"));
	parent->addPage(panelPage,     i18n("Panel"),     QLatin1String("preferences-system-windows"));
	parent->addPage(themePage,     i18n("Theme"),     QLatin1String("plasma"));

	connect(parent,		SIGNAL(destroyed()),	this, SLOT(deleteLater()));

	//--- LOCATION PAGE ---
	//
	cityTableView->setModel( d->pWeatherModel );
	cityTableView->setWordWrap( false );
	cityTableView->setAlternatingRowColors(true);
	cityTableView->setColumnWidth( 0, 250 );
	cityTableView->setColumnWidth( 1, 100 );
	cityTableView->setColumnWidth( 2, 150 );

	connect(btnAddCity,     SIGNAL(clicked()),     this, SLOT(addCity()));
	connect(btnDeleteCity,  SIGNAL(clicked()),     this, SLOT(deleteCity()));
	connect(cityTableView,  SIGNAL(clicked(const QModelIndex &)),   this, SLOT(locationSelected(const QModelIndex &)));
	connect(btnMoveUp,      SIGNAL(clicked()),     this, SLOT(moveSelectedCityUp()));
	connect(btnMoveDown,    SIGNAL(clicked()),     this, SLOT(moveSelectedCityDown()));
	connect(btnSetTimeZone, SIGNAL(clicked()),     this, SLOT(slotSetTimeZone()));

	btnAddCity->setIcon( KIcon("list-add") );
	btnDeleteCity->setIcon( KIcon("list-remove") );
	btnMoveUp->setIcon( KIcon("go-up") );
	btnMoveDown->setIcon( KIcon("go-down") );
	btnSetTimeZone->setIcon( KIcon("clock") );

	//--- THEME PAGE ---
	//
	connect(checkBoxCustomTheme,		SIGNAL(stateChanged(int)),	this, SLOT(changeThemeState(int)));
	connect(checkBoxCustomTheme,		SIGNAL(clicked()),		this, SLOT(enableYawpBackground()));
	connect(checkBoxUseCustomBackground,	SIGNAL(clicked()),		this, SLOT(enableYawpBackground()));
	connect(btnCustomThemeFile,		SIGNAL(released()),		this, SLOT(selectCustomThemeFile()));
	connect(checkBoxUseCustomColor,		SIGNAL(stateChanged(int)),	this,  SLOT(enableCustomFontColorOptions(int)));

	//--- SETUP UNIT SYSTEM ---
#if KDE_IS_VERSION(4,3,70)
	comboTemperature->insertItem( 0, i18n("Celsius"),	QVariant(KUnitConversion::Celsius) );
	comboTemperature->insertItem( 1, i18n("Fahrenheit"),	QVariant(KUnitConversion::Fahrenheit) );

	comboSpeed->insertItem( 0, i18n("Kilometers Per Hour"),	QVariant(KUnitConversion::KilometerPerHour) );
	comboSpeed->insertItem( 1, i18n("Meters Per Second"),	QVariant(KUnitConversion::MeterPerSecond) );
	comboSpeed->insertItem( 2, i18n("Miles Per Hour"),	QVariant(KUnitConversion::MilePerHour) );
	comboSpeed->insertItem( 3, i18n("Knots"),		QVariant(KUnitConversion::Knot) );
	comboSpeed->insertItem( 4, i18n("Beaufort Scale"),	QVariant(KUnitConversion::Beaufort) );

	comboPressure->insertItem( 0, i18n("Kilopascals"),	QVariant(KUnitConversion::Kilopascal) );
	comboPressure->insertItem( 1, i18n("Inches HG"),	QVariant(KUnitConversion::InchesOfMercury) );
	comboPressure->insertItem( 2, i18n("Millibars"),	QVariant(KUnitConversion::Millibar) );
	comboPressure->insertItem( 3, i18n("Hectopascals"),	QVariant(KUnitConversion::Hectopascal) );
#if KDE_IS_VERSION(4,4,92)
	comboPressure->insertItem( 4, i18n("Millimeters HG"),	QVariant(KUnitConversion::MillimetersOfMercury) );
#else
	comboPressure->insertItem( 4, i18n("Torr (Millimeter HG)"),	QVariant(KUnitConversion::Torr) );
#endif

	comboDistance->insertItem( 3, i18n("Kilometers"),	QVariant(KUnitConversion::Kilometer) );
	comboDistance->insertItem( 4, i18n("Miles"),		QVariant(KUnitConversion::Mile) );
#elif KDE_VERSION_MINOR <= 3
	comboTemperature->insertItem( 0, i18n("Celsius"),	QVariant(WeatherUtils::Celsius) );
	comboTemperature->insertItem( 1, i18n("Fahrenheit"),	QVariant(WeatherUtils::Fahrenheit) );

	#if KDE_VERSION_MINOR == 3
		comboSpeed->insertItem( 0, i18n("Kilometers Per Hour"),	QVariant(WeatherUtils::KilometersPerHour) );
		comboSpeed->insertItem( 1, i18n("Meters Per Second"),	QVariant(WeatherUtils::MetersPerSecond) );
		comboSpeed->insertItem( 2, i18n("Miles Per Hour"),	QVariant(WeatherUtils::MilesPerHour) );
	#else
		comboSpeed->insertItem( 0, i18n("Kilometers Per Hour"),	QVariant(WeatherUtils::KilometersAnHour) );
		comboSpeed->insertItem( 1, i18n("Meters Per Second"),	QVariant(WeatherUtils::MetersPerSecond) );
		comboSpeed->insertItem( 2, i18n("Miles Per Hour"),	QVariant(WeatherUtils::MilesAnHour) );
	#endif
	comboSpeed->insertItem( 3, i18n("Knots"),		QVariant(WeatherUtils::Knots) );
	comboSpeed->insertItem( 4, i18n("Beaufort Scale"),	QVariant(WeatherUtils::Beaufort) );

	comboPressure->insertItem( 0, i18n("Kilopascals"),	QVariant(WeatherUtils::Kilopascals) );
	comboPressure->insertItem( 1, i18n("Inches HG"),	QVariant(WeatherUtils::InchesHG) );
	comboPressure->insertItem( 2, i18n("Millibars"),	QVariant(WeatherUtils::Millibars) );
	comboPressure->insertItem( 3, i18n("Hectopascals"),	QVariant(WeatherUtils::Hectopascals) );

	comboDistance->insertItem( 3, i18n("Kilometers"),	QVariant(WeatherUtils::Kilometers) );
	comboDistance->insertItem( 4, i18n("Miles"),		QVariant(WeatherUtils::Miles) );
#endif
	//--- INIT COMBOBOX UPDATE INTERVAL ---
	for(int i = 0; i < 4; ++i)
	{
		int iValue = (4-i)*15;
		comboInterval->insertItem(i, i18n("%1 minutes",iValue), QVariant(iValue));
	}

	initAnimationCombo( comboPageAnimation );
	initAnimationCombo( comboDayformatAnimation );
	initAnimationCombo( comboDetailAnimation );
	initAnimationCombo( comboIconAnimation );

	connect(sliderAnimationDuration, SIGNAL(valueChanged(int)), this, SLOT(sliderAnimationDurationValue(int)));

	//--- PANEL PAGE ---
	//
	connect(checkCompactPanelLayout, SIGNAL(toggled(bool)), this, SLOT(enableCompactPanelLayout(bool)));
	connect(radioExtendedTooltip, SIGNAL(toggled(bool)), this, SLOT(enableExtendedTooltipOptions(bool)));
	
	QButtonGroup * grpPanelWeather = new QButtonGroup(this);
	grpPanelWeather->addButton(radioActualTemp);
	grpPanelWeather->addButton(radioActualIcon);
	grpPanelWeather->addButton(radioActualBoth);
	
	QButtonGroup * grpPanelForecasts = new QButtonGroup(this);
	grpPanelForecasts->addButton(radioForecastTemp);
	grpPanelForecasts->addButton(radioForecastIcon);
	grpPanelForecasts->addButton(radioForecastBoth);
	
	//--- RESIZE AND SET MINIMUM SIZE ---
	QSize minSize( parent->sizeHint() );
	minSize.setWidth( 650 );
	parent->resize( minSize );
	parent->setMinimumSize( minSize );

	if( d->pStorage->ionListModel()->rowCount() == 0 )
	{
		KMessageBox::sorry( parent, i18n("Your system does not have any weather-engine. %1 is useless without at least one weatherengine.", QLatin1String(YAWP_NAME)) );
		btnAddCity->setEnabled( false );
	}
	
	//--- TRIGGER SLOT MODIFIED WHEN CONFIG-DIALOG HAS BEEN CHANGED ---
	//
	//--- Settings Page - tab unit systems ---
	connect(comboDistance, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(comboPressure, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(comboTemperature, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(comboSpeed, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	
	//--- Settings Page - tab animation ---
	connect(comboPageAnimation, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(comboDayformatAnimation, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(comboDetailAnimation, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(comboIconAnimation, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(sliderAnimationDuration, SIGNAL(valueChanged(int)), this, SLOT(settingsChanged()));
	
	//--- Settings Page - tab update behaviour ---
	connect(comboInterval,  SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(spinboxStartDelay, SIGNAL(valueChanged(int)), this, SLOT(settingsChanged()));
	connect(groupBoxTraverseLocationsPeriodically, SIGNAL(clicked()), this, SLOT(settingsChanged()));
	connect(spinTraverseLocationsTimeout, SIGNAL(valueChanged(int)), this, SLOT(settingsChanged()));
	
	//--- Panel Page ---
	connect(grpPanelWeather, SIGNAL(buttonClicked(int)), this, SLOT(settingsChanged()));
	connect(grpPanelForecasts, SIGNAL(buttonClicked(int)), this, SLOT(settingsChanged()));
	connect(comboForecastDays, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(checkCompactPanelLayout, SIGNAL(stateChanged(int)), this, SLOT(settingsChanged()));
	connect(checkInteractivePanelWeatherIcons, SIGNAL(stateChanged(int)), this, SLOT(settingsChanged()));
	connect(radioSimpleTooltip, SIGNAL(clicked()), this, SLOT(settingsChanged()));
	connect(checkTooltipPreview, SIGNAL(stateChanged(int)), this, SLOT(settingsChanged()));
	connect(checkTooltipSateliteMap, SIGNAL(stateChanged(int)), this, SLOT(settingsChanged()));
	connect(checkTooltipThemeBackground, SIGNAL(stateChanged(int)), this, SLOT(settingsChanged()));
	
	//--- Theme Page ---
	connect(comboTheme, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(checkBoxCustomTheme, SIGNAL(stateChanged(int)), this, SLOT(settingsChanged()));
	connect(checkBoxUseCustomBackground, SIGNAL(stateChanged(int)), this, SLOT(settingsChanged()));
	connect(checkBoxUseCustomColor, SIGNAL(stateChanged(int)), this, SLOT(settingsChanged()));
	connect(comboColor, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(comboLowerColor, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(comboShadowsColor, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsChanged()));
	connect(checkBoxDisableTextShadows, SIGNAL(stateChanged(int)), this, SLOT(settingsChanged()));
}

YawpConfigDialog::~YawpConfigDialog()
{
	delete d->pWeatherModel;
	delete d;
}

void
YawpConfigDialog::copyCities( const WeatherServiceModel * model )
{
	if( model )
		d->pWeatherModel->copyCities( *model );
	updateLocationButtons();

	if( d->pWeatherModel->rowCount() > 0 )
	{
		cityTableView->resizeColumnToContents(1);
		cityTableView->resizeColumnToContents(2);
	}
}

const WeatherServiceModel *
YawpConfigDialog::weatherModel() const
{
	return d->pWeatherModel;
}

bool
YawpConfigDialog::cityModelChanged() const
{
	return d->bCityModelChanged;
}

bool
YawpConfigDialog::citySettingsChanged() const
{
	return d->bCitySettingsChanged;
}

bool
YawpConfigDialog::unitsChanged() const
{
	return d->bUnitsChanged;
}

void
YawpConfigDialog::resetChanges()
{
	d->bCityModelChanged = false;
	d->bCitySettingsChanged = false;
	d->bUnitsChanged = false;
}

void
YawpConfigDialog::setData( const Yawp::ConfigData * data )
{
	if( ! d->pConfigDlg || !data )
		return;
	int	iIdx;
	
	//--- SELECT THIS CITY THE USER HAD SELECTED IN YAWP ---
	cityTableView->setCurrentIndex( cityTableView->model()->index(data->iCityIndex, 0, QModelIndex()) );
	locationSelected( cityTableView->currentIndex() );

	//--- UPDATE INTERVAL ---
	iIdx = comboInterval->findData( data->iUpdateInterval );
	comboInterval->setCurrentIndex( iIdx >= 0 ? iIdx : 0 );
	
	spinboxStartDelay->setValue( data->iStartDelay );
	
	groupBoxTraverseLocationsPeriodically->setChecked( data->bTraverseLocationsPeriodically );
	spinTraverseLocationsTimeout->setValue( data->iTraverseLocationTimeout );

	//--- UNIT SYSTEM ---
	iIdx = comboTemperature->findData( data->temperatureSystem );
	comboTemperature->setCurrentIndex( iIdx >= 0 ? iIdx : 0 );
	iIdx = comboSpeed->findData( data->speedSystem );
	comboSpeed->setCurrentIndex( iIdx >= 0 ? iIdx : 0 );
	iIdx = comboDistance->findData( data->distanceSystem );
	comboDistance->setCurrentIndex( iIdx >= 0 ? iIdx : 0 );
	iIdx = comboPressure->findData( data->pressureSystem );
	comboPressure->setCurrentIndex( iIdx >= 0 ? iIdx : 0 );

	//--- ANIMATION ---
	iIdx = comboPageAnimation->findData( data->pageAnimation );
	comboPageAnimation->setCurrentIndex( iIdx >= 0 ? iIdx : 0 );
	iIdx = comboDayformatAnimation->findData( data->daynamesAnimation );
	comboDayformatAnimation->setCurrentIndex( iIdx >= 0 ? iIdx : 0 );
	iIdx = comboDetailAnimation->findData( data->detailsAnimation );
	comboDetailAnimation->setCurrentIndex( iIdx >= 0 ? iIdx : 0 );
	iIdx = comboIconAnimation->findData( data->iconAnimation );
	comboIconAnimation->setCurrentIndex( iIdx >= 0 ? iIdx : 0 );

	sliderAnimationDuration->setValue( data->iAnimationDuration );
	sliderAnimationDurationValue( data->iAnimationDuration );

	//--- THEME PAGE ---
	//
	checkBoxCustomTheme->setCheckState( data->bUseCustomTheme ? Qt::Checked : Qt::Unchecked );
	checkInteractivePanelWeatherIcons->setCheckState( data->bUseInteractivePanelWeatherIcons ? Qt::Checked : Qt::Unchecked );
	
	changeThemeState( checkBoxCustomTheme->checkState() );

	comboTheme->setCurrentIndex( d->theme_map.key(data->sBackgroundName) );
	checkBoxUseCustomBackground->setCheckState( data->bUseCustomThemeBackground ? Qt::Checked : Qt::Unchecked );
	editCustomThemeFile->setText(data->sCustomThemeFile);

	//--- USE CUSTOM COLOR ---
	checkBoxUseCustomColor->setCheckState( data->bUseCustomFontColor ? Qt::Checked : Qt::Unchecked );
	enableCustomFontColorOptions( checkBoxUseCustomColor->checkState() );
	comboColor->setColor( data->fontColor );
	comboLowerColor->setColor( data->lowFontColor );
	comboShadowsColor->setColor( data->shadowsFontColor );

	//--- TEXT SHADOWS ---
	checkBoxDisableTextShadows->setCheckState( data->bDisableTextShadows ? Qt::Checked : Qt::Unchecked );

	//--- PANEL PAGE ---
	//
	//--- current day settings ---
	if( data->todaysWeatherPanelFormat == Yawp::PanelTemperature )
		radioActualTemp->setChecked( true );
	else if( data->todaysWeatherPanelFormat ==  Yawp::PanelIcon )
		radioActualIcon->setChecked( true );
	else
		radioActualBoth->setChecked( true );

	//--- forecast days settings ---
	if( data->forecastWeatherPanelFormat == Yawp::PanelTemperature )
		radioForecastTemp->setChecked( true );
	else if( data->forecastWeatherPanelFormat == Yawp::PanelIcon )
		radioForecastIcon->setChecked( true );
	else
		radioForecastBoth->setChecked( true );
	comboForecastDays->setCurrentIndex( data->iPanelForecastDays );

	checkCompactPanelLayout->setChecked( data->bUseCompactPanelLayout );
	enableCompactPanelLayout( data->bUseCompactPanelLayout );

	if( data->bUseExtendedTooltip )
		radioExtendedTooltip->setChecked( true );
	else
		radioSimpleTooltip->setChecked( true );
	if( data->extendedTooltipOptions.testFlag( Yawp::SatellitePage ) )
		checkTooltipSateliteMap->setChecked( true );
	if( data->extendedTooltipOptions.testFlag( Yawp::PreviewPage ) )
		checkTooltipPreview->setChecked( true );
	if( data->extendedTooltipOptions.testFlag( Yawp::ThemeBackground ) )
		checkTooltipThemeBackground->setChecked( true );
	enableExtendedTooltipOptions( radioExtendedTooltip->isChecked() );
	
	//--- unset the apply button ---
	settingsChanged(false);
}

void
YawpConfigDialog::getData( Yawp::ConfigData * data )
{
	if( ! d->pConfigDlg || !data )
		return;

	//--- SELECTED CITY INDEX, THIS WILL BE THE CITY WE SHOW IN YAWP AFTER CLOSING THE CONFIG DIALOG ---
	QModelIndex selectedCity = cityTableView->currentIndex();
	if( selectedCity.isValid() )
		data->iCityIndex = selectedCity.row();

	//--- UPDATE INTERVAL ---
	int iIdx = comboInterval->currentIndex();
	data->iUpdateInterval = comboInterval->itemData( iIdx ).toInt();
	
	data->iStartDelay = spinboxStartDelay->value();
	
	data->bTraverseLocationsPeriodically = groupBoxTraverseLocationsPeriodically->isChecked();
	data->iTraverseLocationTimeout = spinTraverseLocationsTimeout->value();


	//--- UNIT SYSTEM ---
	int iTempIdx  = comboTemperature->currentIndex();
	int iSpeedIdx = comboSpeed->currentIndex();
	int iPressureIdx = comboPressure->currentIndex();
	int iDistanceIdx = comboDistance->currentIndex();

	d->bUnitsChanged = false;
	if ((int)data->temperatureSystem != comboTemperature->itemData( iTempIdx ).toInt() ||
	    (int)data->speedSystem       != comboSpeed->itemData( iSpeedIdx ).toInt() ||
	    (int)data->pressureSystem    != comboPressure->itemData( iPressureIdx ).toInt() ||
	    (int)data->distanceSystem    != comboDistance->itemData( iDistanceIdx ).toInt() )
	{
		d->bUnitsChanged = true;
	}
	data->temperatureSystem = (YAWP_TEMPERATURE_UNIT)comboTemperature->itemData( iTempIdx ).toInt();
	data->speedSystem       = (YAWP_SPEED_UNIT)comboSpeed->itemData( iSpeedIdx ).toInt();
	data->pressureSystem    = (YAWP_PRESSURE_UNIT)comboPressure->itemData( iPressureIdx ).toInt();
	data->distanceSystem    = (YAWP_DISTANCE_UNIT)comboDistance->itemData( iDistanceIdx ).toInt();


	iIdx = comboPageAnimation->currentIndex();
	data->pageAnimation = (PageAnimator::Transition)comboPageAnimation->itemData( iIdx ).toInt();
	iIdx = comboDayformatAnimation->currentIndex();
	data->daynamesAnimation = (PageAnimator::Transition)comboDayformatAnimation->itemData( iIdx ).toInt();
	iIdx = comboDetailAnimation->currentIndex();
	data->detailsAnimation = (PageAnimator::Transition)comboDetailAnimation->itemData( iIdx ).toInt();
	iIdx = comboIconAnimation->currentIndex();
	data->iconAnimation = (PageAnimator::Transition)comboIconAnimation->itemData( iIdx ).toInt();

	data->iAnimationDuration = sliderAnimationDuration->value();

	//--- THEMENAME ---
	data->bUseCustomTheme = (checkBoxCustomTheme->checkState() == Qt::Checked ? true : false);
	data->sBackgroundName = d->theme_map[comboTheme->currentIndex()];
	data->bUseCustomThemeBackground = (checkBoxUseCustomBackground->checkState() == Qt::Checked ? true : false);
	data->sCustomThemeFile = editCustomThemeFile->text();

	//--- USE CUSTOM COLOR ---
	data->bUseCustomFontColor = (checkBoxUseCustomColor->checkState() == Qt::Checked ? true : false);
	if( data->bUseCustomFontColor )
	{
		data->fontColor = comboColor->color();
		data->lowFontColor = comboLowerColor->color();
		data->shadowsFontColor = comboShadowsColor->color();
	}

	//--- TEXT SHADOWS ---
	data->bDisableTextShadows = (checkBoxDisableTextShadows->checkState() == Qt::Checked ? true : false);

	//--- PANEL PAGE ---
	//
	//--- current day settings ---
	if( radioActualTemp->isChecked() )
		data->todaysWeatherPanelFormat = Yawp::PanelTemperature;
	else if( radioActualIcon->isChecked() )
		data->todaysWeatherPanelFormat = Yawp::PanelIcon;
	else
		data->todaysWeatherPanelFormat = Yawp::PanelTemperature | Yawp::PanelIcon;

	//--- forecast day settings ---
	if( radioForecastTemp->isChecked() )
		data->forecastWeatherPanelFormat = Yawp::PanelTemperature;
	else if( radioForecastIcon->isChecked() )
		data->forecastWeatherPanelFormat = Yawp::PanelIcon;
	else
		data->forecastWeatherPanelFormat = Yawp::PanelTemperature | Yawp::PanelIcon;
	data->iPanelForecastDays = comboForecastDays->currentIndex();

	data->bUseCompactPanelLayout = checkCompactPanelLayout->isChecked();
	data->bUseInteractivePanelWeatherIcons = checkInteractivePanelWeatherIcons->isChecked();
	data->bUseExtendedTooltip = radioExtendedTooltip->isChecked();
	data->extendedTooltipOptions = 0;
	if( checkTooltipSateliteMap->isChecked() )
		data->extendedTooltipOptions |= Yawp::SatellitePage;
	if( checkTooltipPreview->isChecked() )
		data->extendedTooltipOptions |= Yawp::PreviewPage;
	if( checkTooltipThemeBackground->isChecked() )
		data->extendedTooltipOptions |= Yawp::ThemeBackground;
}

void
YawpConfigDialog::changeThemeState(int state)
{
	btnCustomThemeFile->setEnabled( state == Qt::Checked );
	editCustomThemeFile->setEnabled( state == Qt::Checked );
	checkBoxUseCustomBackground->setEnabled( state == Qt::Checked );
}

void
YawpConfigDialog::enableCustomFontColorOptions(int state)
{
	bool bEnabled(state  == Qt::Checked ? true : false);

	label_comboColor->setEnabled(bEnabled);
	label_comboLowerColor->setEnabled(bEnabled);
	label_comboShadowsColor->setEnabled(bEnabled);
	comboColor->setEnabled(bEnabled);
	comboLowerColor->setEnabled(bEnabled);
	comboShadowsColor->setEnabled(bEnabled);
}

void
YawpConfigDialog::sliderAnimationDurationValue( int value )
{
	label_animationDuration->setText( QString("%1 ms").arg(value) );
}

void
YawpConfigDialog::selectCustomThemeFile()
{
	QString sDir;
	if( !editCustomThemeFile->text().isEmpty() )
	{
		QFileInfo info( editCustomThemeFile->text() );
		sDir = info.absolutePath();
	}
	else
		sDir = QDir::homePath();

	QString filename = KFileDialog::getOpenFileName(
		KUrl(sDir),
		"*.svg *.svgz|Scalable Vector Graphics",
		d->pConfigDlg );
	
	if( !filename.isEmpty())
	{
		editCustomThemeFile->setText(filename);
		
		//--- notify config dialog, that settings has been changed ---
		settingsChanged();
	}
}

void
YawpConfigDialog::enableYawpBackground()
{
	comboTheme->setEnabled( !(checkBoxCustomTheme->isChecked() && checkBoxUseCustomBackground->isChecked()) );
}

void
YawpConfigDialog::enableCompactPanelLayout(bool bEnabled)
{
	radioActualIcon->setEnabled( !bEnabled );
	radioActualTemp->setEnabled( !bEnabled );
	radioActualBoth->setEnabled( !bEnabled );
	radioForecastIcon->setEnabled( !bEnabled );
	radioForecastTemp->setEnabled( !bEnabled );
	radioForecastBoth->setEnabled( !bEnabled );
}

void
YawpConfigDialog::enableExtendedTooltipOptions( bool bEnabled )
{
	checkTooltipPreview->setEnabled( bEnabled );
	checkTooltipSateliteMap->setEnabled( bEnabled );
	checkTooltipThemeBackground->setEnabled( bEnabled );
}

void
YawpConfigDialog::addCity()
{
	DlgAddCity dlg( d->pStorage, d->pConfigDlg );
	if( dlg.exec() == QDialog::Accepted )
	{
		CityWeather cityInfo = dlg.getSelectedCity();
		int idx = d->pWeatherModel->addCity( cityInfo );

		if( idx >= 0 )
		{
			cityTableView->setCurrentIndex( d->pWeatherModel->index(idx, 0, QModelIndex()) );
			updateLocationButtons();
			d->bCitySettingsChanged = true;
			d->bCityModelChanged = true;
			
			//--- notify config dialog, that settings has been changed ---
			settingsChanged();
		}
		else
		{
			QMessageBox::information(
				d->pConfigDlg,
				i18n("Error"),
				i18n("City %1 already exists in the list and can not be added again.", cityInfo.city()) );
		}
	}
}

void
YawpConfigDialog::deleteCity()
{
	QModelIndex index = cityTableView->currentIndex();
	if( !index.isValid() )
	{
		KMessageBox::information(
			d->pConfigDlg,
			i18n("No city has been selected to delete."),
			d->pConfigDlg->windowTitle() );
		return;
	}
	const CityWeather * cityInfo = d->pWeatherModel->getCityInfo( index.row() );
	int ret = KMessageBox::questionYesNo(
		d->pConfigDlg,
		i18n("Do you really want to delete city %1?", cityInfo->city()) );
	if( ret == KMessageBox::Yes )
	{
		d->pWeatherModel->removeRows( index.row(), 1 );
		updateLocationButtons();
		d->bCityModelChanged = true;
		
		//--- notify config dialog, that settings has been changed ---
		settingsChanged();
	}
}

void
YawpConfigDialog::moveSelectedCityUp()
{
	moveSelectedCity( -1 );
}

void
YawpConfigDialog::moveSelectedCityDown()
{
	moveSelectedCity( +1 );
}

void
YawpConfigDialog::slotSetTimeZone()
{
	QModelIndex index = cityTableView->currentIndex();
	CityWeather * cityInfo = d->pWeatherModel->getCityInfo( index.row() );
	
	DlgTimeZone dlg(*cityInfo, d->pStorage);
	if( dlg.exec() == QDialog::Accepted )
	{
		cityInfo->setTimeZone( dlg.selectedTimeZone() );
		if( cityInfo->countryCode().isEmpty() )
			cityInfo->setCountryCode( cityInfo->timeZone().countryCode() );
		d->bCitySettingsChanged = true;
		d->bCityModelChanged = true;
		
		//--- notify config dialog, that settings has been changed ---
		settingsChanged();
	}
}

void
YawpConfigDialog::locationSelected( const QModelIndex & index )
{
	int iCities = d->pWeatherModel->rowCount();
	btnMoveUp->setEnabled( index.row() > 0 && iCities > 1 );
	btnMoveDown->setEnabled( index.row()>= 0 && index.row()+1 < iCities );
}

void
YawpConfigDialog::initAnimationCombo( QComboBox * pComboBox )
{
	pComboBox->insertItem(  0, i18n("None"),                    QVariant(PageAnimator::Jump) );
	pComboBox->insertItem(  1, i18n("Crossfade"),               QVariant(PageAnimator::CrossFade) );
	pComboBox->insertItem(  2, i18n("Scanline blend"),          QVariant(PageAnimator::ScanlineBlend) );
	pComboBox->insertItem(  3, i18n("Flip vertically"),         QVariant(PageAnimator::FlipVertically) );
	pComboBox->insertItem(  4, i18n("Flip horizontally"),       QVariant(PageAnimator::FlipHorizontally) );

	if( pComboBox == comboIconAnimation )
		return;

	pComboBox->insertItem(  5, i18n("Roll in vertically"),      QVariant(PageAnimator::RollInVertically) );
	pComboBox->insertItem(  6, i18n("Roll out vertically"),     QVariant(PageAnimator::RollOutVertically) );
	pComboBox->insertItem(  7, i18n("Roll in horizontally"),    QVariant(PageAnimator::RollInHorizontally) );
	pComboBox->insertItem(  8, i18n("Roll out horizontally"),   QVariant(PageAnimator::RollOutHorizontally) );
	pComboBox->insertItem(  9, i18n("Slide in vertically"),     QVariant(PageAnimator::SlideInVertically) );
	pComboBox->insertItem( 10, i18n("Slide out vertically"),    QVariant(PageAnimator::SlideOutVertically) );
	pComboBox->insertItem( 11, i18n("Slide in horizontally"),   QVariant(PageAnimator::SlideInHorizontally) );
	pComboBox->insertItem( 12, i18n("Slide out horizontally"),  QVariant(PageAnimator::SlideOutHorizontally) );
	pComboBox->insertItem( 13, i18n("Open vertically"),         QVariant(PageAnimator::OpenVertically) );
	pComboBox->insertItem( 14, i18n("Close vertically"),        QVariant(PageAnimator::CloseVertically) );
	pComboBox->insertItem( 15, i18n("Open horizontally"),       QVariant(PageAnimator::OpenHorizontally) );
	pComboBox->insertItem( 16, i18n("Close horizontally"),      QVariant(PageAnimator::CloseHorizontally) );
}

void
YawpConfigDialog::updateLocationButtons()
{
	int iCities = d->pWeatherModel->rowCount();
	btnAddCity->setEnabled( d->pStorage->ionListModel()->rowCount() > 0 && iCities < MAXCITIES );
	btnDeleteCity->setEnabled( iCities > 0 );
	btnSetTimeZone->setEnabled( iCities > 0 );
	locationSelected( cityTableView->currentIndex() );
}

void
YawpConfigDialog::moveSelectedCity( int offset )
{
	QModelIndex index = cityTableView->currentIndex();
	if( index.isValid() && d->pWeatherModel->moveCity( index.row(), index.row()+offset ) )
	{
		index = index.sibling(index.row()+offset, 0);
		cityTableView->setCurrentIndex( index );
		locationSelected( index );
		d->bCityModelChanged = true;
		
		//--- notify config dialog, that settings has been changed ---
		settingsChanged();
	}
}

void
YawpConfigDialog::settingsChanged(bool changed)
{
	d->pConfigDlg->enableButton(KDialog::Ok, changed);
	d->pConfigDlg->enableButton(KDialog::Apply, changed);
}
