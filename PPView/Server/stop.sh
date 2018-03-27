#!/bin/bash

source ./start.sh --source-only

CRONOLOG_PID=`ps aux|grep -v grep|grep $CRONOLOG_BIN|grep -c $LOG_PATH`
if [ $CRONOLOG_PID -ne 0 ]; then
	ps aux|grep $CRONOLOG_BIN|grep $LOG_PATH|grep -v grep|awk '{print $2}'|xargs kill -9
fi

PPVIEWCHAT_PID=`ps aux|grep -v grep|grep PPViewChat|grep -c $RUN_PATH`
if [ $PPVIEWCHAT_PID -ne 0 ]; then
	ps aux|grep PPViewChat|grep $RUN_PATH|grep -v grep|grep -v $CRONOLOG_BIN|awk '{print $2}'|xargs kill -9
fi

sleep 2
