#ifndef CONFIG_H
#define CONFIG_H


#define FOOTSWITCHES_COUNT  4
// define firmware version
#define CC_FIRMWARE_MAJOR   0
#define CC_FIRMWARE_MINOR   4
#define CC_FIRMWARE_MICRO   0

// maximum number of devices that can be created
#define CC_MAX_DEVICES          1
// maximum number of actuators that can be created per device
#define CC_MAX_ACTUATORS        4
// maximum number of assignments that can be created per actuator
#define CC_MAX_ASSIGNMENTS      1
// maximum number of options items that can be created per device
#define CC_MAX_OPTIONS_ITEMS    64

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

// define the size of the queue used to store the updates before send them
#define CC_UPDATES_FIFO_SIZE    10

#endif
