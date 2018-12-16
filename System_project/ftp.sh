#!/bin/bash
SERVER='210.115.229.91'
USER='joonb'
PASS='dblab2316'

if [ $1 == "1" ];then
echo "File Upload"
ftp -v -n <<- SCRIPT1
open $SERVER
user $USER $PASS
prompt
put $2
bye
SCRIPT1
fi

if [ $1 == "2" ];then
echo "File Download"
ftp -v -n <<- SCRIPT1
open $SERVER
user $USER $PASS
prompt
get $2
bye
SCRIPT1
fi
