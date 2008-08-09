#!/bin/bash

NAME="$1"
UPPER="`echo $NAME | tr \"[:lower:]\" \"[:upper:]\"`"
LOWER="`echo $NAME | tr \"[:upper:]\" \"[:lower:]\"`"

if [ "$NAME" = "$LOWER" ] || [ "$NAME" = "$UPPER" ] 
then
	echo 'Error: name must contain upper and lower case chars'
	exit 1
fi

mkdir -p ../$LOWER

EXTS="h cpp pro vcproj sln"
for e in $EXTS
do
   if [ -e "template.$e" ]
	then
	  sed "s/Template/$NAME/g" template.$e | sed "s/template/$LOWER/g" | sed "s/TEMPLATE/$UPPER/g" > ../$LOWER/$LOWER.$e
	fi
done

if [ "`grep -l \"SUBDIRS+=$LOWER\" ../plugins.pro`" = "" ]
then
	echo "SUBDIRS+=$LOWER" >> ../plugins.pro
fi
