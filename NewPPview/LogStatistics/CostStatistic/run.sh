#!/bin/bash 
#*-* coding:utf8 *-*

date=`date -d "1 days ago" +%Y%m%d`
env=$1
env_log=$env"_log"

report="output/$env/$date.html"
log_dir="/search/log/LOG/$env_log/$date/ppview*"

find $log_dir  -name "$date*.err*" | xargs -n1 cat |python ./bin/ppview_cost_monitor.py $date $env 1>$report 2>>err

if [ $? -ne 0 ]
then
	exit 1
fi

mail_file="../../conf/mail.list"
mioji_mail="/usr/local/sbin/mioji-mail"

#发邮件
addr=""

while read line
do
#	echo ${line:0:1}
	if [ ${line:0:1} != "#" ]
	then
		addr="${addr};${line}"
#		echo $addr
	fi
#	echo $addr
done < $mail_file 

addr=`echo ${addr} | sed 's/^;\+//g' | sed 's/;\+$//g' `
echo $addr
#addr="shiyuyan@mioji.com"

$mioji_mail -m "$addr" -b "PPView $env cost $date" -f "$report" -h 123

