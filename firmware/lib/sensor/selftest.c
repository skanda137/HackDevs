#include "selftest.h"

#include <stdio.h>

#include "sensor.h"

static selftest_emit_fn emit_line;
static int n_pass;
static int n_fail;

static void report(const char *name, bool ok, const char *detail) {
    char line[128];
    if (ok) {
        snprintf(line, sizeof line, "TEST %s PASS", name);
        n_pass++;
    } else {
        snprintf(line, sizeof line, "TEST %s FAIL %s", name, detail);
        n_fail++;
    }
    emit_line(line);
}

static void feed(sensor_t *s, uint16_t value, int n) {
    for (int i = 0; i < n; i++) {
        sensor_update(s, value);
    }
}

static void test_idle_no_alarm(void) {
    sensor_t s;
    char d[96];
    sensor_init(&s);
    feed(&s, 100, 64);
    snprintf(d, sizeof d, "expected alarm=0 got alarm=%d avg=%u", s.alarm, sensor_average(&s));
    report("idle_no_alarm", !s.alarm, d);
}

static void test_short_burst_ignored(void) {
    sensor_t s;
    char d[96];
    sensor_init(&s);
    feed(&s, 100, 32);
    feed(&s, 4000, 4);
    snprintf(d, sizeof d, "expected alarm=0 got alarm=%d avg=%u", s.alarm, sensor_average(&s));
    report("short_burst_ignored", !s.alarm, d);
}

static void test_sustained_high_alarms(void) {
    sensor_t s;
    char d[96];
    sensor_init(&s);
    feed(&s, 100, 32);
    feed(&s, 4000, 32);
    snprintf(d, sizeof d, "expected alarm=1 got alarm=%d avg=%u", s.alarm, sensor_average(&s));
    report("sustained_high_alarms", s.alarm, d);
}

static void test_hysteresis_holds_then_clears(void) {
    sensor_t s;
    char d[96];
    sensor_init(&s);

    feed(&s, 4000, 32);
    if (!s.alarm) {
        snprintf(d, sizeof d, "step1 expected alarm=1 got alarm=0 avg=%u", sensor_average(&s));
        report("hysteresis_holds_then_clears", false, d);
        return;
    }
    feed(&s, 2900, 32);
    if (!s.alarm) {
        snprintf(d, sizeof d, "step2 expected alarm=1 got alarm=0 avg=%u", sensor_average(&s));
        report("hysteresis_holds_then_clears", false, d);
        return;
    }
    feed(&s, 2000, 32);
    snprintf(d, sizeof d, "step3 expected alarm=0 got alarm=%d avg=%u", s.alarm, sensor_average(&s));
    report("hysteresis_holds_then_clears", !s.alarm, d);
}

void selftest_run(selftest_emit_fn emit) {
    char line[48];
    emit_line = emit;
    n_pass = 0;
    n_fail = 0;

    emit_line("BEGIN");
    test_idle_no_alarm();
    test_short_burst_ignored();
    test_sustained_high_alarms();
    test_hysteresis_holds_then_clears();
    snprintf(line, sizeof line, "SUMMARY pass=%d fail=%d", n_pass, n_fail);
    emit_line(line);
    emit_line("DONE");
}
