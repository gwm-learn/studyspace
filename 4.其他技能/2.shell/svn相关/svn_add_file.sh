#!/bin/bash

WORK_DIR=`pwd`

echo "start working..."

echo "new file scaning..."
new_file=`svn status $WORK_DIR | grep ^? |grep -Ev "$WORK_DIR/new_file.log|$WORK_DIR/changed_file.log" | awk '{printf $2 "  "}' | tee $WORK_DIR/new_file.log`
if [ -n "$new_file" ]; then
	echo "add new file to svn"
	svn add ${new_file}
else
	echo "there is no new file"
fi
