#!/bin/bash

ROOT_DIR=$(pwd)

PROCESSES_DIR=$ROOT_DIR/processes

CONFIG_DIR=$ROOT_DIR/config

LOG_DIR=$ROOT_DIR/log

TOUCH_DIR=$LOG_DIR/touchfile

PROCESS_EXIST=1
PROCESS_NOT_EXIST=0


mkdir -p $TOUCH_DIR
ulimit -c unlimited

function checkProcessExist()
{
    FILES_LIST=$(ls -l $TOUCH_DIR |awk '{print $9}')
    echo "$FILES_LIST" |grep -q "$1"
    if [ $? -eq 0 ]
    then
        echo $PROCESS_EXIST
    else
        echo $PROCESS_NOT_EXIST
    fi
}

function isProcessStart()
{
    sleep 1
    FILES_LIST=$(ls -l $TOUCH_DIR |awk '{print $9}')
    echo "$FILES_LIST" |grep -q "$1"
    if [ $? -eq 0 ]
    then
        echo "$1 start sucess!"
    else
        echo "$1 start failed..."
    fi
}


exist_or_not=$(checkProcessExist game-3)
if [ $exist_or_not = $PROCESS_NOT_EXIST ]
then
    echo "$PROCESSES_DIR/game/game_exec 3 $CONFIG_DIR $LOG_DIR || exit 1 &"
    $PROCESSES_DIR/game/game_exec 3 $CONFIG_DIR $LOG_DIR || exit 1 &
    isProcessStart game-3
else
    echo "process game-3 exist!"
fi

ps -x |grep _exec

