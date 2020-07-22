#!/bin/bash
# Collection of commonly used functions
# Author: Matteo Varvello

# simple function for logging
myprint(){
    timestamp=`date +%s`
    if [ $DEBUG -gt 0 ]
    then
        if [ $# -eq  0 ]
        then
            echo -e "[ERROR][$timestamp]\tMissing string to log!!!"
        else
        	if [ $# -eq  1 ]
			then 
	            echo -e "[$0][$timestamp]\t" $1
			else 
	            echo -e "[$0][$timestamp][$2]\t" $1
			fi 
        fi
    fi
}

# clean a file
clean_file(){
    if [ -f $1 ]
    then
        rm $1
    fi
}

# kill pending processes
my_kill(){
    for pid in `ps aux | grep "$1" | grep -v "grep" | grep -v "master" | grep -v "experiment-manager.sh" | awk '{print $2}'`
    do
        kill -9 $pid
    done
}
