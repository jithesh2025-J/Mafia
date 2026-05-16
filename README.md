# Mafia Game

A C++ implementation of the classic Mafia social deduction game featuring modular object-oriented architecture, role-based gameplay mechanics, voting systems, and turn-based game flow.

---

## Features

- Multiple player roles
- Doctor healing system
- Detective investigation mechanics
- Mafia elimination phase
- Voting and discussion system
- Turn-based gameplay loop
- Modular OOP-based architecture
- Separate header and implementation files
- Error handling and game-state management
- AI-assisted development workflow using GitHub Copilot

---

## Tech Stack

- C++
- Object-Oriented Programming (OOP)
- Git & GitHub

---

## Project Structure

```bash
Mafia/
│
├── AIClient.cpp
├── AIClient.h
├── Detective.cpp
├── Detective.h
├── Doctor.cpp
├── Doctor.h
├── Game.cpp
├── Game.h
├── Player.cpp
├── Player.h
├── Mafia.cpp
├── Mafia.h
├── AllExeptions.cpp
├── AllExeptions.h
├── main.cpp
├── .gitignore
└── README.md
```

---

## Gameplay Overview

The game follows the traditional Mafia format:

1. Players are assigned different roles.
2. Mafia members secretly eliminate players during the night phase.
3. Special roles such as Doctor and Detective perform their actions.
4. Players discuss and vote during the day phase.
5. The game continues until either:
   - All Mafia members are eliminated
   - Mafia outnumbers remaining players

---

## Object-Oriented Design

The project uses modular OOP principles including:

- Classes for different player roles
- Encapsulation of gameplay mechanics
- Separate implementation and header files
- Reusable game logic
- Scalable architecture for future features

---

## How To Compile

Using g++:

```bash
g++ *.cpp -o mafia
```

---

## How To Run

```bash
./mafia
```

For Windows:

```bash
mafia.exe
```

---

## Future Improvements

Planned upgrades and ideas:

- Multiplayer support
- Network socket implementation
- Graphical user interface (GUI)
- AI-controlled players
- Save/load game system
- Voice chat integration
- Improved game balancing
- Online matchmaking
- Web-based version using React and Node.js

---

## Learning Outcomes

This project helped in learning and practicing:

- Object-Oriented Programming
- Game-state management
- Modular software architecture
- Debugging and problem solving
- Git and version control
- Collaborative AI-assisted development workflow

---

## GitHub

Repository:
https://github.com/jithesh2025-J/Mafia

---

## Author

Developed by Jithesh S

GitHub:
https://github.com/jithesh2025-J

---

## License

This project is open-source and available under the MIT License.
