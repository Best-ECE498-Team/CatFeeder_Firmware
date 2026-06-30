# PawPlate Firmware

Firmware for the PawPlate Intelligent Wet Food Dispenser.

Built using:

* STM32G474RE
* STM32CubeIDE v1.19.0
* STM32CubeMx v6.170
* STM32CubeProgrammer
* Sublime Merge (optional)
* Any editor you like

---

## Getting Started

### Clone the Repository

```bash
git clone <repository-url>
```

### Open in STM32CubeIDE

1. Open STM32CubeIDE v1.19.0
2. Import Existing Project
3. Select the repository folder
4. Build the project

---

## Project Structure

```text
App/
├── Communication_Module/
├── Feeding_Module/
├── Thermal_Module/
└── System_Manager/
└── ...(To be added)/

Core/
Drivers/
Middlewares/
Startup/
Platform/
```

### Module Layout

Each module follows the same structure:

```text
Module/
├── Inc/
│   ├── xxx_module.h
│   ├── xxx_task.h
│
└── Src/
    ├── xxx_module.c
    └── xxx_task.c
```

### Responsibilities

| File         | Responsibility                 |
| ------------ | ------------------------------ |
| xxx_task.c   | FreeRTOS task                  |
| xxx_module.c | Module logic and state machine |
| xxx_driver.c | Hardware access                |
| xxx_module.h | Public API                     |

---

## Development Rules

### 1. Keep Modules Independent

Each module owns its own data and state.

Avoid:

```c
extern SomeGlobalVariable;
```

Use module APIs instead.

---

### 2. Drivers Only Control Hardware

Drivers should not contain:

* State machines
* Scheduling
* Business logic

---

### 3. Tasks Communicate Through Messages

Use:

* Message Queues
* Event Flags
* Semaphores

Avoid directly modifying another task's internal state.

---

### 4. One Responsibility Per File

Examples:

```text
heater_driver.c      -> Heater hardware
thermal_module.c     -> Thermal logic
thermal_task.c       -> Thermal task
```

---

## Git Workflow

### Before Starting Work

Update your local main branch:

```bash
git checkout main
git pull origin main
```

### Create a Feature Branch

```bash
git checkout -b feature/my-feature
```

Examples:

```text
feature/thermal-control
feature/feed-door-driver
feature/uart-protocol
```

### Commit Changes

```bash
git add .
git commit -m "Implement TEC driver"
```

### Push Branch

```bash
git push origin feature/my-feature
```

### Create Pull Request

1. Push your feature branch
2. Open GitHub
3. Create a Pull Request
4. Request a review
5. Merge only after approval

Never commit directly to `main`.

---

## Code Review Checklist

Before creating a PR:

* Project builds successfully
* No new warnings introduced
* Code follows module structure
* Hardware code stays in drivers
* Tested on hardware when possible
