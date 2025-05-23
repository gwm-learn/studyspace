#!/bin/bash

WORK_DIR=
RUN_CMD=0
FORCE=
SAVE_PATH=
PASSWORD="123"

usage_info(){
    echo "----------------------------- info -------------------------------"
    echo "|Usage:clean_code.sh [option]                                    |"
    echo "|option:-c or --changed to clean changed file in svn repository  |"
    echo "|option:-t or --temp to delete temp file not in svn repository   |"
    echo "|option:-a or --all to clean up the code thoroughly              |"
    echo "|option:-f or --force to force clean up the code                 |"
    echo "|option:-p or --path to set path to clean up                     |"
    echo "|option:-h or --help to see this shell script info               |"
    echo "------------------------ author: gaoweiming ---------------------"	
}

scan_file(){
    echo "flie scaning [$1][`pwd`]..."
    svn status $1 | tee $1/file.log
}

clean_temp_file(){
    echo "clean temp file in [$1][`pwd`]..."
    if [ ! -e $1"/file.log" ]; then
        scan_file $1
    fi
    temp_file=`cat $1/file.log | grep  ^? | grep -Ev "file.log" | awk '{printf $2 "  "}'`
    if [ -n "$temp_file" ]; then
        echo "clean temp file start"
        if [ -n "$FORCE" ]; then
            echo "$PASSWORD" | $FORCE rm -rf ${temp_file}
        else
            rm -rf ${temp_file}
        fi
        echo "clean temp file end"
    else
        echo "temp file is null"
    fi
}

clean_changed_file(){
    echo "clean changed file in [$1][`pwd`]..."
    if [ ! -e $1"/file.log" ]; then
        scan_file $1
    fi
    changed_file=`cat $1/file.log | grep -v ^? | awk '{printf $2 "  "}'`
    if [ -n "$changed_file" ]; then
        echo "clean changed file start"
        if [ -n "$FORCE" ]; then
            echo "$PASSWORD" | $FORCE svn revert ${changed_file}  --depth infinity
        else
            svn revert ${changed_file}  --depth infinity
        fi
        echo "clean changed file end"
    else
        echo "changed file is null"
    fi
}

clean_file(){
    clean_temp_file $1
    clean_changed_file $1
}

swap_seconds(){
    SEC=$[ $2 - $1 ]
    if [ $SEC -lt 60 ]; then
        echo "Total:${SEC}s"
    elif [ $SEC -ge 60 ] && [ $SEC -lt 3600 ];then
        echo "Total:$(( SEC / 60 ))m $(( SEC % 60 ))s"
    elif [ $SEC -ge 3600 ]; then
        echo "Total:$(( SEC / 3600 ))h $(( (SEC % 3600) / 60 ))m $(( (SEC % 3600) % 60 ))s"
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
}

echo "start working..."
startTime=`date +%Y-%m-%d-%H:%M:%S`
startTime_s=`date +%s`

deal_input_param $*

if [ -e $WORK_DIR"/file.log" ]; then
    rm -rf $WORK_DIR/file.log
fi

if [ $RUN_CMD = 1 ]; then
    clean_changed_file $WORK_DIR
elif [ $RUN_CMD = 2 ]; then
    clean_temp_file $WORK_DIR
elif [ $RUN_CMD = 3 ]; then
    clean_file $WORK_DIR
else
    echo "..."
fi

endTime=`date +%Y-%m-%d-%H:%M:%S`
endTime_s=`date +%s`

echo "Begin:$startTime"
echo "End:  $endTime"

swap_seconds $startTime_s $endTime_s
