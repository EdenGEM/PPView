#!/bin/bash

source ./start.sh --source-only

while true
do
{
CRONOLOG_PID=`ps aux|grep -v grep|grep $CRONOLOG_BIN|grep -c "$LOG_PATH"`
PPVIEWCHAT_PID=`ps aux|grep -v grep|grep PPViewChat|grep -v $CRONOLOG_BIN|grep -c $RUN_PATH`

if [ $CRONOLOG_PID -eq 1 -a $PPVIEWCHAT_PID -eq 1 ]; then
	echo "exist"
	sleep 1
else
	echo "cronolog and PPViewChat pid is error, will reboot service"
	sh stop.sh
	sh start.sh
fi
sleep 3
}
done
