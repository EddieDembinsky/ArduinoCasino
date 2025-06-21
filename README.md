# Arduino Casino Game System

A comprehensive casino gaming system implementing multiple games with persistent balance tracking and LCD interface. Features EEPROM data persistence, state machine architecture, and real-time user interaction.

## Games

Blackjack
Full implementation with card logic, ace handling, and dealer AI
Slot Machine
3-reel slots with weighted probability and payout calculations
Roulette
Complete betting system with animated wheel simulation
Poker
Framework prepared for implementation
## Technical Features

EEPROM Persistence
Balance and statistics maintained across power cycles
State Machine Architecture
Clean separation between game states
Multi-input Interface
Joystick navigation with context-sensitive button controls
Memory Optimization
85% program memory utilization on Arduino Uno
Error Handling
Input validation, atomic operations, and data corruption recovery
Custom Graphics
Dynamic LCD character generation and display buffering
## Implementation Details

Seeded pseudorandom number generation for fair gameplay
Hardware input debouncing with software validation
Interrupt-driven input for non-blocking interface
Modular game architecture for extensibility
Authentic casino rules and payout ratios
## Future Development

### Planned Features
- Multi-player capabilities
- Poker implementation

### Hardware Upgrades
- Flashing LEDs
- Buzzer

## Specifications

| Component | Details |
|-----------|---------|
| Platform | Arduino Uno / ATmega328P |
| Memory Usage | 85% Flash, 20 bytes EEPROM |
| Performance | <50ms input latency, <100ms save operations |

---
