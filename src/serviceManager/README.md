# Service Manager

Lifecycle management service that supervises all other services: start, stop,
suspend, resume, and watchdog-backed heartbeat monitoring.

## Key Features

- Centralized service lifecycle control (start / stop / suspend / resume)
- Watchdog-backed heartbeat monitoring with configurable timeout
- Shell commands for runtime inspection
- Index-free logging using thread names

## Configuration

| Kconfig symbol | Description |
|---|---|
| `CONFIG_ENYA_SERVICE_MANAGER` | Enable the service manager |
| `CONFIG_ENYA_SERVICE_MANAGER_THREAD_PRIORITY` | Service manager thread priority |
| `CONFIG_ENYA_SERVICE_MANAGER_SHELL` | Enable shell commands |

## API

See @ref serviceManager.h and @ref serviceManagerUtil.h for the full API.
