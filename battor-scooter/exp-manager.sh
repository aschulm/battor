#!/bin/bash
## Note:   Script to automate mobile battor tests 
## Author: Matteo Varvello 
## Date:   07/21/2020

#helper to  load utilities files
load_file(){
    if [ -f $1 ]
    then
        source $1
    else
        echo "Utility file $1 is missing"
        exit -1
    fi
}

# import utilities files needed
curr_dir=`pwd`
f=$curr_dir"/common.sh"
DEBUG=1
load_file $f

# setup phone prior to an experiment 
phone_setup(){
    # disable notification 
    myprint "[INFO] Disabling notifications for the experiment"
    adb -s $device_id shell settings put global heads_up_notifications_enabled 0

    # set desired brightness
    myprint "[INFO] Setting low screen brightness [TBD]"
    screen_brightness=50
    adb -s $device_id shell settings put system screen_brightness $screen_brightness

    #get and log some useful info
    dev_model=`adb -s $device_id shell getprop ro.product.model | sed s/" "//g` # S9: SM-G960U
    android_vrs=`adb -s $device_id shell getprop ro.build.version.release`
    myprint "[INFO] DEV-MODEL: $dev_model ANDROID-VRS: $android_vrs"    

    # remove screen timeout 
    myprint "[INFO] Remove screen timeout [TBD]"
    max_screen_timeout="2147483647"
    adb -s $device_id shell settings put system screen_off_timeout $max_screen_timeout

    # close all pending applications
    myprint "[INFO] Close all pending applications"
    adb -s $device_id shell "input keyevent KEYCODE_HOME"
    adb -s $device_id shell "input keyevent KEYCODE_APP_SWITCH"
    sleep 2
    myprint "FIXME -- this depend on the device"
}

# script usage
usage(){
    echo "================================================================================"
    echo "USAGE: $0 --opt, --sense, --battor, --id, --adb"
    echo "================================================================================"
    echo "--opt      Exp. Option: [start,stop]"
    echo "--sense    Start sensing application"
    echo "--battor   Start battor power measurement" 
    echo "--id       Test identifier to use" 
    echo "--adb      ADB device identifier"
    echo "================================================================================"
    exit -1
}

# verify adb is running and device is found   
verify_adb(){
    myprint "Testing that sensing device is connected via USB"
    adb devices > /dev/null
    adb devices | grep $adb_id > /dev/null 
    if [ $? -ne 0 ]
    then 
        myprint "[ERROR] Sensing was requested but device (adb-id:$adb_id) was not found."
        exit -1 
    fi
}

# verify that battor is installed
verify_battor(){    
    hash battor > /dev/null 2>&1 
    if [ $? -ne 0 ]
    then 
        myprint "[ERROR] battor is not installed. Please see: https://github.com/svarvel/battor"
        exit -1
    fi 
}
  
# prepare phone and battor for an experiment
prepare_experiment(){        
    # sensing 
    if [ $use_sense == "true" ]
    then
        # verify adb connectivity
        verify_adb

        # prepare the device for minimum noise 
        phone_setup
        sleep 3

        # launch app on the device 
        myprint "Launch the sensing app -- FIXME" 
        #adb -s $adb_id shell am start -n $package/$activity -a android.intent.action.VIEW
    fi 

    # battery measurements
    if [ $use_battor == "true" ]
    then
        # verify battor is installed 
        verify_battor
    
        # starts buffering
        battor -b

        # verify buffering 
        val=`battor -o 2>/dev/null`
        if [ $val -eq 0 ]
        then 
            myprint "[ERROR] Something went wrong when starting battor"
        else 
            myprint "[INFO] Battor was successfully started"
        fi 
    fi 
}

# end an experiment and collect data 
end_experiment(){        
    if [ $use_sense == "true" ]
    then
        # verify adb connectivity
        verify_adb

        # stop the app 
        myprint "[INFO] Closing sensing app."
        adb -s $device_id shell "am force-stop $package"

        # pull the data 
        myprint "[INFO] Pulling data -- FIXME"
        local_sense_log=$res_folder"/sensing.csv"        
        remote_sense_log=""  #FIXME 
        #adb -s $device_id pull $remote_sense_log $local_sense_log
    fi 
    if [ $use_battor == "true" ]
    then
        # verify battor is installed 
        verify_battor

        # stop buffering and pull data 
        battery_log=$res_folder"/battery.csv"
        battor -d > $battery_log

        # verify buffering stopped 
        val=`battor -o 2>/dev/null`
        if [ $val -ne 0 ]
        then 
            myprint "[ERROR] Something went wrong when stopping battor"
        else 
            myprint "[INFO] Battor was successfully stopped" 
        fi 
    fi 

    # manage database 
    myprint "TODO -- database"
}

# parameters 
adb_id="1234"               # adb identifier
use_sense="false"           # start app sensing collection 
use_battor="false"          # start power measurements collection 
test_id=`date +%s`          # test identifier 
res_folder="./results"      # base folder for results
opt="None"                  # option [before,after]
package=""                  # sense app package (FIXME)
activity=""                 # sense app activity (FIXME)
        
# read input parameters
while [ "$#" -gt 0 ]
do
    case "$1" in       
        --opt)
            shift;
            opt="$1"
            shift; 
            ;;
        
        --adb)
            shift;
            adb_id="$1"
            shift; 
            ;;
        --sense)
            shift;
            use_sense="true"
            ;;
        --battor)
            shift;
            use_battor="true"
            ;;
        --id)
            shift;
            test_id="$1"
            shift; 
            ;;
        -h | --help)
            usage
            ;;
        -*)
            echo "ERROR: Unknown option $1"
            usage
            ;;
    esac
done

# check input 
if [ $opt == "None" ]
then 
    myprint "[ERROR] --opt is a required parameter"
    usage
fi 
if [ $use_sense == "false" -a $use_battor == "false" ]
then 
    myprint "[ERROR] One option between --sense or --battor is required"
    usage
fi 

# folder organization
res_folder="${res_folder}/${test_id}"
mkdir -p $res_folder

# switch among supported options
myprint "[INFO] Option: $opt Test-ID: $test_id"
case $opt in
    "start")
        prepare_experiment
        ;;
    
    "stop")
        end_experiment
        ;;
    
    "*")
        myprint "[WARNING] Option $opt not supported yet"
        exit -1 
        ;;
esac