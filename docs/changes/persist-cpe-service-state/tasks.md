## 1. Implementation

- [x] 1.1 Add `CPE_SERVICE_STATE_FILE` macro to `src/config.h` (`"/tmp/parodus_cpe_service_state"`)
- [x] 1.2 Add helper function to write `cpe-service-state` to the tmp file (in `src/config.c` or `src/upstream.c`)
- [x] 1.3 Add helper function to read and validate `cpe-service-state` from the tmp file (in `src/config.c`)
- [x] 1.4 Update `setDefaultValuesToCfg()` in `src/config.c` to call the read helper and use persisted value if valid
- [x] 1.5 Update state change logic in `src/upstream.c` (around line 275) to call the write helper after updating `cpe_service_state`

## 2. Testing

- [x] 2.1 Add unit tests for read helper: file exists with valid state, file exists with invalid state, file does not exist
- [x] 2.2 Add unit tests for write helper: verify file is created/updated with correct content
- [x] 2.3 Add integration test for `setDefaultValuesToCfg()` verifying it reads persisted state on startup
- [ ] 2.4 Verify existing `test_packMetaData` tests still pass (no regressions)

## Dependencies

- Tasks 1.2 and 1.3 can be done in parallel
- Task 1.4 depends on 1.3; Task 1.5 depends on 1.2
- All testing tasks (2.x) depend on their corresponding implementation tasks
