# switch-bench

A lab-bench room for Switch. A person reports a firmware symptom in Discord. A dev agent (Claude Code) finds the cause and fixes it on a branch. A bench agent (Codex) runs that branch on the machine with the board attached and posts the raw result. A human approves the merge.

Switch is the only link between the three: the agent that can touch the hardware, the agent that can change the code, and the person directing both from somewhere else.

## Topology

One laptop (the one with the board) runs Switch Console, the local Docker server and both agents. The other teammate joins the Discord room and directs, answers approval prompts and approves merges. A Console-run local server belongs to a single computer, so a second laptop cannot register its own agents on it. That is why both agents live on the bench laptop.

## What is in the repo

| Path | Purpose |
|---|---|
| `firmware/lib/sensor/` | Moving-average alarm (`sensor.c`) and the self-test (`selftest.c`) shared by host and board |
| `firmware/host/test_host.c` | Host build of the self-test, used when no board is attached |
| `firmware/src/main.cpp` | Arduino sketch that repeats the same self-test over serial |
| `bench.py` | Runs the self-test on a target and prints a JSON verdict |
| `bench-room.template.yaml` | Switch template: two agents and one room |

## Run the bench without Switch

```
python bench.py test --target host
```

Needs `gcc` and Python 3. Exit code 0 means all passed, 1 a test failed, 2 the harness failed.

The repository intentionally starts with one planted firmware bug for the demo. Its location is not documented here, so the agents must work from the bench output.

## Run on the ESP32 bench

```text
python bench.py test --target esp32 --port COM8
```

The verified bench uses an ESP32 Dev Module on COM8. PlatformIO and PySerial are required.

## Run it in Switch

1. Install Switch Console, choose "Run a server on this computer" (needs Docker), connect Discord, and link your Discord account.
2. Install and sign in to Claude Code and Codex on the bench laptop.
   Codex login: pin a model your login supports (we use `gpt-5.6-sol`). The default `gpt-6-sol` failed with a ChatGPT login.
3. Push this repo to GitHub. The bench laptop needs git credentials that can push branches.
4. In Console: Templates, Import template, paste `bench-room.template.yaml`, fill in the repo URL and bench arguments, create.
5. The kickoff makes the bench agent run main and post the baseline.

## Verification status

Confirmed by running:
- The host target builds with `-Wall -Wextra -Werror` and reports 2 pass, 2 fail on the planted bug.
- The ESP32 target compiles, flashes over COM8 and reports the same 2 pass, 2 fail baseline.
- The development agent's fix branch passes all 4 tests on both the host and the ESP32.
- The Discord room, agent handoff, fix-branch push and human approval gate work in a live Switch room.
- `bench.py` rejects truncated output, a missing summary, a summary that disagrees with the tests and malformed test lines.

The supplied room template parses successfully. On this Windows bench, the room and agents were configured manually after automated provisioning encountered a Windows execution-host limitation.

## Safety note

Both agents use `addressing: anyone`, so anyone in the room can instruct them, and the bench agent flashes real hardware. Keep the Discord channel private to the team.
