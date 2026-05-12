## ADDED Requirements

### Requirement: CPE Service State Persistence

The system SHALL persist the `cpe-service-state` metadata value to a file at `/tmp/parodus_cpe_service_state` whenever the state changes.

#### Scenario: State change triggers file write
- **WHEN** `cpe-service-state` is updated to a valid value (e.g., `fully-manageable`, `operational`, `non-operational`)
- **THEN** the system writes that value to `/tmp/parodus_cpe_service_state`

#### Scenario: File write failure
- **WHEN** the system fails to write to `/tmp/parodus_cpe_service_state`
- **THEN** the system logs an error and continues operation without aborting

### Requirement: CPE Service State Recovery on Startup

The system SHALL read `cpe-service-state` from `/tmp/parodus_cpe_service_state` during `setDefaultValuesToCfg()` if the file exists and contains a valid state.

#### Scenario: Valid persisted state on startup
- **WHEN** parodus starts and `/tmp/parodus_cpe_service_state` exists with content `fully-manageable`
- **THEN** `cpe_service_state` in `ParodusCfg` is set to `fully-manageable`

#### Scenario: Invalid persisted state on startup
- **WHEN** parodus starts and `/tmp/parodus_cpe_service_state` exists with invalid content
- **THEN** `cpe_service_state` in `ParodusCfg` is set to `unknown`

#### Scenario: No persisted state file on startup
- **WHEN** parodus starts and `/tmp/parodus_cpe_service_state` does not exist
- **THEN** `cpe_service_state` in `ParodusCfg` is set to `unknown`

## MODIFIED Requirements

### Requirement: CPE Service State Default Value

The system SHALL set `cpe_service_state` to the value read from `/tmp/parodus_cpe_service_state` if the file exists and contains a valid state; otherwise it SHALL default to `"unknown"`.

#### Scenario: Default with no persisted file
- **WHEN** `setDefaultValuesToCfg()` is called and no persisted state file exists
- **THEN** `cpe_service_state` is `"unknown"`

#### Scenario: Default with valid persisted file
- **WHEN** `setDefaultValuesToCfg()` is called and persisted state file contains `"operational"`
- **THEN** `cpe_service_state` is `"operational"`
