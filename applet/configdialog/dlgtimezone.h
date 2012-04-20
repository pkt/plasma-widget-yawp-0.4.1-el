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
 
#ifndef DIALOG_TIMEZONE_H
#define DIALOG_TIMEZONE_H

#include "ui_dlgtimezone.h"
#include "yawpday.h"

//--- QT4 ---
#include <QDialog>

namespace Yawp
{
	class Storage;
}

class DlgTimeZone : public QDialog,
                   private Ui::DlgTimeZone
{
	Q_OBJECT
public:
	DlgTimeZone(const CityWeather & cityInfo, const Yawp::Storage * pStorage, QWidget * parent = NULL);
	~DlgTimeZone();
	
	QString selectedTimeZone() const;

	
private Q_SLOTS:
	void slotShowAll();
	void slotShowPreselected();
	void slotApplySelection( QAbstractButton * button );
	
private:
	void showAllTimeZones(bool showAll);
	
	bool m_bShowAll;
	const Yawp::Storage * m_pStorage;
};

#endif	// DIALOG_TIMEZONE_H
