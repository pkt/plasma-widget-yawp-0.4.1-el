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

#ifndef DIALOG_ADD_CITY
#define DIALOG_ADD_CITY

//--- LOCAL ---
#include "ui_dlgaddcity.h"
#include "yawpday.h"

//--- QT4 ---
#include <QDialog>
#include <QIcon>
#include <QPointer>

//--- KDE4 ---
#include <Plasma/DataEngine>

namespace Yawp
{
	class Storage;
}
class QAbstractButton;
class KProgressDialog;

class DlgAddCity : public QDialog,
                   private Ui::DlgAddCity
{
	Q_OBJECT
public:
	DlgAddCity( Yawp::Storage * pStorage, QWidget * parent = NULL);
	~DlgAddCity();

	CityWeather getSelectedCity() const;

private Q_SLOTS:
	void slotAbort();
	void slotValidateTextInput(const QString & text);
	void slotFindLocations();
	void dataUpdated(const QString & sAction, const Plasma::DataEngine::Data & data);
	void slotApplySelection(QAbstractButton *);

private:
	void updateLocations( const QHash<QString, QVariant> & data );
	void enableApply();
	
	void getExtraLocationData( const QVariant & data, QString & sStationType, QString & sDistance ) const;

	QPointer<KProgressDialog>    m_pProgressDlg;
	Yawp::Storage              * m_pStorage;
	QString                      m_sCurrentAction;
};

#endif
