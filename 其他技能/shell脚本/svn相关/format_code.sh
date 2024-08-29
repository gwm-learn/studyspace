#!/bin/bash

CMD_FILE=
OPTION_PARAM=
OPTION_NUM=0
NUM_ONE=1
OPTION_TYPE=
FILE_NUM=0
END_C=".c"
END_CPP=".cpp"
END_H=".h"
MAX_NUM=20

usage_info(){
        echo "----------------------------- info -----------------------------------------------------"
        echo "|Usage:format_code.sh [option]                                                         |"
        echo "|option:-f [filename] or --file [filename] to specifies the format file                |"
        echo "|option:-p [pathname] or --path [pathname] to specifies the format file in this path   |"
        echo "|option:-h or --help to see this shell script info                                     |"
        echo "------------------------ author: gaoweiming --------------------------------------------"
}

all_file_in_path(){
	while [ $# -ne 0 ]
       	do
		file_in_path $1;
        	shift
        done
}

file_in_path(){
	if [ -d $1 ]; then

		for file in `ls $1 `
		do
			if [ -d $1"/"$file ]; then
				file_in_path $1"/"$file
			else
				if [[ $file = *$END_C ]] || [[ $file = *$END_H ]] || [[ $file = *$END_CPP ]]; then 
					file_in_path $1"/"$file
				fi
			fi
		done
	elif [ -f $1 ]; then
		CMD_FILE=$CMD_FILE" $1"
		echo "PATH:"$1
		FILE_NUM=`expr $FILE_NUM + $NUM_ONE`
	fi
	if [ $FILE_NUM -gt $MAX_NUM ]; then
		echo "file_num large than $MAX_NUM, need run clang-format"
		clang-format -style=file:/home/gwm/SHELL/.clang-format -i $CMD_FILE
		CMD_FILE=""
		FILE_NUM=0
	fi
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
	clang-format -style=file:/home/gwm/SHELL/.clang-format -i $OPTION_PARAM
elif [ $OPTION_TYPE = "path" ]; then
	all_file_in_path $OPTION_PARAM
	clang-format -style=file:/home/gwm/SHELL/.clang-format -i $CMD_FILE
else
	usage_info
fi

