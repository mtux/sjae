#!/bin/bash

NAME="$1"
UPPER="`echo $NAME | tr \"[:lower:]\" \"[:upper:]\"`"
LOWER="`echo $NAME | tr \"[:upper:]\" \"[:lower:]\"`"

# process arguments
DO_OPTIONS=0
DO_INTERFACE=0
while [ $# -ge 1 ]; do
	if [ "$1" == "--options" ]
	then
		DO_OPTIONS=1
	fi
	if [ "$1" == "--interface" ]
	then
		DO_INTERFACE=1
	fi
	shift
done

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
	  if [ $DO_OPTIONS -eq 1 ]
	  then
		if [ $DO_INTERFACE -eq 1 ]
		then
			sed "s/Template/$NAME/g" template.$e | sed "s/template/$LOWER/g" | sed "s/TEMPLATE/$UPPER/g" | sed "s/\/\/ OPT_CODE//g" \
				| sed "s/\/\/ INT_CODE//g" | sed "/^.*\/\/ NO_INT_CODE/d" > ../$LOWER/$LOWER.$e
		else
			sed "s/Template/$NAME/g" template.$e | sed "s/template/$LOWER/g" | sed "s/TEMPLATE/$UPPER/g" | sed "s/\/\/ OPT_CODE//g" \
				| sed "/^.*\/\/ INT_CODE/d" | sed "s/\/\/ NO_INT_CODE//g" > ../$LOWER/$LOWER.$e
		fi
	  else
		if [ $DO_INTERFACE -eq 1 ]
		then
			sed "s/Template/$NAME/g" template.$e | sed "s/template/$LOWER/g" | sed "s/TEMPLATE/$UPPER/g" | sed "/^.*\/\/ OPT_CODE/d" \
				| sed "s/\/\/ INT_CODE//g" | sed "/^.*\/\/ NO_INT_CODE/d" > ../$LOWER/$LOWER.$e
		else
			sed "s/Template/$NAME/g" template.$e | sed "s/template/$LOWER/g" | sed "s/TEMPLATE/$UPPER/g" | sed "/^.*\/\/ OPT_CODE/d" \
				| sed "/^.*\/\/ INT_CODE/d" | sed "s/\/\/ NO_INT_CODE//g" > ../$LOWER/$LOWER.$e
		fi
	  fi
	fi
done

if [ $DO_OPTIONS -eq 1 ]
then
	EXTS="h cpp ui"
	for e in $EXTS
	do
	  sed "s/TemplateOptions/${NAME}Options/g" templateoptions.$e | sed "s/templateoptions/${LOWER}options/g" | sed "s/TEMPLATEOPTIONS/${UPPER}OPTIONS/g" > ../$LOWER/${LOWER}options.$e
	done  
	echo "HEADERS += ../../include/options_i.h ${LOWER}options.h" >> ../$LOWER/$LOWER.pro
	echo "SOURCES += ${LOWER}options.cpp" >> ../$LOWER/$LOWER.pro
	echo "FORMS += ${LOWER}options.ui" >> ../$LOWER/$LOWER.pro
fi

if [ $DO_INTERFACE -eq 1 ]
then
	sed "s/Template/$NAME/g" template_i.temp_h | sed "s/template/$LOWER/g" | sed "s/TEMPLATE/$UPPER/g" > ../../include/${LOWER}_i.h
	echo "HEADERS += ../../include/${LOWER}_i.h" >> ../$LOWER/$LOWER.pro
fi

# replace the vc project's guid with a new one
if [ -e "../$LOWER/$LOWER.vcproj" ]
then
  GUID=`uuidgen`
  UP_GUID="`echo $GUID | tr \"[:lower:]\" \"[:upper:]\"`"
  sed "/^\tProjectGUID=/ s/\".*\"/\"{$UP_GUID}\"/" ../$LOWER/$LOWER.vcproj > ../$LOWER/$LOWER.vcproj_test
  mv ../$LOWER/$LOWER.vcproj_test ../$LOWER/$LOWER.vcproj
fi

# add the new directory to the qt project file
#if [ "`grep -l \"SUBDIRS+=$LOWER\" ../plugins.pro`" = "" ]
#then
#	echo "SUBDIRS+=$LOWER" >> ../plugins.pro
#fi
