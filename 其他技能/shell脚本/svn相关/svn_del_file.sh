#!/bin/bash

WORK_DIR=`pwd`

echo "start working..."

echo "new file scaning..."
del_file=`svn status $WORK_DIR | grep ^! |grep -Ev "$WORK_DIR/del_file.log|$WORK_DIR/changed_file.log" | awk '{printf $2 "  "}' | tee $WORK_DIR/del_file.log`
if [ -n "$del_file" ]; then
	echo "del file in svn"
	svn del ${del_file}
else
	echo "there is no file to del"
fi
