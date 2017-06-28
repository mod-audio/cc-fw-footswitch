#ifndef CONFIG_H
#define CONFIG_H

// define firmware version
#define CC_FIRMWARE_MAJOR   0
#define CC_FIRMWARE_MINOR   2
#define CC_FIRMWARE_MICRO   0

// maximum number of devices that can be created
#define CC_MAX_DEVICES          1
// maximum number of actuators that can be created per device
#define CC_MAX_ACTUATORS        4
// maximum number of assignments that can be created per actuator
#define CC_MAX_ASSIGNMENTS      1
// maximum number of options items that can be created per device
#define CC_MAX_OPTIONS_ITEMS    64

// define the size of the queue used to store the updates before send them
#define CC_UPDATES_FIFO_SIZE    10

#endif
