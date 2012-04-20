#!/bin/sh
#
# Script to update translation catalog files
#

BASEDIR="../" # root of translatable sources
PROJECT="plasma_applet_yawp" # project name
BUGADDR="http://www.kde-look.org/content/show.php?content=94106" # MSGID-Bugs
WDIR=`pwd` # working dir
DAYLIST=lists/days.list
DAYLISTLONG=lists/days_long.list
WEATHERLIST=lists/weather.list
COUNTRYLIST=lists/countries.list
CITYLIST=lists/cities.list
WINDLIST=lists/wind.list
PRESSURELIST=lists/pressure.list

add_list()
{
  LIST=$1
  HINT=$2

  if [ -z "$LIST" ]
  then
    echo ">>ERR<< add_list() - missing parameter LIST - exiting"
    return
  fi

  if [ -f "$LIST" ]
  then
    cat $LIST | while read ROW
    do
      if [ -z "$HINT" ]
      then
        echo "tr2i18n(\"${ROW}\")" >> ${WDIR}/rc.cpp
      else
        echo "i18nc(\"${HINT}\", \"${ROW}\")" >> ${WDIR}/rc.cpp
      fi
    done
  else
    echo ">>ERR<< add_list() - file $LIST does not exist."
  fi

}

echo "Preparing rc files"
cd ${BASEDIR}
# we use simple sorting to make sure the lines do not jump around too much from system to system
find . -name '*.rc' -o -name '*.ui' -o -name '*.kcfg' |grep -v "unittest" | sort > ${WDIR}/rcfiles.list
xargs --arg-file=${WDIR}/rcfiles.list extractrc > ${WDIR}/rc.cpp
# additional string for KAboutData
#echo 'i18nc("NAME OF TRANSLATORS","Your names");' >> ${WDIR}/rc.cpp
#echo 'i18nc("EMAIL OF TRANSLATORS","Your emails");' >> ${WDIR}/rc.cpp

cd ${WDIR}

# Add Days
add_list "$DAYLIST"

# Add Long day names
add_list "$DAYLISTLONG" "Forecast for day"


# Add weather descriptions
add_list "$WEATHERLIST" "Forecast description"

# Add wind directions descriptions
add_list "$WINDLIST" "Wind direction"

# Add the countries for translation
add_list $COUNTRYLIST "Country or state"

# Add pressure list 
add_list "$PRESSURELIST" "Pressure"


# Add cities
#add_list $CITYLIST

echo "Done preparing rc files"


echo "Extracting messages"
cd ${BASEDIR}
# see above on sorting
find . -name '*.cpp' -o -name '*.h' -o -name '*.c' |grep -v "unittest"| sort > ${WDIR}/infiles.list
echo "rc.cpp" >> ${WDIR}/infiles.list
cd ${WDIR}
xgettext --from-code=UTF-8 -C -kde -ci18n -ki18n:1 -ki18nc:1c,2 -ki18np:1,2 -ki18ncp:1c,2,3 -ktr2i18n:1 \
-kI18N_NOOP:1 -kI18N_NOOP2:1c,2 -kaliasLocale -kki18n:1 -kki18nc:1c,2 -kki18np:1,2 -kki18ncp:1c,2,3 \
--msgid-bugs-address="${BUGADDR}" \
--files-from=infiles.list -D ${BASEDIR} -D ${WDIR} -o ${PROJECT}.pot || { echo "error while calling xgettext. aborting."; exit 1; }
echo "Done extracting messages"


echo "Merging translations"
if [ $# -eq 1 ]
then
  catalogs=`find . -name "${1}.po"`
else
  catalogs=`find . -name '*.po'`
fi
for cat in $catalogs
do
  echo $cat
  msgmerge -o $cat.new $cat ${PROJECT}.pot
  mv $cat.new $cat
done
echo "Done merging translations"


echo "Cleaning up"
cd ${WDIR}
rm rcfiles.list
rm infiles.list
rm rc.cpp
echo "Done"

