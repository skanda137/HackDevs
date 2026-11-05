#ifndef SELFTEST_H
#define SELFTEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Line protocol, one line per call to emit():
 *   BEGIN
 *   TEST <name> PASS
 *   TEST <name> FAIL <detail>
 *   SUMMARY pass=<n> fail=<n>
 *   DONE
 * The host binary and the board firmware run this same code. */
typedef void (*selftest_emit_fn)(const char *line);

void selftest_run(selftest_emit_fn emit);

#ifdef __cplusplus
}
#endif

#endif
