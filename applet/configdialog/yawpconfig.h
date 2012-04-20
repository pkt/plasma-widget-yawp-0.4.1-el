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

#ifndef YAWPCONFIG_H
#define YAWPCONFIG_H

//--- LOCAL ---
#include "yawpdefines.h"
#include "utils.h"
#include "ui_yawplocationspage.h"
#include "ui_yawpsettingspage.h"
#include "ui_yawppanelpage.h"
#include "ui_yawpthemepage.h"

//--- QT4 ---
#include <QButtonGroup>
#include <QObject>
#include <QMap>

//--- KDE4 ---
#include <Plasma/DataEngine>
#include "../yawpdefines.h"

namespace Yawp
{
	class Storage;
}
class WeatherServiceModel;
class KProgressDialog;
class KConfigDialog;


/** Configuration window.
	@author Ruan <ruans@kr8.co.za>
 */
class YawpConfigDialog : public QObject,
                         private Ui::LocationsPage,
                         private Ui::SettingsPage,
                         private Ui::PanelPage,
                         private Ui::ThemePage
{
	Q_OBJECT
public:
	YawpConfigDialog( KConfigDialog * parent, Yawp::Storage * storage );
	virtual ~YawpConfigDialog();

	void setData( const Yawp::ConfigData * data );
	void getData( Yawp::ConfigData * data );

	const WeatherServiceModel * weatherModel() const;
	void copyCities( const WeatherServiceModel * model );

	bool cityModelChanged() const;
	bool citySettingsChanged() const;
	bool unitsChanged() const;
	
	// reset the boolean values cityModelChanged, citySettingsChanged and unitsChanged
	// when we receive accept signal from the config dialog, we have to reset this values,
	// otherwise we will request new weather data every time, we click accept.
	void resetChanges();

signals:
	void save();
	
private slots:
	void settingsChanged(bool changed = true);

	void sliderAnimationDurationValue(int ms);

	void changeThemeState(int state);
	void selectCustomThemeFile();
	void enableYawpBackground();
	void enableCustomFontColorOptions(int state);

	void enableCompactPanelLayout(bool enabled);
	void enableExtendedTooltipOptions(bool enabled);

	void addCity();
	void deleteCity();
	void moveSelectedCityUp();
	void moveSelectedCityDown();
	void locationSelected( const QModelIndex & index );
	
	void slotSetTimeZone();

private:
	void updateLocationButtons();
	void initAnimationCombo( QComboBox * pComboBox );
	void moveSelectedCity( int offset );

private:
	struct Private;
	Private * d;
};

#endif //YAWP_CONFIG_H
