## Context

Parodus maintains a `cpe_service_state` field in its global `ParodusCfg` struct. This state is updated at runtime via upstream messages (in `upstream.c`) and packed into metadata sent to the cloud. On restart, it resets to `"unknown"`, losing the last known state.

The project already uses `/tmp/` files for similar persistence (e.g., `/tmp/webpanotifyready` in `connection.c`).

## Goals / Non-Goals

- **Goal**: Persist `cpe-service-state` across parodus restarts using a `/tmp/` file
- **Goal**: Validate file contents before using them (reuse existing validation logic)
- **Non-Goal**: Persist across device reboots (`/tmp/` is cleared on reboot, which is acceptable — `"unknown"` is correct after a full reboot)

## Decisions

- **File path**: `/tmp/parodus_cpe_service_state` — follows existing `/tmp/` file conventions in the project
- **File format**: Plain text, single line containing the state string (e.g., `"fully-manageable"`)
- **Write location**: In `upstream.c` after `cpe_service_state` is updated in `ParodusCfg`, write the new value to the file
- **Read location**: In `setDefaultValuesToCfg()` in `config.c`, attempt to read the file before setting the default
- **Validation**: Reuse the same valid state check from `upstream.c` (`fully-manageable`, `operational`, `non-operational`); reject anything else and fall back to `"unknown"`

## Risks / Trade-offs

- **File I/O on every state change** → Acceptable; state changes are infrequent
- **Race condition if parodus crashes mid-write** → Mitigated by small file size (single short write); partial data fails validation and falls back to `"unknown"`

## Open Questions

None — the approach is straightforward and follows existing project patterns.
