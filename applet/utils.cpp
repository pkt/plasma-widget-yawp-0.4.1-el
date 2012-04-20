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
#include "utils.h"
#include "countrymap.h"
#include "yawpday.h"
#include "logger/streamlogger.h"

//--- QT4 ---
#include <QDesktopServices>
#include <QTime>
#include <QUrl>

//--- KDE4 ---
#include <KLocalizedString>
#include <KMessageBox>
#include <KMimeTypeTrader>
#include <KService>
#include <KRun>
#include <KUrl>

namespace Utils
{

void
OpenUrl(const QString & url)
{
	KRun::runUrl(KUrl(url), QString::fromLatin1("text/html"), 0);
}

bool
GetCountryCode( const QString & sCountry, QString & sCountryCode, const Yawp::Storage * pStorage )
{
	sCountryCode.clear();
	if( sCountry.isEmpty() )
		return false;

	dStartFunct();
	QString sTmpCountry;
	/*** Handle special cases for country names here ***
	*    Some countries contains an article (e.g.: accuweather "Harbour Island, The Bahamas(Harbour Island)"
	*    in this case we will not find the countrycode for this country.
	*/
	if( sCountry.left(4).compare("the ", Qt::CaseInsensitive) == 0 )
		sTmpCountry = sCountry.right( sCountry.length()-4 ).simplified();
	else if( sCountry.compare("uk", Qt::CaseInsensitive) == 0 )
		sTmpCountry = "United Kingdom";
	else if( sCountry.compare("usa", Qt::CaseInsensitive) == 0 )
		sTmpCountry = "us";
	else
		sTmpCountry = sCountry;

	if( !pStorage->unitedStatesMap()->state(sTmpCountry).isEmpty() )
	{
		sCountryCode = "us";
	}
	else if( !pStorage->unitedStatesMap()->stateCode(sTmpCountry).isEmpty() )
	{
		sCountryCode = "us";
	}
	if( sCountryCode.isEmpty() )
	{
		sCountryCode = pStorage->countryMap()->countryCode(sTmpCountry);
	}
	if( sCountryCode.isEmpty() &&
		!pStorage->countryMap()->country(sTmpCountry).isEmpty() )
	{
		sCountryCode = sTmpCountry.toLower();
	}

	dEndFunct();
	return !sCountryCode.isEmpty();
}

void
ExtractLocationInfo( const QString & sLocation,
                     QString & sCity,
                     QString & sDistrict,
                     QString & sCountry )
{
	QString sTmpLocation;
	sCity.clear();
	sDistrict.clear();
	sCountry.clear();

	/*** Extract the district ...(DISTRICT)...
	*/
	int iPos = sLocation.indexOf('(');
	if( iPos > 0 )
	{
		int iClose = sLocation.lastIndexOf(')');
		if( iClose > iPos )
		{
			sDistrict = sLocation.mid(iPos+1, iClose-iPos-1).simplified();
			sTmpLocation = sLocation.left(iPos) + sLocation.right(sLocation.length()-iClose-1);
		}
	}

	/*** Tokenize the cityname without the district, because some providers uses braces in the district as well.
	*    e.g.: accuweather: "London, United Kingdom(Westminster, England)"
	*/
	const QString * pLocation = sTmpLocation.isEmpty() ? &sLocation : &sTmpLocation;
	iPos = pLocation->lastIndexOf(',');
	if( iPos > 0 )
	{
		/*** when we have no district yet, than interpret the location string like this:
		*    CITY_NAME, DISTRICT, STATE
		*/
		if( sDistrict.isEmpty() )
		{
			int iDistrictPos = pLocation->lastIndexOf(',', iPos-1);
			dTracing() << "DistrictPos =" << iDistrictPos;
			if( iDistrictPos > 0 )
			{
				sCity = pLocation->left(iDistrictPos).simplified();
				sDistrict = pLocation->mid(iDistrictPos+1, iPos-iDistrictPos-1).simplified();
			}
		}
		if( sCity.isEmpty() )
			sCity = pLocation->left(iPos).simplified();
		sCountry = pLocation->right(pLocation->length()-iPos-1).simplified();
	}
	else
		sCity = pLocation->simplified();
}

QString
LocalizedWeatherString(const QString sWeather )
{
// 	kDebug() << "sWeather: " << sWeather << endl;

	/* normalize string:
	 *  - all letters to lower case
	 *  - only one space
	 *  - replace " and " by "# "
	 *  - replace " with " by "@with " )
	 */
	QString sNormalized = sWeather.toLower().replace(QRegExp(" +")," ").replace(" and ","# ").replace(" with ","@with ");

	// split to parts, translate them and create traslated string
	// parts can be split by coma, semicolon, hash (and) or at (with)
	QString sResult;
	QStringList vSemiTokens = sNormalized.split(';');
	QStringList vColTokens;
	QStringList vAndTokens;
	QStringList vWithTokens;

	for (int i = 0; i < vSemiTokens.count(); i++)
	{ // Semicolon
// 		kDebug() << " -> vSemiTokens[" << i << "]: " << vSemiTokens[i] << endl;
		vColTokens = vSemiTokens[i].split(',');
		for (int j = 0; j < vColTokens.count(); j++)
		{ // Coma
// 			kDebug() << " ---> vColTokens["<<j<<"]: " << vColTokens[j] << endl;
			vAndTokens = vColTokens[j].split('#');
			for (int k = 0; k < vAndTokens.count(); k++)
			{ // hash (#)
				if (k > 0)
					sResult += " " + i18nc("Forecast description","and") + " ";
// 				kDebug() << " ---> vAndTokens["<<k<<"]: " << vAndTokens[k] << endl;
				vWithTokens = vAndTokens[k].split('@');
				for (int l = 0; l < vWithTokens.count(); l++)
				{ // at (@)
// 					kDebug() << " ----> vWithTokens["<<l<<"]: " << vWithTokens[l] << endl;
					if ( l > 0 )
						sResult += " ";
					sResult += i18nc("Forecast description",vWithTokens[l].trimmed().toLocal8Bit().data());
				}
			}
			if (j < vColTokens.count() - 1)
				sResult += ", ";
		}
		if (i < vSemiTokens.count() - 1)
			sResult += "; ";
	}

	// First letter to Upper case
	sResult[0] = sResult[0].toUpper();

	// return unified string
	return sResult;
}

QStringList
GetTimeZones( const CityWeather & cityInfo, const Yawp::Storage * pStorage )
{
	dDebug() << cityInfo.localizedCityString() << "  countrycode =" << cityInfo.countryCode()
		<< "  country =" << cityInfo.country();

	QStringList vTimeZones;
	if( cityInfo.countryCode().compare("us", Qt::CaseInsensitive) == 0 )
	{
		if( cityInfo.country().length() > 2 )
		{
			QString sCode = pStorage->unitedStatesMap()->stateCode(cityInfo.country());
			if( !sCode.isEmpty() )
				vTimeZones = pStorage->unitedStatesMap()->timeZones(sCode);
		}
		if( vTimeZones.isEmpty() )
			vTimeZones = pStorage->unitedStatesMap()->timeZones(cityInfo.country());
	}
	if( vTimeZones.isEmpty() )
		vTimeZones = pStorage->countryMap()->timeZones(cityInfo.countryCode());
	return vTimeZones;
}


}	// end of namespace Utils
