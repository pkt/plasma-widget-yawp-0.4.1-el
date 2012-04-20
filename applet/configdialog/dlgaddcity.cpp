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
#include "countrymap.h"
#include "dlgaddcity.h"
#include "ionlistmodel.h"
#include "utils.h"
#include "yawpdefines.h"
#include "logger/streamlogger.h"

//--- QT4 ---
#include <QPushButton>

//--- KDE4 ---
#include <KMessageBox>
#include <KProgressDialog>
#include <Plasma/DataEngine>

#define CityRole		Qt::UserRole
#define CountryRole		Qt::UserRole + 1
#define CountryCodeRole		Qt::UserRole + 2
#define ExtraDataRole		Qt::UserRole + 3
#define ProviderRole		Qt::UserRole + 4


DlgAddCity::DlgAddCity( Yawp::Storage * pStorage, QWidget * parent )
	: QDialog( parent )
{
	m_pStorage = pStorage;
	
	setupUi( this );

	bttnFind->setIcon( KIcon("edit-find") );
	buttonBox->button( QDialogButtonBox::Apply )->setIcon( KIcon("dialog-ok") );
	buttonBox->button( QDialogButtonBox::Cancel )->setIcon( KIcon("dialog-cancel") );

	//--- workarround for the not working accept signal ---
	connect(buttonBox,     SIGNAL(clicked(QAbstractButton *)),   this, SLOT(slotApplySelection(QAbstractButton *)));
	connect(bttnFind,      SIGNAL(released()),                   this, SLOT(slotFindLocations()));
	connect(editLocation,  SIGNAL(textChanged(const QString &)), this, SLOT(slotValidateTextInput(const QString &)));

	comboProvider->clear();
	comboProvider->setModel( m_pStorage->ionListModel() );
	bttnFind->setEnabled( false );
	slotValidateTextInput(editLocation->text());
	enableApply();
}

DlgAddCity::~DlgAddCity()
{
}

CityWeather
DlgAddCity::getSelectedCity() const
{
	dStartFunct();
	const QListWidgetItem * item = cityList->currentItem();
	CityWeather cityInfo;
	if( item )
	{
		cityInfo.setCity(        QUrl::fromPercentEncoding(item->data(CityRole).toString().toUtf8()) );
		cityInfo.setCountry(     QUrl::fromPercentEncoding(item->data(CountryRole).toString().toUtf8()) );
		cityInfo.setCountryCode( QUrl::fromPercentEncoding(item->data(CountryCodeRole).toString().toUtf8()) );
		cityInfo.setExtraData(   QUrl::fromPercentEncoding(item->data(ExtraDataRole).toString().toUtf8()) );
		cityInfo.setProvider(    QUrl::fromPercentEncoding(item->data(ProviderRole).toString().toUtf8()) );
		
		dTracing() << "Requested preselected timezones";
		QStringList vTimeZones( Utils::GetTimeZones( cityInfo, m_pStorage ) );
		if( vTimeZones.count() == 1 )
			cityInfo.setTimeZone( vTimeZones.at(0) );
	}
	dDebug() << cityInfo.city() << cityInfo.country() << cityInfo.countryCode() << cityInfo.extraData() << cityInfo.provider();
	dEndFunct();
	return cityInfo;
}

void
DlgAddCity::updateLocations( const QHash<QString, QVariant> & container )
{
	dStartFunct();
	QVariant data( container.value("validate") );
	if( !data.toBool() || data.type() != QVariant::String )
	{
		dWarning() << "Invalid data...";
		return;
	}

	dDebug() << "City search result:" << data;
	QStringList vTokens = data.toString().split("|");
	if( vTokens.count() >= 3 && vTokens.at(1).compare("valid") == 0 )
	{
		if( vTokens.at(2).compare("single") == 0 ||
		    vTokens.at(2).compare("multiple") == 0 )
		{
			int iPos = 3;
			QString sLocation, sExtra;

//			dDebug() << "Tokenized search result:" << vTokens;

			//--- go through all places and extract all informations ---
			while( iPos < vTokens.count() && vTokens.at(iPos).compare("place") == 0 )
			{
				sLocation = vTokens.at(iPos+1);
				sExtra.clear();

				//--- go through all attributes that belongs to the current place ---
				iPos+=2;
				while( iPos+1 < vTokens.count() && vTokens.at(iPos).compare("place") != 0 )
				{
					if( vTokens.at(iPos).compare("extra") == 0 )
						sExtra = vTokens.at(iPos+1);
					iPos+=2;
				}

				if( !sLocation.isEmpty() )
				{
					QListWidgetItem * item = new QListWidgetItem;

					if( !sExtra.isEmpty() && container.contains(sExtra) )
					{
						QString sStationType, sDistance, sItemText;
						getExtraLocationData( container.value(sExtra), sStationType, sDistance );
						
						if (sStationType.compare("airport", Qt::CaseInsensitive) == 0)
							sItemText += i18n(sStationType.toUtf8().constData()) + ": ";
						
						sItemText += sLocation;
						
						if (!sDistance.isEmpty())
							sItemText += QString(" (%1 %2)").arg(i18n("distance:")).arg(sDistance);
						
						item->setText(sItemText);
					}
					else
						item->setText(sLocation);

					/*  We can not change the value sLocation, since some
					 *  providers or dataengines use this as a key to find the specific city!!!
					 */
					item->setData( CityRole, sLocation );
					item->setData( ExtraDataRole, sExtra );
					item->setData( ProviderRole, vTokens.at(0) );
					
					QString sCity, sDistrict, sCountry, sCountryCode;
					Utils::ExtractLocationInfo(sLocation, sCity, sDistrict, sCountry);
					if( Utils::GetCountryCode(sCountry, sCountryCode, m_pStorage) )
					{
						item->setData( CountryCodeRole, sCountryCode );
						item->setData( CountryRole, sCountry );
						
						QPixmap flag = m_pStorage->countryMap()->getPixmapForCountryCode( sCountryCode );
						item->setIcon(QIcon(flag));
					}
					cityList->addItem( item );
				}
			}
		}
	}
	else if( vTokens.count() >= 2 && vTokens.at(1).compare("timeout") == 0 )
	{
		KMessageBox::error(this,
			i18n("The applet was not able to contact the server, please try again later."));
//		dDebug() << vTokens;
	}
	else if( vTokens.count() >= 2 && vTokens.at(1).compare("malformed") == 0 )
	{
		KMessageBox::error(this,
			i18n("Ion has rejected invalid or malformed command."));
	} 
	else if( vTokens.count() >= 4 )
	{
		KMessageBox::error(this,
			i18n("The place '%1' is not valid. The weather-service is not able to find this place.",
			vTokens.at(3)));
//		dDebug() << vTokens;
	}
	else
		KMessageBox::error(this, i18n("The applet was not able to contact the server, due to an unknown error."));
	
	dEndFunct();
}

