#include "sensor.h"

#include <string.h>

void sensor_init(sensor_t *s) {
    memset(s, 0, sizeof *s);
}

uint16_t sensor_average(const sensor_t *s) {
    if (s->count == 0) {
        return 0;
    }
    return (uint16_t)(s->sum / s->count);
}

bool sensor_update(sensor_t *s, uint16_t raw) {
    if (s->count == SENSOR_WINDOW) {
        s->sum -= s->samples[s->head];
    } else {
        s->count++;
    }
    s->samples[s->head] = raw;
    s->sum += raw;
    s->head = (uint8_t)((s->head + 1) % SENSOR_WINDOW);

    if (s->count == SENSOR_WINDOW) {
        uint16_t avg = sensor_average(s);
        if (!s->alarm && avg >= SENSOR_ALARM_ON) {
            s->alarm = true;
        } else if (s->alarm && avg <= SENSOR_ALARM_OFF) {
            s->alarm = false;
        }
    }
    return s->alarm;
}
