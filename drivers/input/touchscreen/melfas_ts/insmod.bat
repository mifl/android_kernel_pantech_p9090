

adb remount
adb push Z:\BUILD\MSM8960ICS-3241\LINUX\android\kernel\drivers\input\touchscreen\melfas_ts\melfas_ts.ko /system/lib/modules/
adb shell rmmod /system/lib/modules/melfas_ts.ko
adb shell insmod /system/lib/modules/melfas_ts.ko
adb shell chmod 777 /dev/melfas_ts  
adb shell chmod 777 /dev/touch_fops 

