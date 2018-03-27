#!/bin/bash

ulimit -c unlimited
ulimit -f unlimited

NEED_CRONOLOG="1"  ## 1 means use cronolog to split log
if [ $# -eq 1 ]; then
	NEED_CRONOLOG=$1
fi

## varliable
RUN_PATH=`pwd`
CRONOLOG_BIN="cronolog"
LOG_PATH=$RUN_PATH"/logs/"

main() {
	echo "RUN_PATH: "$RUN_PATH
	if [ "x$NEED_CRONOLOG" == "x1" ]; then
		command -V $CRONOLOG_BIN || exit -1
		command -V ./PPViewChat || exit -2
		export	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../release/lib/
		nohup $RUN_PATH/PPViewChat $RUN_PATH/../conf/server.cfg 2>&1 | $CRONOLOG_BIN $LOG_PATH"/%Y%m%d/%Y%m%d_%H.err.log" &
		sleep 1
		CRONOLOG_PID=`ps aux|grep -v grep|grep $CRONOLOG_BIN|grep -c $LOG_PATH`
		PPVIEWCHAT_PID=`ps aux|grep -v grep|grep PPViewChat|grep -v $CRONOLOG_BIN|grep -c $RUN_PATH`
		if [ $CRONOLOG_PID -eq 1 -a $PPVIEWCHAT_PID -eq 1 ]; then
			echo "cronolog and PPViewChat start OK"
		else 
			echo "cronolog and PPViewChat start Error, will exit"
			sh stop.sh
		fi
	else
		export	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/mioji/lib/
		nohup $RUN_PATH/PPViewChat $RUN_PATH/../conf/server.cfg >& err &
	fi
}

if [ "${1}" != "--source-only" ]; then
	main "${@}"
fi


