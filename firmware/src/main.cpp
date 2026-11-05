#include <Arduino.h>

#include "selftest.h"

static void emit_serial(const char *line) {
    Serial.println(line);
}

void setup() {
    Serial.begin(115200);
}

// The self-test repeats so the bench can attach at any moment after reset.
void loop() {
    delay(3000);
    selftest_run(emit_serial);
}