void
DlgAddCity::enableApply()
{
	dStartFunct();
	QPushButton * bttnApply = buttonBox->button(QDialogButtonBox::Apply);
	bttnApply->setEnabled( cityList->count() > 0 );
	dEndFunct();
}

void
DlgAddCity::slotValidateTextInput(const QString & text)
{
	bttnFind->setEnabled( !text.isEmpty() );
}

void
DlgAddCity::slotFindLocations()
{
	dStartFunct();
	if( comboProvider->count() == 0 || editLocation->text().isEmpty() )
	{
		KMessageBox::sorry( this, i18n("You have to enter a city to search for.") );
	}
	else
	{
		cityList->clear();
		enableApply();
		bttnFind->setEnabled( false );

		m_pProgressDlg = new KProgressDialog(this, i18nc("Progress dialog caption", "Please wait..."),
			i18nc("Progress dialog description","%1 is retrieving the list of cities from the internet, "
			"please wait.",  YAWP_NAME));
		m_pProgressDlg->progressBar()->setMinimum(0);
		m_pProgressDlg->progressBar()->setMaximum(0);
		m_pProgressDlg->setAllowCancel(true);
		m_pProgressDlg->setModal(true);
		m_pProgressDlg->setAutoClose(true);
		m_pProgressDlg->setVisible(true);
		
		connect(m_pProgressDlg, SIGNAL(destroyed()), this, SLOT(slotAbort()));

		int iProviderIndex = comboProvider->currentIndex();
		dTracing() << "CurrentProvider: " << iProviderIndex;
		
		QString sProvider = comboProvider->itemData( iProviderIndex >= 0 ? iProviderIndex : 0 ).toString();
        QString sLocation = editLocation->text();
		m_sCurrentAction  = QString("%1|validate|%2").arg(sProvider).arg(sLocation);
		dTracing() << "Using command: " << m_sCurrentAction;
		
		m_pStorage->ionListModel()->engine()->connectSource(m_sCurrentAction, this);
	}
	dEndFunct();
}

void
DlgAddCity::dataUpdated(const QString & sAction, const Plasma::DataEngine::Data & data)
{
	if( m_pProgressDlg == NULL )
		return;

	dStartFunct();
	m_pStorage->ionListModel()->engine()->disconnectSource(sAction, this);

	QStringList actionList = sAction.split("|");

	if( actionList.count() >= 3 && actionList.at(1).compare("validate") == 0 )
	{
		updateLocations(data);
		bttnFind->setEnabled( true );

		m_pProgressDlg->setVisible(false);
		m_pProgressDlg->deleteLater();
	}
	enableApply();

	dEndFunct();
}

void
DlgAddCity::slotApplySelection( QAbstractButton * button )
{
	if( m_pProgressDlg != NULL )
		m_pProgressDlg->deleteLater();
	
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

void
DlgAddCity::slotAbort()
{
	dStartFunct();
	if( m_pProgressDlg )
	{
		m_pStorage->ionListModel()->engine()->disconnectSource(m_sCurrentAction, this);
		m_pProgressDlg->setVisible(false);
		m_pProgressDlg->deleteLater();
		bttnFind->setEnabled( true );
	}
	dEndFunct();
}

void
DlgAddCity::getExtraLocationData( const QVariant & data, QString & sStationType, QString & sDistance ) const
{
	if( data.type() != QVariant::String || !data.toBool() )
		return;
	QStringList vTokens( data.toString().split(QChar('|')) );
	for( int idx=0; idx+1 <= vTokens.count(); idx+=2 )
	{
		if( vTokens.at(idx).compare("stationtype") == 0 )
			sStationType = vTokens.at(idx+1);
		else if( vTokens.at(idx).compare("distance") == 0 )
			sDistance = vTokens.at(idx+1);
	}
}
