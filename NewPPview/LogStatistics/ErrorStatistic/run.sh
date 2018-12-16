#!/bin/bash 
#*-* coding:utf8 *-*

dir=`pwd`
day=1
env=$1
env_log=$env"_log"
#if  [ $# -eq 1 ]
#then
#	day=$1
#fi

date=`date +%Y%m%d -d "1 days ago"`

python $dir/../../Common/checkDate.py -d $day
res=$?
if [ $res -eq 0 ]
then
	echo "date -> $day"
	date=$day
else
	echo "$day day ago"
	date=`date +%Y%m%d -d "$day days ago"`
fi
report="./output/$env/$date.html"
#exit 0

#environment="test"

#log_dir="/search/mj_stats/online/PPViewChat/*/$date/"
log_dir="/search/log/LOG/$env_log/$date/pp*"

find $log_dir  -name "$date*.err*" | xargs -n1 cat |python ./bin/ppview_monitor_success.py -d $date -e $env 

python $dir/bin/ppview_monitor_error.py -d $date -e $env

python $dir/bin/ppview_report.py -d $date -e $env > $report

#发邮件

mail_file="../../conf/mail.list"
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
#addr="shiyuyan@mioji.com"
$mioji_mail -m "$addr" -b "PPView $env 统计 $date" -f "$report" -h 123

