Final High School (12th Grade) Engineering Project
**Overview**

This repository documents the design and implementation of a fully custom handheld game console developed as a final high school project.
All hardware integration, firmware, and game logic were designed and programmed from scratch by me.

The project focuses on low-level system design, embedded programming, and the interaction between hardware and software under strict memory and performance constraints.

**System Architecture**

The console is built around an Arduino UNO R3 (ATmega328P) acting as the main microcontroller.
It interfaces with:

a 128×64 monochrome OLED display over I²C

six physical buttons using internal pull-up resistors

a passive piezo buzzer for audio feedback

an external USB power source for portable operation

All components were first prototyped on a breadboard, then transferred to a soldered perfboard for stability.

**Software Design**

All software was developed in Arduino IDE (C/C++) without external game engines.

Key technical aspects include:

Direct pixel-based rendering using Adafruit_GFX and Adafruit_SSD1306

Custom 8×8 bitmaps stored in PROGMEM to minimize SRAM usage

Frame-based rendering loop with explicit screen refresh control

Button input handling with software debouncing

Collision detection and game state management

Simple audio feedback generated via PWM (tone())

**Games Implemented**

F1 Snake

A Snake-inspired game with a Formula 1 theme.
The player controls a car that grows by collecting parts, with direction-based sprite rendering, trail effects, and optional collision rules.

2D Tennis

A simplified Pong-style game featuring:

Player-controlled paddle

Basic AI opponent

Multiple difficulty levels affecting ball speed and AI reaction time

Real-time score display (HUD)

**Mechanical Design**

The console enclosure was designed in 3D and printed using a school 3D printer.
Several design iterations were required to ensure proper component fit, button alignment, and screen visibility.

**Project Goals**

Understand the internal architecture of a simple 2D game console

Learn embedded systems programming with limited memory and processing power

Integrate hardware and software into a fully functional system

Demonstrate autonomous learning and problem-solving in a technical project

**Author**

Pau Josep Cantos Mera
Final High School Project – 2025/2026
