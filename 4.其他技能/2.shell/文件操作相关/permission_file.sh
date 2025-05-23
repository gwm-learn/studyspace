#!/bin/bash

CMD_FILE=
OPTION_PARAM=
OPTION_NUM=0
NUM_ONE=1
OPTION_TYPE=
USER="gwm"
PASSWORD="123"

usage_info(){
	echo "----------------------------- info ---------------------------------------------------------"
	echo "|Usage:permission_file.sh [option]                                                         |"
	echo "|option:-f [filename] or --file [filename] to specifies the permission file                |"
	echo "|option:-p [pathname] or --path [pathname] to specifies the permission file in this path   |"
	echo "|option:-h or --help to see this shell script info                                         |"
	echo "------------------------ author: gaoweiming ------------------------------------------------"
}

deal_input_param(){
	while [ $# -ne 0 ]
	do
	echo "param:$*"
			case "$1" in
		-f|--file)
			OPTION_NUM=`expr $OPTION_NUM + $NUM_ONE`
			OPTION_TYPE="file"
			if [ -z $2 ]; then
				break
			else
				shift
				continue
			fi
			;;
		-p|--path)
			OPTION_NUM=`expr $OPTION_NUM + $NUM_ONE`
			OPTION_TYPE="path"
			if [ -z $2 ]; then
				break
			else
				shift
				continue
			fi
		;;
		-h|--help)
						usage_info
			if [ -z $2 ]; then
				break
			else
				shift
				continue
			fi
		;;
		*)
			OPTION_PARAM=$OPTION_PARAM" $1"
			if [ -z $2 ]; then
				break
			else
				shift
				continue
			fi
		;;

	esac
		shift
	done
}

if [ $# -eq 1 ]; then
	usage_info
	exit 0
fi

deal_input_param $*

if [ $OPTION_NUM != 1 ]; then
	echo "OPTION_NUM:$OPTION_NUM"
	usage_info
	exit 1
fi

if [ $OPTION_TYPE = "file" ]; then
	echo $PASSWORD | sudo -S chown $USER $OPTION_PARAM
	echo $PASSWORD | sudo -S chgrp $USER $OPTION_PARAM
elif [ $OPTION_TYPE = "path" ]; then
	echo "chown $USER -R $OPTION_PARAM"
	echo $PASSWORD | sudo -S chown $USER -R $OPTION_PARAM
	echo "chgrp $USER -R $OPTION_PARAM"
	echo $PASSWORD | sudo -S chgrp $USER -R $OPTION_PARAM
else
	usage_info
fi
