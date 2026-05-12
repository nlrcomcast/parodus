# Project Context

## Purpose
Parodus is a C implementation of the XMiDT client coordinator. XMiDT is a highly scalable, highly available, generic message routing system. Parodus establishes and maintains a persistent WebSocket connection to the XMiDT cloud server, handles bidirectional message routing between local clients (via nanomsg IPC) and the cloud, manages authentication (JWT/mTLS), heartbeat keepalive, and graceful reconnection with exponential backoff.

## Tech Stack
- **Language**: C (C99 standard)
- **Build System**: CMake
- **WebSocket Library**: nopoll (1.0.3)
- **IPC / Message Queue**: nanomsg (1.1.4)
- **Serialization**: msgpack-c (cpp-3.1.1), cJSON (v1.7.8)
- **HTTP Client**: libcurl (7.63.0)
- **Message Protocol**: wrp-c (Webpa Routing Protocol)
- **Logging**: cimplog (1.0.2)
- **Encoding**: trower-base64 (v1.1.1)
- **Authentication**: cjwt (JWT handling), OpenSSL (TLS/mTLS)
- **Optional**: libseshat (service discovery), ucresolv (DNS query), rbus (event bus)
- **Containerization**: Docker (Ubuntu 20.04 base)

## Project Conventions

### Code Style
- C99 standard with `-D_GNU_SOURCE`
- Snake_case for functions and variables (e.g., `get_parodus_cfg()`, `parse_mac_address()`)
- CamelCase for struct types (e.g., `ParodusCfg`, `ParodusMsg`, `UpStreamMsg`)
- Header guards using `#ifndef` / `#define` pattern
- Logging via `ParodusError()`, `ParodusInfo()`, `ParodusPrint()` macros (wrapping cimplog)
- Thread safety enforced via pthread mutexes and condition variables
- Static global configuration accessed through getter/setter functions

### Architecture Patterns
- **Multi-threaded daemon**: Main thread manages WebSocket connection; separate threads for upstream/downstream message processing, heartbeat, and service alive checks
- **Message routing**: Upstream (local clients → cloud) and downstream (cloud → local clients) via nanomsg IPC sockets
- **Connection management**: Persistent WebSocket with exponential backoff retry, JWT-based server discovery, and redirect handling
- **Configuration**: Command-line argument parsing into a global `ParodusCfg` struct with thread-safe accessors
- **Signal handling**: SIGTERM/SIGINT for graceful shutdown, SIGUSR1 for system restart, with configurable shutdown reasons
- **Callback-based events**: Connection status change and ping status change handlers registered by consumers
- **Conditional compilation**: Feature flags (`ENABLE_SESHAT`, `FEATURE_DNS_QUERY`, `ENABLE_WEBCFGBIN`, `WAN_FAILOVER_SUPPORTED`, `INCLUDE_BREAKPAD`) for optional components
- **Build modes**: `BUILD_YOCTO` for system-provided libraries vs. self-built ExternalProject dependencies

### Testing Strategy
- **Unit Testing Framework**: CUnit
- **Mocking Framework**: cmocka (function-level mocking)
- **Coverage**: gcov with `-fprofile-arcs -ftest-coverage -O0 -g` flags
- **Memory Checking**: Valgrind (`--leak-check=full --show-reachable=yes`)
- **Test count**: 35+ test binaries covering unit, integration, protocol, authentication, configuration, and threading
- **Test naming**: `test_<module>.c` mirroring `src/<module>.c`
- **Execution**: `cmake .. && make && make test`, or individual test runs via Valgrind

### Git Workflow
- Fork-based contribution model
- Feature branches with focused, narrowly-scoped pull requests (3-4 logical commits max)
- One issue addressed per PR
- Comcast CLA signing required for contributions
- Code reviewed via GitHub code review tool
- README and wiki updated for behavior changes and new features

## Domain Context
Parodus operates within the XMiDT ecosystem for CPE (Customer Premises Equipment) device management. It runs on embedded devices (cable modems, routers, set-top boxes) and acts as the local coordinator between device services and the XMiDT cloud platform. Messages use the WRP (Webpa Routing Protocol) format serialized with msgpack. Device identity is established through hardware metadata (MAC address, serial number, manufacturer, model) passed as command-line arguments. The system supports WAN failover, interface up/down events, and connection health monitoring for RDK (Reference Design Kit) based devices.

## Important Constraints
- RSA-only JWT algorithm enforcement (RS256, RS384, RS512); non-RSA algorithms explicitly disallowed
- Privilege de-escalation via `drop_root_privilege()` at startup
- Must handle network instability gracefully with backoff retry and reconnection
- C99 compliance required
- Thread safety is critical — all shared state protected by mutexes/condition variables
- Embedded device deployment: resource-constrained environments (memory, CPU)
- Must support both Yocto-based (system library) and standalone (self-built dependency) builds

## External Dependencies
- **XMiDT Cloud Server**: WebSocket endpoint for device-to-cloud communication (configurable via `--webpa-url`)
- **Token Server**: JWT token acquisition endpoint (configurable via `--token-server-url`)
- **Seshat**: Optional service discovery/registration service
- **nanomsg IPC**: Local upstream socket at `tcp://127.0.0.1:6666` for inter-process communication with device services
- **DNS TXT Records**: Optional DNS-based service discovery (`--dns-txt-url`)
- **RBUS**: Optional event bus for WAN state and interface change events
- **OpenSSL**: TLS/mTLS for secure WebSocket and HTTP connections
- **Breakpad**: Optional crash reporting integration

## Additional Notes
- Current stable version: 1.1.4
- The project includes a Dockerfile for containerized builds and testing on Ubuntu 20.04
- Patch files exist for nanomsg customization (`patches/nanomsg.patch`)
- The codebase includes stub implementations (`seshat_interface_stub.c`, `token_stub.c`) for builds without optional features
- Connection health is tracked via file-based status reporting (`cloud_status`, `connection_health_file`, `close_reason_file`)
