#!/bin/bash

ROOT_DIR=$(pwd)

PROCESSES_DIR=$ROOT_DIR/processes

CONFIG_DIR=$ROOT_DIR/config

LOG_DIR=$ROOT_DIR/log

TOUCH_DIR=$LOG_DIR/touchfile

function checkProcessKilled()
{
    PROCESS_LIST=$(ps -ef |grep "$1" |grep _exec |awk '{print $8}')

    echo "$PROCESS_LIST" |grep -q "$1"
    if [ $? -eq 0 ]
    then
        echo "stop $1 ......"
        sleep 1
        checkProcessKilled $1
    else
        rm -f $TOUCH_DIR/$1*
        echo "$1 stop sucess!"
    fi
}

killall game_exec
checkProcessKilled game
