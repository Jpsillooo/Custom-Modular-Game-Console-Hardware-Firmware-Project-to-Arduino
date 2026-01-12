**Overview**

This repository contains my final high school (12th grade) project: the design and development of a fully custom handheld game console.
The entire system — hardware integration, firmware, and game logic — was designed and implemented from scratch by me.

The project focuses on embedded systems, low-level programming, and the interaction between hardware and software under strict memory and performance constraints.

**System Architecture**

The console is built around an Arduino UNO R3 (ATmega328P) acting as the main microcontroller.

**Hardware Components**

Arduino UNO R3

128×64 monochrome OLED display (I²C)

6 physical push buttons (internal pull-up configuration)

Passive piezo buzzer for audio feedback

External USB power supply (power bank)

The system was first prototyped on a breadboard and later soldered onto a perfboard to obtain a stable and portable final version.

**Firmware**

The firmware is written in Arduino (C/C++) and developed using Arduino IDE.

**Location:**

/firmware
  └── console.ino

**Software Features**

Low-level pixel rendering using Adafruit_GFX and Adafruit_SSD1306

Manual frame-based rendering loop

Custom 8×8 sprites stored in PROGMEM to reduce SRAM usage

Button input handling using internal pull-up resistors

Software debouncing for reliable input

Collision detection and game state management

Audio feedback generated using PWM (tone())

The code is designed to run within the memory and performance limits of the ATmega328P microcontroller.

**Games Implemented**

F1 Snake

A Snake-inspired 2D game with a Formula 1 theme.

Direction-based sprite rendering

Trail effect to simulate motion

Collectible parts that extend the player

Optional collision rules

**2D Tennis**

A simplified Pong-style tennis game.

Player-controlled paddle

AI-controlled opponent

Multiple difficulty levels

Real-time HUD for score display

**Mechanical Design**

The console enclosure was designed in 3D and printed using a school 3D printer.
Several iterations were required to adjust component placement, button alignment, and internal spacing.

**Project Goals**

Learn the fundamentals of embedded systems and microcontrollers

Understand basic console architecture

Develop low-level game logic without external engines

Integrate hardware and software into a functional system

Demonstrate autonomous learning and problem-solving skills

**How to Run**

Open console.ino in Arduino IDE

Install required libraries:

Adafruit_GFX

Adafruit_SSD1306

Select Arduino UNO as the target board

Upload the firmware to the board

Power the console via USB



**Author**

Pau Josep Cantos Mera
Final High School Project – 2025/2026
