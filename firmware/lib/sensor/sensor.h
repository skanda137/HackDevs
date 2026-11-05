#ifndef SENSOR_H
#define SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Moving-average alarm for a 12-bit ADC channel (0..4095 counts).
 * The alarm is evaluated only once the window is full.
 * It turns on when the average reaches SENSOR_ALARM_ON and turns off
 * when the average falls to SENSOR_ALARM_OFF or below (hysteresis). */
#define SENSOR_WINDOW 32
#define SENSOR_ALARM_ON 3000
#define SENSOR_ALARM_OFF 2800

typedef struct {
    uint16_t samples[SENSOR_WINDOW];
    uint8_t head;
    uint8_t count;
    uint16_t sum;
    bool alarm;
} sensor_t;

void sensor_init(sensor_t *s);
bool sensor_update(sensor_t *s, uint16_t raw);
uint16_t sensor_average(const sensor_t *s);

#ifdef __cplusplus
}
#endif

#endif
