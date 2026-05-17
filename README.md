# Morbid Growth - Infection Horror Game

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5-black.svg)](https://www.unrealengine.com/)
[![School Project](https://img.shields.io/badge/Second%20Year-Collaboration-purple.svg)]()

![Game Screenshot](documents/screenshots/gameplay1.png)

## Table of Contents
- [About The Project](#about-the-project)
- [The Infection System](#the-infection-system)
- [Features](#features)
- [Built With](#built-with)
- [Getting Started](#getting-started)
- [Controls](#controls)
- [Team & Collaboration](#team--collaboration)
- [Screenshots](#screenshots)

## About The Project

**Morbid Growth** is a first-person horror game developed during my second year as a **programmer-designer collaboration** (2 programmers and 3 designers). The designers created a detailed specification document, and we implemented the infection AI system.

The game pits the player against an intelligent, spreading infection that consumes the environment tile by tile. Unlike standard horror enemies, this infection "thinks" — it prioritizes spreading toward you, grows watching eyes from its mass, and launches organic tendril attacks when you get too close. Your only defense is a flamethrower with limited, regenerating fuel.

### The Core Loop

1. **Infection spreads** across the map from predetermined starting points
2. **You burn infected tiles** with your flamethrower to survive
3. **Higher-level infections** develop eyes and launch tendril attacks
4. **The infection hunts you** — it actively spreads in your direction

### Collaboration Context

This project was unique because I worked directly from **designers' design document** (included in the repository). All gameplay values — spread speed, attack chance, damage numbers, fuel rates — were specified by the designer to be adjustable without touching code.

## The Infection System

### Tile-Based Propagation

The map is composed entirely of tiles, each with:
- An infection level (0-5+)
- Visual representation that changes with each level
- Collision behavior that evolves as infection grows
- Individual spread timing (randomized per tile)

### Spread Logic

| Rule | Description |
|------|-------------|
| **Direction** | Spreads to adjacent tiles (no diagonals) |
| **Player priority** | Tiles within [tilePlayerDetectionRange] prefer spreading toward the player |
| **Max level cap** | Tiles at max infection level stop spreading |
| **Random timing** | Each tile spreads every [minSpreadTime] to [maxSpreadTime] seconds |

### Propagation Volumes

Designers can place rectangular volumes that **restrict where infection can spread**. This allows:
- Controlled difficulty curves
- Performance optimization
- Designed "safe zones" and "danger zones"

## Features

### Infection Level Progression

| Level | Visual | Behavior | Collision |
|-------|--------|----------|-----------|
| **0** | None (clean tile) | Not infected | None |
| **1** | Small black sphere | Slows player on contact | Solid |
| **2** | Larger black sphere | Slows player on contact | Solid |
| **3** | Bulging sphere (vertex displacement) | Slows + damage over time + can attack | Solid |
| **4+** | Same visual as level 3 | Same behavior, higher HP | Solid |
| **5+** | **Eye transformation** | Tracks player, blinks | Solid |

### Eye System (Level 5+ Special)

When a tile reaches level 4, it has a **1 in [infectedEyeChance] chance** to transform into an eye instead of a normal sphere.

- **Tracks the player** - The eye follows your movement
- **Blinks naturally** - Random intervals between [minBlinkTime] and [maxBlinkTime]
- **Can be burned** - Flamethrower reverts eyes to normal spheres
- **Regenerates** - Partially burned eyes grow back

### Tendril Attack (Level 3+)

**Attack details:**
- Attack chance: 1/[tendrilAttackChance] per interval
- Tendril follows a **random curved path** (looks organic, creates panic)
- Deals [tendrilAttackDamage] on hit
- Visual growth effect lasts [tendrilAttackDuration]
- **Only ONE fast tendril** can exist at a time (prevents unfair spam)

### Rapid Root System

When you approach infected tiles, a **fast-moving root** sprints toward your position:
- Travels at [fastTendriSpeed]
- **Infects every tile it touches** (sets them to Level 1)
- Creates chain reactions — one root can trigger massive spread

### Player Systems

| System | Details |
|--------|---------|
| **Perspective** | First-person in protective suit |
| **Health** | [playerMaxHealth] HP |
| **Movement Speed** | [playerMovementSpeed] (reduced when touching infected tiles) |
| **Contact Damage** | [tileContactDamage] every [tileDamageInterval] seconds while touching infection |

### Flamethrower Defense

| Stat | Value |
|------|-------|
| **Damage per hit** | [flamethrowerDamage] |
| **Projectiles per shot** | [flamethrowerProjectileAmount] raycasts |
| **Fire rate** | [flamethrowerFireRate] shots/second |
| **Max fuel** | [maxFlamethrowerFuel] |
| **Fuel consumption** | [flamethrowerFuelDepleteAmount]/second while firing |
| **Fuel regeneration** | [fuelRegenerationAmount]/second while not firing |
| **Minimum fuel to fire** | [flamethrowerMinFuelThreshold]% |

## Built With

| Category | Technology |
|----------|------------|
| **Engine** | Unreal Engine 5 |
| **Language** | C++17 |
| **Infection AI** | Custom tile adjacency system |
| **Visual Effects** | Vertex displacement for bulging spheres |
| **Collision** | UE5 Collision System |

## Getting Started

### Prerequisites
- **Unreal Engine 5.0+**
- **Windows 10/11**
- **Visual Studio 2019/2022** with C++ development tools

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/morbid-growth.git
cd morbid-growth

# Generate Visual Studio project files
# Right-click MorbidGrowth.uproject → Generate Visual Studio project files

# Open the solution
double-click MorbidGrowth.sln

# Build in Visual Studio (F5)
