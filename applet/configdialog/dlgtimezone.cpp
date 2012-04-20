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

#include "logger/streamlogger.h"
#include "dlgtimezone.h"
#include "countrymap.h"
#include "utils.h"
#include "../config.h"

#include <KSystemTimeZone>


DlgTimeZone::DlgTimeZone(const CityWeather & cityInfo, const Yawp::Storage * pStorage, QWidget * parent)
	: QDialog(parent),
	  m_pStorage( pStorage )
{
	setupUi( this );
	setWindowTitle( i18nc("yaWP Settings Dialog", "%1 Settings", YAWP_NAME) );

	QString sHeader;
	sHeader = i18n("Select the timezone for <b>%1</b>.", cityInfo.localizedCityString());
	labelHeader->setText(sHeader);

	if( !cityInfo.countryCode().isEmpty() )
	{
		QStringList vTimeZones( Utils::GetTimeZones(cityInfo, pStorage) );
		if( vTimeZones.count() > 0 )
			cmbPreselected->addItems(vTimeZones);
	}

	const QMap<QString, KTimeZone> & map = KSystemTimeZones::timeZones()->zones();
	QMap<QString, KTimeZone>::const_iterator it = map.begin();
	while( it != map.end() )
	{
		QIcon flag(m_pStorage->countryMap()->getPixmapForCountryCode(it.value().countryCode()));
		cmbAll->addItem(flag, it.key());
		it++;
	}

	btnShowPreselected->setEnabled( cmbPreselected->count() > 0 );
	showAllTimeZones( !cmbPreselected->count() > 0 );

	connect(btnShowAll, SIGNAL(clicked()), this, SLOT(slotShowAll()));
	connect(btnShowPreselected, SIGNAL(clicked()), this, SLOT(slotShowPreselected()));
	connect(buttonBox,     SIGNAL(clicked(QAbstractButton *)),   this, SLOT(slotApplySelection(QAbstractButton *)));

}

DlgTimeZone::~DlgTimeZone()
{
}

QString
DlgTimeZone::selectedTimeZone() const
{
	if( m_bShowAll )
		return cmbAll->currentText();
	return cmbPreselected->currentText();
}

void
DlgTimeZone::showAllTimeZones(bool showAll)
{
	m_bShowAll = showAll;

	labelPreselected->setVisible( !showAll );
	labelAll->setVisible( showAll );

	cmbPreselected->setVisible( !showAll );
	cmbAll->setVisible( showAll );

	btnShowPreselected->setVisible( showAll );
	btnShowAll->setVisible( !showAll );
}

void
DlgTimeZone::slotShowAll()
{
	showAllTimeZones(true);
}

void
DlgTimeZone::slotShowPreselected()
{
	showAllTimeZones(false);
}

void
DlgTimeZone::slotApplySelection( QAbstractButton * button )
{
	switch( buttonBox->buttonRole( button ) )
	{
	case QDialogButtonBox::ApplyRole:
		accept();
		break;
	case QDialogButtonBox::RejectRole:
		reject();
		break;
	default:
		break;
	}
}
