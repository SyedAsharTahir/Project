# Project
Minesweeper
This is a modern implementation of the classic Minesweeper game built using C++ and the SFML library. It features a 10x10 playable grid, three difficulty levels (Easy, Medium, Hard), hint functionality, high score tracking, save/load capabilities, and visual effects like tile animations, screen shake, and particle effects. The game includes a menu system with controls, credits, and settings for an enhanced user experience.
Table of Contents

Installation
Usage
Features
Controls
File Structure
Dependencies
Contributing
License

Installation

Clone the repository:
git clone https://github.com/SyedAsharTahir/minesweeper.git
cd minesweeper


Install SFML:

On Ubuntu:sudo apt-get install libsfml-dev


On macOS (using Homebrew):brew install sfml


On Windows:
Download SFML from sfml-dev.org and follow the setup instructions for your compiler (e.g., Visual Studio, MinGW).




Compile the game:

Ensure you have a C++ compiler (e.g., g++).
Compile the code with SFML libraries linked:g++ -c main.cpp -I/path/to/sfml/include
g++ main.o -o minesweeper -L/path/to/sfml/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio


Replace /path/to/sfml with the actual path to your SFML installation.


Ensure resource files:

Place the images/, sound/, and fonts/ directories in the same directory as the executable. These contain:
images/tiles.jpg: Tile sprites for numbers, mines, and flags.
sound/*.wav: Sound effects for clicks, flags, wins, and losses.
fonts/arial.ttf: Font for text rendering.





Usage
Run the compiled executable:
./minesweeper


Navigate the main menu using the mouse or keyboard (Up/Down arrows, Enter to select).
Start a new game, adjust difficulty, or access settings (controls, credits, high scores).
During gameplay, reveal tiles, place flags, and use hints to avoid mines.
Save or load your game progress from the pause menu.

Features

Dynamic Gameplay: 10x10 grid with randomized mine placement based on difficulty (10%, 15%, or 20% mine probability).
Difficulty Levels: Easy (10% mines), Medium (15% mines), Hard (20% mines).
Hints: Up to 3 hints per game to reveal safe tiles.
Save/Load: Persist game state to savegame.txt and resume later.
High Scores: Tracks top 5 scores based on revealed tiles and time, saved to highscores.txt.
Visual Effects:
Tile animations (scaling on reveal/flag).
Screen shake and flash on game over.
Particle effects for win celebrations and menu backgrounds.


Audio: Sound effects for actions (click, flag, mine, win) and background music for menus.
Menu System: Navigate through main menu, settings, controls, credits, and difficulty selection.

Controls

Left Click: Reveal a tile.
Right Click: Place or remove a flag on a hidden tile.
Middle Click: Chord (reveal adjacent tiles if the correct number of flags are placed around a revealed tile).
R: Restart the game.
H: Use a hint to reveal a safe tile.
Esc: Pause the game or return to the main menu from other screens.
Up/Down Arrows: Navigate menu options.
Enter: Select a menu option.
1, 2, 3: Select difficulty (Easy, Medium, Hard) in the difficulty menu.

File Structure
minesweeper/
├── main.cpp              # Main game source code
├── images/
│   └── tiles.jpg         # Tile sprites (numbers, mines, flags)
├── sound/
│   ├── click.wav         # Tile reveal sound
│   ├── flag.wav          # Flag placement/removal sound
│   ├── lobby.wav         # Menu background music
│   ├── lose_flowergarden_short.wav  # Game over sound (short)
│   ├── lose_flowergarden_medium.wav # Game over sound (medium)
│   ├── lose_flowergarden_long.wav   # Game over sound (long)
│   ├── lose_minesweeper.wav         # Mine hit sound
│   ├── start.wav         # Game start/load sound
│   └── win.wav           # Win sound
├── fonts/
│   └── arial.ttf         # Font for text rendering
├── savegame.txt          # Saved game state (generated)
└── highscores.txt        # High scores (generated)

Dependencies

SFML 2.5+: Simple and Fast Multimedia Library for graphics, windowing, audio, and system utilities.
C++11 or later: Standard C++ library for random number generation, file I/O, and other utilities.

Contributing

Fork the repository.
Create a feature branch (git checkout -b feature-branch).
Commit your changes (git commit -m 'Add new feature').
Push to the branch (git push origin feature-branch).
Open a Pull Request.
