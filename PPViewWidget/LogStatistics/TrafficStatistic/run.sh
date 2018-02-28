#!/bin/bash 
#*-* coding:utf8 *-*

dir=`pwd`
day=1

if  [ $# -eq 1 ]
then
	day=$1
fi

date=`date +%Y%m%d -d "1 days ago"`

python $dir/bin/checkDate.py -d $day
res=$?
if [ $res -eq 0 ]
then
	echo "date -> $day"
	date=$day
else
	echo "$day day ago"
	date=`date +%Y%m%d -d "$day days ago"`
fi
report="./output/$date.html"
#exit 0

environment="online"

log_dir="/search/log/LOG/chat_${environment}_log/$date/pp*"
#log_dir="/search/log/PPViewWidget/LogStatistics/TrafficStatistic"

find $log_dir  -name "$date*.err" | xargs -n1 cat |python ./bin/ppview_monitor_success.py -d $date 
#find $log_dir  -name "$date*.err" | xargs -n1 cat |python ./bin/st.py -d $date 

python $dir/bin/ppview_report.py -d $date > $report
#python $dir/bin/rt.py -d $date >$report
#发邮件

mail_file="./conf/mail.list"
mioji_mail="/usr/local/sbin/mioji-mail"

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
#addr="caoxiaolan@mioji.com"
$mioji_mail -m "$addr" -b "PPView 8003 统计 $date" -f "$report" -h 123

