#ifndef CONFIG_H
#define CONFIG_H

// define firmware version
#define CC_FIRMWARE_MAJOR   0
#define CC_FIRMWARE_MINOR   5
#define CC_FIRMWARE_MICRO   0

//amount of footswitches
#define FOOTSWITCHES_COUNT  4

// maximum number of devices that can be created
#define CC_MAX_DEVICES          1
// maximum number of actuators that can be created per device
#define CC_MAX_ACTUATORS        4
// maximum number of assignments that can be created per actuator
#define CC_MAX_ASSIGNMENTS      1
// maximum number of options items that can be created per device
#define CC_MAX_OPTIONS_ITEMS    15
//amount of actuator groups
#define CC_MAX_ACTUATORGROUPS  2

//// Tap Tempo
// defines the time that the led will stay turned on (in milliseconds)
#define TAP_TEMPO_TIME_ON         100
// defines the default timeout value (in milliseconds)
#define TAP_TEMPO_DEFAULT_TIMEOUT 3000
// defines the difference in time the taps can have to be registered to the same sequence (in milliseconds)
#define TAP_TEMPO_TAP_HYSTERESIS  100
// defines the time (in milliseconds) that the tap can be over the maximum value to be registered
#define TAP_TEMPO_MAXVAL_OVERFLOW 50

//amount of colours available for LED cycling
#define LED_COLOURS_AMOUNT		7

//frame size needs to be an uneven number, otherwise the master will round it
#define CC_OPTIONS_LIST_FRAME_SIZE  3

// define the size of the queue used to store the updates before send them
#define CC_UPDATES_FIFO_SIZE    10


//ID's for saving settings
#define EEPROM_VERSION_ID	0
#define PAGE_SETTING_ID 	1
#define CC_CHAIN_ID_ID  	2

#define DEFAULT_PAGES		3
#define DEFAULT_DEVICE_ID	0

#define MAX_PAGES			3

#define EEPROM_ID			1


#endif
