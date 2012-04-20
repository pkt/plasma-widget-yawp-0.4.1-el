#!/bin/bash

FILE="file.svg"

if [ -d "$1" ]; then
    cd "$1"
    FILE=$2
else
    FILE=$1
fi

if [ -z "$FILE" ]; then
    echo "Imbeds all png's in a directory into a svg"
    echo "Useage:"
    echo "make_svg_icons.sh [directory] [file_name.svg]"
    exit
fi

printf "" > $FILE

printf  "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n\
   <!-- Created with Inkscape (http://www.inkscape.org/) -->\n\
   <svg\n\
   xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n\
   xmlns:cc=\"http://creativecommons.org/ns#\"\n\
   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n\
   xmlns:svg=\"http://www.w3.org/2000/svg\"\n\
   xmlns=\"http://www.w3.org/2000/svg\"\n\
   xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n\
   xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n\
   xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n\
   width=\"744.09448819\"\n\
   height=\"1052.3622047\"\n\
   id=\"svg2\"\n\
   sodipodi:version=\"0.32\"\n\
   inkscape:version=\"0.46\"\n\
   sodipodi:docname=\"$FILE\"\n\
   inkscape:output_extension=\"org.inkscape.output.svg.inkscape\">\n" >> $FILE

for ICON in `ls *.png`; do 
	printf "<image\n\
              y=\"500.3622\"\n\
              x=\"300.99988\"\n\
              id=\"$ICON\"\n\
              height=\"128\"\n\
              width=\"128\"\n\
              xlink:href=\"data:image/png;base64," >> $FILE; 
	base64 $ICON >> $FILE; 
    echo '" />' >> $FILE;
done

printf  '</svg>' >> $FILE

