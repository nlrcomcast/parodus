## Why

When parodus restarts or crashes, the `cpe-service-state` metadata resets to `"unknown"` even though a valid state was known before the restart. Persisting this value to a `/tmp/` file allows recovery of the last known state across restarts.

## What Changes

- Write `cpe-service-state` to a file in `/tmp/` whenever it changes
- On startup in `setDefaultValuesToCfg()`, read the persisted file if it exists and use its value instead of defaulting to `"unknown"`
- If the file does not exist or contains invalid data, default to `"unknown"` (current behavior)

## Impact

- Affected specs: `cpe-service-state`
- Affected code:
  - `src/config.c` — `setDefaultValuesToCfg()`: read persisted state on startup
  - `src/upstream.c` — `update_cpe_service_state()` (around line 275): write state to file on change
  - `src/config.h` — add macro for the tmp file path
  - `tests/test_config.c` — test persistence read logic
  - `tests/test_upstream.c` or `tests/test_upstream_sock.c` — test persistence write logic
