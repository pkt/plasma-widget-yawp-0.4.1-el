#!/bin/bash
#
# Script to extract a new weather descriptions 
# from .xsession-errors log files
# 
# NOTE: yaWP have to be compiled with debug enabled
#            ( install.sh -d )
# 

if [ ! -f plasma_applet_yawp.pot ]
then
  echo "Error: This is not correct directory"
  echo "       file plasma_applet_yawp.pot missing."
  exit
fi

[ -f lists/weather.list.new ] && rm -f lists/weather.list.new

mv lists/weather.list lists/weather.list.new

# Collect all weather strings from logs
count_before=$(cat lists/weather.list.new |wc -l)

FILES=~/.xsession-errors*
[ -f /tmp/yawp.log ] && FILES="$FILES /tmp/yawp.log"

for FILE in $(ls -1 $FILES )
do
  echo "Processing file: $FILE"
  grep "WeatherDataProcessor::updateLoca" "$FILE" |sed 's/) ,  (/\n/g' | grep "Short Forecast Day" \
       |cut -d "|" -f 3 |sort -u >>lists/weather.list.new
  grep "WeatherDataProcessor::updateLoca" "$FILE" |sed 's/) ,  (/\n/g' | grep "Current Conditions" \
       |cut -d "," -f 3 |sed -e 's/ "//g' -e 's/")//g' |sort -u >>lists/weather.list.new
done

# process new strings
# - all lower case
# - split by coma, semicolon, "and" and "with"

LANG=C
export IFS=";,#@"
(cat lists/weather.list.new |tr '[A-Z]' '[a-z]' | sed 's/ and /#/g'| sed 's/ with /@with /g' |while read S1 S2 S3 S4 S5
 do 
   echo "$S1" |sed -e 's/^ *//' -e 's/ $//'
   [ "$S2" ] && echo "$S2" |sed -e 's/^ *//' -e 's/ $//'
   [ "$S3" ] && echo "$S3" |sed -e 's/^ *//' -e 's/ $//'
   [ "$S4" ] && echo "$S4" |sed -e 's/^ *//' -e 's/ $//'
   [ "$S5" ] && echo "$S5" |sed -e 's/^ *//' -e 's/ $//'
 done) |sed -E 's/ +/ /g' |sort -u >lists/weather.list

rm -f lists/weather.list.new

# count number of newly added files
count_after=$(cat lists/weather.list |wc -l)
(( count_new = count_after - count_before ))

echo
if [ $count_new -gt 0 ]
then
  echo "Added ${count_new} new strings."
else
  echo "No new strings added"
fi

