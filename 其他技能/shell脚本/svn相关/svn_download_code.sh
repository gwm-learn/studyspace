#!/bin/bash

SVN_CODE_REPO_URL=
SVN_CODE_REPO_LOCAL_NAME=
SVN_CODE_REPO_USERNAME=
SVN_CODE_REPO_PASSWORD=
SVN_DOWNLOAD_CODE_PARA=

usage_info(){
	echo "--------------------------------------- info --------------------------------------------"
	echo "|Usage:SVN_DOWNLOAD_CODE.sh [option]                                                    |"
	echo "|option:-r or --repourl url of the svn repository                                       |"
	echo "|option:-l or --localname name of the local svn repository                              |"
	echo "|option:-u or --username username of the svn repository                                 |"
	echo "|option:-p or --password password of the svn repository                                 |"
	echo "|option:-h or --help to see this shell script info                                      |"
	echo "|example:SVN_DOWNLOAD_CODE.sh -r http://xx.xx.xx -l svn_repo_01 -u username -p password |"
	echo "--------------------------------- author: gaoweiming ------------------------------------"
}

deal_param(){
	if [ -n "$SVN_CODE_REPO_URL" ]; then
		SVN_DOWNLOAD_CODE_PARA=$SVN_CODE_REPO_URL
	else
		echo "error: no repourl"
		usage_info
		exit 1
	fi

	if [ -n "$SVN_CODE_REPO_LOCAL_NAME" ]; then
		SVN_DOWNLOAD_CODE_PARA=$SVN_DOWNLOAD_CODE_PARA" "$SVN_CODE_REPO_LOCAL_NAME
	else
		echo "error: no localname"
		usage_info
		exit 1
	fi

	if [ -n "$SVN_CODE_REPO_USERNAME" ]; then
		SVN_DOWNLOAD_CODE_PARA=$SVN_DOWNLOAD_CODE_PARA" --username="$SVN_CODE_REPO_USERNAME
	else
		echo "error: no username"
		usage_info
		exit 1
	fi

	if [ -n "$SVN_CODE_REPO_PASSWORD" ]; then
		SVN_DOWNLOAD_CODE_PARA=$SVN_DOWNLOAD_CODE_PARA" --password="$SVN_CODE_REPO_PASSWORD
	else
		echo "error: no password"
		usage_info
		exit 1
	fi
}

download_code(){
	if [ -z "$SVN_DOWNLOAD_CODE_PARA" ]; then
		echo "error: param is null"
		usage_info
		exit 1
	fi
	echo "========== svn co $SVN_DOWNLOAD_CODE_PARA =========="
	svn co $SVN_DOWNLOAD_CODE_PARA

	while(($? != 0))
	do
		echo "========== svn co fail =========="
		echo "========== svn cleanup  ========== "
		cd $SVN_CODE_REPO_LOCAL_NAME
		svn cleanup
		cd -

		echo "========== svn co retry ========== "
		echo "========== svn co $SVN_DOWNLOAD_CODE_PARA =========="
		svn co $SVN_DOWNLOAD_CODE_PARA
	done

	echo "========== svn co success =========="
}

if [ $# == 0 ]; then
	usage_info
	exit 1
fi

ARGS=`getopt -o r:l:u:p:h --long repourl:,localname:,username:,password:,help -n "$0" -- "$@"`
eval set -- "${ARGS}"

echo $ARGS

while true
do
	case "$1" in
		-r|--repourl)
			SVN_CODE_REPO_URL=$2
			shift 2
			;;
		-l|--localname)
			SVN_CODE_REPO_LOCAL_NAME=$2
			shift 2
			;;
		-u|--username)
			SVN_CODE_REPO_USERNAME=$2
			shift 2
			;;
		-p|--password)
			SVN_CODE_REPO_PASSWORD=$2
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

deal_param

download_code

