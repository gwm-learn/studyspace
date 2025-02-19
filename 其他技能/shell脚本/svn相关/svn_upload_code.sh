#!/bin/bash

NUM_ONE=1
FILE_NUM=0
END_A=".swp"
END_B=".DS_Store"
MAX_DIR_NUM=5000
MAX_FILE_NUM=5000
SVN_FLAG=
SVN_CODE_UPLOAD_DIR=
SVN_CODE_UPLOAD_FILE=
SVN_CODE_UPLOAD_UNUPLOAD=

usage_info(){
    echo "----------------------------- info -----------------"
    echo "|svn_upload_code.sh [option] <dir>                  |"
    echo "|option:-d or --dir upload dir                      |"
    echo "|option:-f or --file upload file                    |"
    echo "|option:-u or --unupload unupload file              |"
    echo "|option:-h or --help                                |"
    echo "------------------------ author: gaoweiming --------"
}

upload_file(){
    echo "========== svn upload start =========="
    svn commit -m "[broadlink] upload"

    while(($? != 0))
    do
        echo "========== svn upload fail, retry it ========== "
        svn cleanup
        svn commit -m "[broadlink] upload"
    done

    echo "========== svn upload success =========="
}

file_in_path(){
    if [ -d $1 ]; then
        for file in `ls $1 `
        do
            if [ -d $1"/"$file ]; then
                file_in_path $1"/"$file
            else
                if [[ $file = *$END_A ]] || [[ $file = *$END_B ]] ; then
                    echo ""
                    echo ".swp .DS_Store do not upload"
                    echo ""
                else
                    file_in_path $1"/"$file
                fi
            fi
        done
    elif [ -f $1 ]; then
        SVN_FLAG=`svn st $1 --depth=empty 2>&1 | grep ^? | awk '{print $1}'`
        if [ -n "$SVN_FLAG" ]; then
            FILE_NUM=`expr $FILE_NUM + $NUM_ONE`
            svn add $1 --depth=empty -q 2> /dev/null
            if [ $? != 0 ]; then
                SVN_FLAG=`svn st $1 --depth=empty -q  2>&1 | grep "a peg revision is not allowed here" | awk '{print $3}'`
                if [ -n "$SVN_FLAG" ]; then
                    svn add $1"@" --depth=empty -q
                    echo -e "*\c"
                else
                    echo -e ".\c"
                fi
            else
                echo -e "*\c"
            fi
        else
            echo -e ".\c"
        fi
    fi
    if [ $FILE_NUM -gt $MAX_FILE_NUM ]; then
        echo ""
        echo "file_num large than $MAX_FILE_NUM, need run svn upload"
        echo "uploading file..."
        upload_file
        FILE_NUM=0
        echo ""
    fi
}

dir_in_path(){
    if [ -d $1 ]; then
        SVN_FLAG=`svn st $1 --depth=empty 2>&1 | grep ^? | awk '{print $1}'`
        if [ -n "$SVN_FLAG" ]; then
            FILE_NUM=`expr $FILE_NUM + $NUM_ONE`
            svn add $1 --depth=empty -q 2> /dev/null
            echo -e "*\c"
        else
            echo -e ".\c"
        fi
        for file in `ls $1 `
        do
            if [ -d $1"/"$file ]; then
                dir_in_path $1"/"$file
            fi
        done
    fi
    if [ $FILE_NUM -gt $MAX_DIR_NUM ]; then
        echo ""
        echo "dir_num large than $MAX_DIR_NUM, need run svn upload"
        echo "uploading dir..."
        upload_file
        FILE_NUM=0
        echo ""
    fi
}

unupload_file_in_path(){
    if [ -d $1 ]; then
        for file in `ls $1 `
        do
            if [ -d $1"/"$file ]; then
                cd $1"/"$file
                pwd
                svn add . --no-ignore --force
                upload_file
                cd -
            fi
        done
    fi
}

all_file_in_path(){
    echo "scan file..."
    while [ $# -ne 0 ]
    do
        file_in_path $1;
        shift
    done
    echo "scan finish"
}

all_dir_in_path(){
    echo "scan dir..."
    while [ $# -ne 0 ]
    do
        dir_in_path $1;
        shift
    done
    echo "scan finish"
}

all_unupload_file_in_path(){
    echo "scan unupload file..."
    while [ $# -ne 0 ]
    do
        unupload_file_in_path $1;
        shift
    done
    echo "scan finish"
}

if [ $# == 0 ]; then
    usage_info
    exit 1
fi

ARGS=`getopt -o d:f:u:h --long dir:,file:,unupload:,help -n "$0" -- "$@"`
eval set -- "${ARGS}"

echo $ARGS

while true
do
    case "$1" in
        -d|--dir)
            SVN_CODE_UPLOAD_DIR=$2
            shift 2
            ;;
        -f|--file)
            SVN_CODE_UPLOAD_FILE=$2
            shift 2
            ;;
        -u|--unupload)
            SVN_CODE_UPLOAD_UNUPLOAD=$2
            shift 2
            ;;
        -h|--help)
            usage_info;
            shift
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Internal error!"
            usage_info;
            exit 1
            ;;
    esac
done

if [ -n "$SVN_CODE_UPLOAD_DIR" ]; then
    echo "uploading dir..."
    all_dir_in_path $SVN_CODE_UPLOAD_DIR
    upload_file
fi

if [ -n "$SVN_CODE_UPLOAD_FILE" ]; then
    echo "uploading file..."
    all_file_in_path $SVN_CODE_UPLOAD_FILE
    upload_file
fi

if [ -n "$SVN_CODE_UPLOAD_UNUPLOAD" ]; then
    echo "uploading unupload file..."
    all_unupload_file_in_path $SVN_CODE_UPLOAD_UNUPLOAD
fi