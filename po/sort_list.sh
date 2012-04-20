#!/bin/sh

#
# Sort the list
#

  LIST=$1

  if [ -z "$LIST" ]
  then
    echo ">>ERR<< sort_list() - missing parameter LIST - exiting"
    exit 1
  fi

  if [ -f "$LIST" ]
  then
    echo "Sorting $LIST"
    mv "$LIST" "${LIST}.unsorted"
    LANG=C sort -u "${LIST}.unsorted" > "$LIST" && rm "${LIST}.unsorted"
    echo "Done"
  else
    echo ">>ERR<< sort_list() - file $LIST does not exist."
  fi

