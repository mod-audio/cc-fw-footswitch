#ifndef CC_UTILS_H
#define CC_UTILS_H


/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdint.h>


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/


/*
****************************************************************************************************
*       CONFIGURATION
****************************************************************************************************
*/


/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/

typedef struct string_t {
    uint8_t size;
    char *text;
} string_t;

typedef struct str16_t {
    uint8_t size;
    char text[17];
} str16_t;

typedef struct version_t {
    uint8_t major, minor, micro;
} version_t;


/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

/*
 http://stackoverflow.com/a/15171925/1283578
 8-bit CRC with polynomial x^8+x^6+x^3+x^2+1, 0x14D.
 Chosen based on Koopman, et al. (0xA6 in his notation = 0x14D >> 1):
 http://www.ece.cmu.edu/~koopman/roses/dsn04/koopman04_crc_poly_embedded.pdf
*/
uint8_t crc8(const uint8_t *data, uint32_t len);

string_t *string_create(const char *str);
int string_serialize(const string_t *str, uint8_t *buffer);
string_t *string_deserialize(const uint8_t *data, uint32_t *written);
void string_destroy(string_t *str);

int str16_create(const char *str, str16_t *dest);
int str16_serialize(const str16_t *str, uint8_t *buffer);
int str16_deserialize(const uint8_t *data, str16_t *str);

int bytes_to_float(const uint8_t *array, float *pvar);


/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/


#endif
