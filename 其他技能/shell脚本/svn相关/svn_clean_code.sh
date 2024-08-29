#!/bin/bash

WORK_DIR=
RUN_CMD=0
FORCE=
SAVE_PATH=
ROOT_DIR=`pwd`

usage_info(){
	echo "----------------------------- info -------------------------------"
	echo "|Usage:clean_code.sh [option]                                    |"
	echo "|option:-c or --changed to revert changed file in svn repository |"
	echo "|option:-t or --temp to delete temp file not in svn repository   |"
	echo "|option:-a or --all to clean up the code thoroughly              |"
	echo "|option:-f or --force to force clean up the code                 |"
	echo "|option:-p or --path to set path to clean up                     |"
	echo "|option:-h or --help to see this shell script info               |"
	echo "------------------------ author: gaoweiming ---------------------"	
}

remove_temp_file(){
	echo "temp flie scaning..."
	temp_file=`svn status $WORK_DIR | grep  ^? | grep -Ev "temp_file.log|changed_file.log" | awk '{printf $2 "  "}' | tee $ROOT_DIR/temp_file.log`
	if [ -n "$temp_file" ]; then
		echo "rm temp file start"
		$FORCE rm -rf ${temp_file}
		echo "rm temp file end"
	else
		echo "temp_file is null"
	fi
}

revert_changed_file(){
	echo "changed file scaning..."
	changed_file=`svn status $WORK_DIR | grep -v ^? | awk '{printf $2 "  "}' | tee $ROOT_DIR/changed_file.log`
	if [ -n "$changed_file" ]; then
		echo "revert changed file start"
		$FORCE svn revert ${changed_file}  --depth infinity
		echo "revert changed file end"
	else
		echo "changed_file is null"
	fi
}

if [ $# -lt 2 ]; then
	usage_info
	exit 0
fi

deal_input_param(){
        while [ $# -ne 0 ]
        do
        	echo "param:$*"
		case "$1" in
	-c|--changed)
			RUN_CMD=1;
			SAVE_PATH=
			shift
			;;
	-t|--temp)
			RUN_CMD=2;
			SAVE_PATH=
			shift
			;;
	-a|--all)
			RUN_CMD=3;
			SAVE_PATH=
			shift
			;;
	-h|--help)
			usage_info;
			SAVE_PATH=
			shift
			;;
	-f|--force)
			FORCE="sudo -S ";
			SAVE_PATH=
			shift
			;;
	-p|--path)
			SAVE_PATH="true"
			shift
			;;
	--)
			shift
			break
			;;
	*)
			if [ -z $SAVE_PATH ]; then
				shift
			else
				WORK_DIR=$WORK_DIR" $1"
				shift
			fi
			;;
		esac
	done

	if [ -z $WORK_DIR ]; then
		WORK_DIR=`pwd`
	fi
	
	echo "param config end work dir [$WORK_DIR]"
}

echo "start working..."

deal_input_param $*

if [ $RUN_CMD = 1 ]; then
	revert_changed_file;
elif [ $RUN_CMD = 2 ]; then
	remove_temp_file;
elif [ $RUN_CMD = 3 ]; then
	remove_temp_file;
	revert_changed_file;
else
	echo "..."
fi
