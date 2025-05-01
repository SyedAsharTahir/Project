#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Config.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <vector>
#include <ctime>
#include <optional>
#include <cstdlib>
#include <random>
#include <fstream>
#include <algorithm>
using namespace sf;
using namespace std;

// Main game class encapsulating Minesweeper logic, rendering, and input handling
class MinesweeperGame {
private:
    // Constants for grid dimensions and tile size
    static const int GRID_SIZE = 12; // 12x12 grid (10x10 playable, 1-cell border)
    static const int TILE_SIZE = 32; // Pixel size of each tile
    static constexpr float GRID_OFFSET_X = 20.f; // X offset to center grid horizontally
    static constexpr float GRID_OFFSET_Y = 50.f; // Y offset to position grid vertically
    int MINE_PROBABILITY = 10; // Initial mine probability (10% for easy mode)

    // Enum for game states to manage different screens
    enum GameState { MENU, PLAYING, CONTROLS, CREDITS, DIFFICULTY, HIGH_SCORES, SETTINGS, PAUSE };

    // SFML window and core rendering resources
    RenderWindow window; // Main game window (450x500 pixels)
    Texture tileTexture; // Texture for tile sprites (numbers, mines, flags)
    Sprite tileSprite{tileTexture}; // Reusable sprite for drawing tiles
    Font font; // Font for all text rendering

    // Background rectangle for gameplay
    RectangleShape background; // Solid color background for game grid

    // Text objects for game UI
    Text gameOverText{font, "Game Over", 30}; // Displayed when player hits a mine
    Text winText{font, "You Win!", 30}; // Displayed when all non-mines are revealed
    Text scoreText{font, "Score: 0", 20}; // Tracks revealed non-mine tiles
    Text timerText{font, "Time: 0", 20}; // Tracks elapsed game time
    Text minesText{font, "Mines: 0", 20}; // Shows total mines
    Text hintsText{font, "Hints: 3", 20}; // Shows available hints
    Text difficultyConfirmText{font, "", 20}; // Confirms difficulty selection
    Text restartText{font, "Press R to Restart or H for Hint", 20}; // Instructions post-game
    Text statusMessage{font, "", 20}; // Temporary messages (e.g., save/load status)
    Text highScoresText{font, "HIGH SCORES:\n", 20}; // Lists top 5 scores
    Clock confirmClock; // Timer for difficulty confirmation message
    Clock statusMessageClock; // Timer for status messages
    bool showingConfirm = false; // Tracks if difficulty confirmation is displayed
    bool showingStatusMessage = false; // Tracks if status message is displayed
    const float confirmDuration = 1.5f; // Duration for difficulty confirmation (seconds)
    const float statusMessageDuration = 3.0f; // Duration for status messages (seconds)

    // Animation for tiles (e.g., scale effect when revealed or flagged)
    struct TileAnimation {
        float scale = 1.0f; // Current scale factor
        float timer = 0.0f; // Animation progress
        bool active = false; // Whether animation is active
    };
    TileAnimation tileAnimations[GRID_SIZE][GRID_SIZE]; // Animation state per tile
    const float tileAnimationDuration = 0.2f; // Duration of tile animations (seconds)

    // Particle system for win celebration and menu background
    struct Particle {
        Vector2f position; // Current position
        Vector2f velocity; // Movement direction and speed
        Color color; // Particle color
        float lifetime; // Remaining time before disappearance
    };
    vector<Particle> particles; // Active particles
    const float particleLifetime = 2.0f; // Particle duration for win effect (seconds)

    // Animation for menu items (e.g., pulsing effect)
    struct MenuAnimation {
        float scale = 1.0f; // Current scale factor
        float timer = 0.0f; // Animation progress
    };
    vector<MenuAnimation> menuAnimations; // Animation state per menu item
    const float menuAnimationSpeed = 2.0f; // Speed of menu item pulsing
    const float menuAnimationAmplitude = 0.05f; // Scale variation for pulsing

    // Animation for static screen titles
    float titleAnimationTimer = 0.0f; // Timer for title pulsing effect

    // Transition overlay for smooth state changes
    RectangleShape transitionOverlay; // Black overlay for fade effect
    float transitionAlpha = 0.0f; // Current opacity (0 to 1)
    float transitionDuration = 0.3f; // Duration of transition (seconds)
    bool inTransition = false; // Tracks if transition is active
    GameState targetState = MENU; // Destination state for transition

    // Game state and menu items
    GameState currentState = MENU; // Current game state
    vector<Text> menuItems; // Main menu items (Play, Difficulty, etc.)
    vector<Text> settingsItems; // Settings menu items (Controls, Credits, etc.)
    vector<Text> pauseItems; // Pause menu items (Resume, Save, etc.)
    vector<Text> difficultyItems; // Difficulty menu items (Easy, Medium, Hard)
    int selectedMenuItem = 0; // Currently selected menu item index
    Text menuTitle{font, "MINESWEEPER", 36}; // Main menu title

    // Screen shake effect for game over
    bool shakeActive = false; // Tracks if shake is active
    float shakeTime = 0.f; // Current shake progress
    float shakeDuration = 0.5f; // Shake duration (seconds)
    float shakeMagnitude = 8.0f; // Max shake offset (pixels)
    Vector2f originalViewCenter; // Default view center for grid
    Clock shakeClock; // Timer for shake effect
    std::mt19937 randomEngine{static_cast<unsigned>(std::time(nullptr))}; // Random number generator
    std::uniform_real_distribution<float> randomDist{-1.f, 1.f}; // Random float in [-1, 1]

    // Flash effect for game over
    RectangleShape flashOverlay; // White overlay for flash
    Clock flashClock; // Timer for flash effect
    float flashDuration = 0.0f; // Duration of flash (seconds)

    // Sound system managing all audio
    struct SoundSystem {
        // Sound buffers for various game events
        SoundBuffer startBuffer, clickBuffer, mineBuffer, winBuffer, flagBuffer, hintBuffer;
        SoundBuffer loseBuffers[3]; // Multiple lose sounds for variety
        Sound startSound; // Played on game start/load
        Sound clickSound; // Played on tile reveal
        Sound mineSound; // Played on mine hit
        Sound winSound; // Played on win
        Sound flagSound; // Played on flag placement/removal
        Sound hintSound; // Played on hint usage
        Sound loseSounds[3]; // Played on game over
        Music menuMusic; // Background music for menus

        SoundSystem() :
            startSound(startBuffer), clickSound(clickBuffer),
            mineSound(mineBuffer), winSound(winBuffer),
            flagSound(flagBuffer), hintSound(hintBuffer),
            loseSounds{Sound(loseBuffers[0]), Sound(loseBuffers[1]), Sound(loseBuffers[2])},
            menuMusic()
        {}

        // Load all sound files, throw on critical failures
        void loadSounds() {
            if (!startBuffer.loadFromFile("sound/start.wav") ||
                !clickBuffer.loadFromFile("sound/click.wav") ||
                !mineBuffer.loadFromFile("sound/lose_minesweeper.wav") ||
                !winBuffer.loadFromFile("sound/win.wav") ||
                !flagBuffer.loadFromFile("sound/flag.wav") ||
                !hintBuffer.loadFromFile("sound/click.wav")) {
                throw runtime_error("Failed to load critical sound files");
            }
            if (!loseBuffers[0].loadFromFile("sound/lose_flowergarden_short.wav") ||
                !loseBuffers[1].loadFromFile("sound/lose_flowergarden_medium.wav") ||
                !loseBuffers[2].loadFromFile("sound/lose_flowergarden_long.wav")) {
                cerr << "Warning: Some lose sound files failed to load" << endl;
            }
            if (!menuMusic.openFromFile("sound/lobby.wav")) {
                cerr << "Warning: Failed to load menu music" << endl;
            } else {
                menuMusic.setLooping(true);
                menuMusic.setVolume(50); // Default volume
            }
        }
    } sounds;

    // Game grid and state
    int grid[GRID_SIZE][GRID_SIZE] = {0}; // Actual grid: 0-8 (mine count), 9 (mine)
    int visibleGrid[GRID_SIZE][GRID_SIZE] = {0}; // Visible grid: 0-8 (revealed), 9 (mine), 10 (hidden), 11 (flagged)
    bool gameOver = false; // Tracks if player hit a mine
    bool gameWon = false; // Tracks if all non-mines are revealed
    int score = 0; // Number of revealed non-mine tiles
    Clock gameClock; // Tracks game duration
    Time elapsedTime = gameClock.getElapsedTime(); // Current elapsed time
    int totalMines = 0; // Total mines in current game
    float timeOffset = 0.f; // Saved time for load/resume
    int hintsAvailable = 3; // Available hints for revealing safe tiles

    // Check if a file exists
    bool fileExists(const string& filename) {
        ifstream file(filename);
        return file.good();
    }

    // Display a temporary status message with specified color
    void showStatusMessage(const string& message, const Color& color = Color::White) {
        statusMessage.setString(message);
        statusMessage.setFillColor(color);
        statusMessage.setPosition(Vector2f(100.f, 420.f)); // Bottom center
        showingStatusMessage = true;
        statusMessageClock.restart();
        sounds.clickSound.play();
    }

    // Start animation for a tile (e.g., scale up/down for reveal/flag)
    void startTileAnimation(int x, int y) {
        tileAnimations[x][y].scale = (visibleGrid[x][y] == 11 || visibleGrid[x][y] == 10) ? 1.0f : 0.0f;
        tileAnimations[x][y].timer = 0.0f;
        tileAnimations[x][y].active = true;
    }

    // Spawn particles for win celebration
    void spawnWinParticles() {
        particles.clear();
        for (int i = 0; i < 50; i++) {
            Particle p;
            p.position = Vector2f(rand() % 450, -10.f); // Start above screen
            p.velocity = Vector2f((rand() % 200 - 100) / 100.f, rand() % 200 / 100.f + 2.0f); // Random direction
            p.color = Color(rand() % 256, rand() % 256, rand() % 256); // Random color
            p.lifetime = particleLifetime;
            particles.push_back(p);
        }
    }

    // Start a fade transition to a new game state
    void startTransition(GameState newState) {
        inTransition = true;
        transitionAlpha = 0.0f;
        targetState = newState;
        flashClock.restart();
        if (newState == MENU || newState == SETTINGS || newState == PAUSE || newState == DIFFICULTY) {
            selectedMenuItem = 0; // Reset menu selection
        }
        // Manage music: pause in gameplay, play in menus
        if (newState == PLAYING) {
            sounds.menuMusic.pause();
        } else if (currentState == PLAYING && 
                   (newState == MENU || newState == SETTINGS || newState == PAUSE || 
                    newState == DIFFICULTY || newState == CONTROLS || newState == CREDITS || 
                    newState == HIGH_SCORES)) {
            if (sounds.menuMusic.getStatus() != sf::SoundSource::Status::Playing) {
                sounds.menuMusic.play();
            }
        }
    }

    // Return total number of mines (used for display)
    int countRemainingMines() {
        return totalMines;
    }

    // Initialize game grid with mines and numbers
    void initializeGrid() {
        totalMines = 0;
        // Place mines randomly based on MINE_PROBABILITY
        for (int i = 1; i <= 10; i++) {
            for (int j = 1; j <= 10; j++) {
                visibleGrid[i][j] = 10; // All tiles hidden
                grid[i][j] = (rand() % 100 < MINE_PROBABILITY) ? 9 : 0; // 9 for mine
                if (grid[i][j] == 9) totalMines++;
            }
        }

        // Calculate adjacent mine counts for non-mine tiles
        for (int i = 1; i <= 10; i++) {
            for (int j = 1; j <= 10; j++) {
                if (grid[i][j] == 9) continue;
                int mines = 0;
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (i + dx >= 1 && i + dx <= 10 && j + dy >= 1 && j + dy <= 10) {
                            if (grid[i + dx][j + dy] == 9) mines++;
                        }
                    }
                }
                grid[i][j] = mines; // Set number of adjacent mines
            }
        }
        minesText.setString("Mines: " + to_string(totalMines));
        hintsAvailable = 3; // Reset hints
        hintsText.setString("Hints: " + to_string(hintsAvailable));
    }

    // Recursively reveal a tile and adjacent tiles if no adjacent mines
    void revealTile(int x, int y) {
        if (x < 1 || x > 10 || y < 1 || y > 10 || visibleGrid[x][y] != 10) return;

        visibleGrid[x][y] = grid[x][y]; // Reveal tile
        startTileAnimation(x, y);
        if (grid[x][y] == 0) {
            // Recursively reveal adjacent tiles if no adjacent mines
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    revealTile(x + dx, y + dy);
                }
            }
        } else {
            score++; // Increment score for non-zero tile
        }
    }

    // Provide a hint by revealing a random safe tile
    void provideHint() {
        if (gameOver || gameWon) {
            showStatusMessage("Game is over, no hints available", Color::Red);
            return;
        }
        if (hintsAvailable <= 0) {
            showStatusMessage("No hints remaining", Color::Red);
            return;
        }

        // Collect all safe (non-mine, hidden) tiles
        vector<pair<int, int>> safeTiles;
        for (int i = 1; i <= 10; i++) {
            for (int j = 1; j <= 10; j++) {
                if (grid[i][j] != 9 && visibleGrid[i][j] == 10) {
                    safeTiles.emplace_back(i, j);
                }
            }
        }

        if (safeTiles.empty()) {
            showStatusMessage("No safe tiles to reveal", Color::Red);
            return;
        }

        // Randomly select and reveal a safe tile
        std::uniform_int_distribution<size_t> dist(0, safeTiles.size() - 1);
        auto [x, y] = safeTiles[dist(randomEngine)];
        revealTile(x, y);
        hintsAvailable--;
        hintsText.setString("Hints: " + to_string(hintsAvailable));
        showStatusMessage("Hint used: Safe tile revealed", Color::Green);
        sounds.hintSound.play();
    }

    // Trigger screen shake effect (e.g., on game over)
    void triggerShake(float duration, float magnitude) {
        shakeActive = true;
        shakeTime = 0.f;
        shakeDuration = duration;
        shakeMagnitude = magnitude;
        shakeClock.restart();
    }

    // Trigger screen flash effect (e.g., on game over)
    void triggerFlash(float duration) {
        flashDuration = duration;
        flashClock.restart();
        flashOverlay.setFillColor(Color(255, 255, 255, 255));
    }

    // Update visual effects (shake, flash, menu animations, title animation)
    void updateEffects() {
        if (shakeActive) {
            shakeTime = shakeClock.getElapsedTime().asSeconds();
            if (shakeTime >= shakeDuration) {
                shakeActive = false;
                View view = window.getView();
                view.setCenter(originalViewCenter); // Reset view
                window.setView(view);
            } else {
                // Apply decaying random offset to view
                float decay = 1.f - shakeTime / shakeDuration;
                float currentMagnitude = shakeMagnitude * decay;
                float offsetX = randomDist(randomEngine) * currentMagnitude;
                float offsetY = randomDist(randomEngine) * currentMagnitude;
                offsetX = std::max(-shakeMagnitude, std::min(shakeMagnitude, offsetX));
                offsetY = std::max(-shakeMagnitude, std::min(shakeMagnitude, offsetY));
                View view = window.getView();
                Vector2f newCenter = originalViewCenter + Vector2f(offsetX, offsetY);
                // Clamp view to prevent excessive movement
                newCenter.x = std::max(225.f - shakeMagnitude, std::min(225.f + shakeMagnitude, newCenter.x));
                newCenter.y = std::max(250.f - shakeMagnitude, std::min(250.f + shakeMagnitude, newCenter.y));
                view.setCenter(newCenter);
                window.setView(view);
            }
        }

        if (flashDuration > 0) {
            // Fade out flash overlay
            float elapsed = flashClock.getElapsedTime().asSeconds();
            float progress = elapsed / flashDuration;
            if (progress >= 1.0f) {
                flashDuration = 0;
                flashOverlay.setFillColor(Color(255, 255, 255, 0));
            } else {
                int alpha = static_cast<int>(255 * (1.0f - progress));
                flashOverlay.setFillColor(Color(255, 255, 255, alpha));
            }
        }

        // Update menu item animations (pulsing effect)
        for (size_t i = 0; i < menuAnimations.size(); i++) {
            menuAnimations[i].timer += 1.0f / 60.0f;
            float offset = (i == selectedMenuItem) ? 0.1f : 0.0f;
            menuAnimations[i].scale = 1.0f + menuAnimationAmplitude * sin(menuAnimations[i].timer * menuAnimationSpeed) + offset;
        }

        // Update title animation for static screens
        titleAnimationTimer += 1.0f / 60.0f;
    }

    // Draw gameplay background
    void drawGameBackground() {
        window.draw(background); // Draw solid color background
    }

    // Draw menu background with gradient and particles
    void drawMenuBackground() {
        // Gradient
        VertexArray gradient(PrimitiveType::Triangles, 6);
        gradient[0].position = Vector2f(0, 0);
        gradient[1].position = Vector2f(450, 0);
        gradient[2].position = Vector2f(450, 500);
        gradient[3].position = Vector2f(450, 500);
        gradient[4].position = Vector2f(0, 500);
        gradient[5].position = Vector2f(0, 0);
        Color topColor(50, 50, 100);
        Color bottomColor(20, 20, 50);
        for (int i = 0; i < 3; ++i) gradient[i].color = topColor;
        for (int i = 3; i < 6; ++i) gradient[i].color = bottomColor;
        window.draw(gradient);

        // Particles
        if (currentState != PLAYING && particles.size() < 20) { // Avoid particles during gameplay win effect
            Particle p;
            p.position = Vector2f(rand() % 450, rand() % 500);
            p.velocity = Vector2f((rand() % 100 - 50) / 100.f, (rand() % 100 - 50) / 100.f);
            p.color = Color(100, 100, 255, 100); // Faint blue
            p.lifetime = 5.0f; // Longer lifetime for menu
            particles.push_back(p);
        }
        for (auto it = particles.begin(); it != particles.end();) {
            it->position += it->velocity;
            it->lifetime -= 1.0f / 60.0f;
            if (it->lifetime <= 0 || it->position.x < 0 || it->position.x > 450 || it->position.y < 0 || it->position.y > 500) {
                it = particles.erase(it);
            } else {
                RectangleShape particleShape(Vector2f(3.f, 3.f));
                particleShape.setPosition(it->position);
                particleShape.setFillColor(it->color);
                window.draw(particleShape);
                ++it;
            }
        }
    }

    // Handle mouse clicks on the game grid
    void handleMouseClick(int x, int y) {
        Vector2i mousePos = Mouse::getPosition(window);
        Vector2f worldPos = window.mapPixelToCoords(mousePos);
        x = static_cast<int>((worldPos.x - GRID_OFFSET_X) / TILE_SIZE);
        y = static_cast<int>((worldPos.y - GRID_OFFSET_Y) / TILE_SIZE);

        if (x < 1 || x > 10 || y < 1 || y > 10) return; // Ignore out-of-bounds clicks

        if (Mouse::isButtonPressed(Mouse::Button::Left)) {
            if (visibleGrid[x][y] == 11) return; // Ignore flagged tiles
            if (grid[x][y] == 9) {
                // Hit a mine: reveal all mines and end game
                gameOver = true;
                for (int i = 1; i <= 10; i++) {
                    for (int j = 1; j <= 10; j++) {
                        if (grid[i][j] == 9) visibleGrid[i][j] = 9;
                    }
                }
                triggerShake(0.5f, 8.0f);
                triggerFlash(0.3f);
                sounds.mineSound.play();
                sounds.loseSounds[0].play();
                sounds.loseSounds[1].play();
                sounds.loseSounds[2].play();
            } else {
                revealTile(x, y);
                sounds.clickSound.play();
            }
        } else if (Mouse::isButtonPressed(Mouse::Button::Right)) {
            // Toggle flag on hidden tile
            if (visibleGrid[x][y] == 10) {
                visibleGrid[x][y] = 11;
                startTileAnimation(x, y);
                sounds.flagSound.play();
            } else if (visibleGrid[x][y] == 11) {
                visibleGrid[x][y] = 10;
                startTileAnimation(x, y);
                sounds.flagSound.play();
            }
        } else if (Mouse::isButtonPressed(Mouse::Button::Middle) && visibleGrid[x][y] < 9 && visibleGrid[x][y] >= 0) {
            // Middle-click chord: reveal adjacent tiles if correct flags placed
            int flagCount = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 1 && nx <= 10 && ny >= 1 && ny <= 10 && visibleGrid[nx][ny] == 11) {
                        flagCount++;
                    }
                }
            }
            if (flagCount == grid[x][y]) {
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 1 && nx <= 10 && ny >= 1 && ny <= 10 && visibleGrid[nx][ny] == 10) {
                            if (grid[nx][ny] == 9) {
                                // Chord hit a mine: end game
                                gameOver = true;
                                for (int i = 1; i <= 10; i++) {
                                    for (int j = 1; j <= 10; j++) {
                                        if (grid[i][j] == 9) visibleGrid[i][j] = 9;
                                    }
                                }
                                triggerShake(0.5f, 8.0f);
                                triggerFlash(0.3f);
                                sounds.mineSound.play();
                                sounds.loseSounds[0].play();
                                sounds.loseSounds[1].play();
                                sounds.loseSounds[2].play();
                                return;
                            }
                            revealTile(nx, ny);
                        }
                    }
                }
                sounds.clickSound.play();
            }
        }
    }

    // Check if all non-mine tiles are revealed to win
    void checkWinCondition() {
        if (gameOver || gameWon) return;

        bool allNonMinesRevealed = true;
        for (int i = 1; i <= 10; i++) {
            for (int j = 1; j <= 10; j++) {
                if (grid[i][j] != 9 && (visibleGrid[i][j] == 10 || visibleGrid[i][j] == 11)) {
                    allNonMinesRevealed = false;
                    break;
                }
            }
            if (!allNonMinesRevealed) break;
        }

        if (allNonMinesRevealed) {
            gameWon = true;
            updateHighScore();
            sounds.winSound.play();
            spawnWinParticles();
        }
    }

    // Save current game state to file
    void saveGame() {
        if (totalMines == 0) {
            showStatusMessage("No active game to save", Color::Red);
            cerr << "Save failed: No active game (totalMines = 0)" << endl;
            return;
        }

        ofstream file("savegame.txt");
        if (!file.is_open()) {
            showStatusMessage("Error: Could not save game", Color::Red);
            cerr << "Save failed: Could not open savegame.txt for writing" << endl;
            return;
        }

        // Save mine probability, game state, grid, score, and time
        file << MINE_PROBABILITY << "\n";
        file << (gameOver ? 1 : 0) << " " << (gameWon ? 1 : 0) << " " << hintsAvailable << "\n";
        for (int i = 1; i <= 10; i++) {
            for (int j = 1; j <= 10; j++) {
                file << grid[i][j] << " " << visibleGrid[i][j] << " ";
            }
            file << "\n";
        }
        file << score << " " << (gameClock.getElapsedTime().asSeconds() + timeOffset) << "\n";

        if (file.fail()) {
            showStatusMessage("Error: Failed to write save data", Color::Red);
            cerr << "Save failed: Failed to write to savegame.txt" << endl;
            file.close();
            return;
        }

        file.close();
        showStatusMessage("Game saved successfully", Color::Green);
        cerr << "Save successful: savegame.txt written" << endl;
    }

    // Load game state from file
    bool loadGame() {
        if (!fileExists("savegame.txt")) {
            showStatusMessage("No saved games", Color::Red);
            cerr << "Load failed: savegame.txt does not exist" << endl;
            return false;
        }

        ifstream file("savegame.txt");
        if (!file.is_open()) {
            showStatusMessage("Error: Could not open save file", Color::Red);
            cerr << "Load failed: Could not open savegame.txt for reading" << endl;
            return false;
        }

        // Read mine probability
        int mineProb;
        if (!(file >> mineProb) || mineProb < 0 || mineProb > 100) {
            showStatusMessage("Error: Invalid save file format", Color::Red);
            cerr << "Load failed: Invalid MINE_PROBABILITY (" << mineProb << ")" << endl;
            file.close();
            return false;
        }
        MINE_PROBABILITY = mineProb;

        // Read game state and hints
        int go, gw, hints;
        if (!(file >> go >> gw >> hints) || (go != 0 && go != 1) || (gw != 0 && gw != 1) || hints < 0) {
            showStatusMessage("Error: Invalid game state in save file", Color::Red);
            cerr << "Load failed: Invalid gameOver (" << go << "), gameWon (" << gw << "), or hints (" << hints << ")" << endl;
            file.close();
            return false;
        }
        gameOver = go;
        gameWon = gw;
        hintsAvailable = hints;

        // Read grid data
        totalMines = 0;
        for (int i = 1; i <= 10; i++) {
            for (int j = 1; j <= 10; j++) {
                int g, vg;
                if (!(file >> g >> vg) || 
                    g < 0 || g > 9 || 
                    (vg < 0 || (vg > 11 && vg != 9))) {
                    showStatusMessage("Error: Invalid grid data in save file", Color::Red);
                    cerr << "Load failed: Invalid grid[" << i << "][" << j << "] = " << g << ", visibleGrid = " << vg << endl;
                    file.close();
                    return false;
                }
                grid[i][j] = g;
                visibleGrid[i][j] = vg;
                if (g == 9) totalMines++;
            }
        }

        // Read score and elapsed time
        float elapsed;
        if (!(file >> score >> elapsed) || score < 0 || elapsed < 0) {
            showStatusMessage("Error: Invalid score or time in save file", Color::Red);
            cerr << "Load failed: Invalid score (" << score << ") or elapsed time (" << elapsed << ")" << endl;
            file.close();
            return false;
        }
        timeOffset = elapsed;
        gameClock.restart();
        minesText.setString("Mines: " + to_string(totalMines));
        hintsText.setString("Hints: " + to_string(hintsAvailable));
        file.close();

        // Reset visual effects
        shakeActive = false;
        flashDuration = 0.f;
        flashOverlay.setFillColor(Color(255, 255, 255, 0));
        View view = window.getView();
        view.setCenter(originalViewCenter);
        window.setView(view);

        showStatusMessage("Game loaded successfully", Color::Green);
        sounds.startSound.play();
        cerr << "Load successful: MINE_PROBABILITY = " << MINE_PROBABILITY << ", score = " << score << ", time = " << elapsed << ", hints = " << hintsAvailable << endl;
        return true;
    }

    // Update high scores with current game score
    void updateHighScore() {
        vector<pair<int, float>> scores;
        ifstream in("highscores.txt");
        int s;
        float t;
        while (in >> s >> t) {
            scores.emplace_back(s, t);
        }
        in.close();
        scores.emplace_back(score, elapsedTime.asSeconds() + timeOffset);
        // Sort by score (descending), then time (ascending)
        sort(scores.rbegin(), scores.rend(), [](const auto& a, const auto& b) {
            return a.first == b.first ? a.second < b.second : a.first > b.first;
        });
        // Save top 5 scores
        ofstream out("highscores.txt");
        for (int i = 0; i < min(5, (int)scores.size()); i++) {
            out << scores[i].first << " " << scores[i].second << "\n";
        }
        out.close();
        updateHighScoresText();
    }

    // Update high scores text for display
    void updateHighScoresText() {
        ifstream file("highscores.txt");
        string text = "HIGH SCORES:\n";
        int s;
        float t;
        int rank = 1;
        while (file >> s >> t && rank <= 5) {
            text += to_string(rank) + ". Score: " + to_string(s) + ", Time: " + to_string((int)t) + "s\n";
            rank++;
        }
        file.close();
        highScoresText.setString(text);
    }

    // Check if mouse is over a specific tile
    bool isMouseOverTile(int x, int y) {
        Vector2i mousePos = Mouse::getPosition(window);
        Vector2f worldPos = window.mapPixelToCoords(mousePos);
        float adjustedX = x * TILE_SIZE + GRID_OFFSET_X;
        float adjustedY = y * TILE_SIZE + GRID_OFFSET_Y;
        return worldPos.x >= adjustedX && worldPos.x < adjustedX + TILE_SIZE &&
               worldPos.y >= adjustedY && worldPos.y < adjustedY + TILE_SIZE;
    }

    // Get menu item under mouse cursor
    int getMenuItemAtMouse() {
        Vector2i mousePos = Mouse::getPosition(window);
        Vector2f worldPos = window.mapPixelToCoords(mousePos);
        vector<Text>* items = (currentState == MENU) ? &menuItems : (currentState == SETTINGS) ? &settingsItems : &pauseItems;
        const float startY = 150.f;
        const float itemHeight = 50.f;
        const float itemWidth = 150.f;

        for (size_t i = 0; i < items->size(); i++) {
            float y = startY + i * 60.f;
            float x = 150.f;
            if (worldPos.x >= x && worldPos.x < x + itemWidth &&
                worldPos.y >= y && worldPos.y < y + itemHeight) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    // Get difficulty item under mouse cursor
    int getDifficultyItemAtMouse() {
        Vector2i mousePos = Mouse::getPosition(window);
        Vector2f worldPos = window.mapPixelToCoords(mousePos);
        const float startY = 100.f;
        const float itemHeight = 50.f;
        const float itemWidth = 200.f;

        for (size_t i = 0; i < difficultyItems.size(); i++) {
            float y = startY + i * 50.f;
            float x = 100.f;
            if (worldPos.x >= x && worldPos.x < x + itemWidth &&
                worldPos.y >= y && worldPos.y < y + itemHeight) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    // Activate selected menu item based on current state
    void activateMenuItem(int itemIndex) {
        if (currentState == MENU) {
            switch (itemIndex) {
                case 0: sounds.menuMusic.pause(); startTransition(PLAYING); reset(); break; // Start new game
                case 1: startTransition(DIFFICULTY); break; // Open difficulty menu
                case 2: startTransition(SETTINGS); break; // Open settings menu
                case 3: sounds.menuMusic.stop(); window.close(); break; // Exit game
            }
        } else if (currentState == SETTINGS) {
            switch (itemIndex) {
                case 0: startTransition(CONTROLS); break; // Show controls
                case 1: startTransition(CREDITS); break; // Show credits
                case 2: startTransition(HIGH_SCORES); updateHighScoresText(); break; // Show high scores
                case 3: startTransition(MENU); break; // Return to main menu
            }
        } else if (currentState == PAUSE) {
            switch (itemIndex) {
                case 0: startTransition(PLAYING); break; // Resume game
                case 1: saveGame(); break; // Save current game
                case 2: if (loadGame()) startTransition(PLAYING); break; // Load saved game
                case 3: provideHint(); break; // Use a hint
                case 4: startTransition(MENU); break; // Return to main menu
            }
        }
    }

    // Activate selected difficulty item
    void activateDifficultyItem(int itemIndex) {
        switch (itemIndex) {
            case 0:
                MINE_PROBABILITY = 10;
                difficultyConfirmText.setString("Easy difficulty selected!");
                break;
            case 1:
                MINE_PROBABILITY = 15;
                difficultyConfirmText.setString("Medium difficulty selected!");
                break;
            case 2:
                MINE_PROBABILITY = 20;
                difficultyConfirmText.setString("Hard difficulty selected!");
                break;
            default:
                return;
        }
        showingConfirm = true;
        confirmClock.restart();
        sounds.clickSound.play();
    }

    // Draw the game grid and UI
    void drawGrid() {
        window.clear();
        drawGameBackground(); // Draw solid color background
        window.draw(background); // Existing background rectangle

        // Draw glow effect for hovered tiles
        for (int i = 1; i <= 10; i++) {
            for (int j = 1; j <= 10; j++) {
                if (isMouseOverTile(i, j)) {
                    RectangleShape glow(Vector2f(TILE_SIZE + 4, TILE_SIZE + 4));
                    glow.setPosition(Vector2f(
                        i * TILE_SIZE + GRID_OFFSET_X - 2,
                        j * TILE_SIZE + GRID_OFFSET_Y - 2
                    ));
                    glow.setFillColor(Color(255, 255, 0, 100)); // Yellow semi-transparent glow
                    window.draw(glow);
                }

                // Set tile color based on state
                if (visibleGrid[i][j] == 11 && grid[i][j] != 9 && gameOver) {
                    tileSprite.setColor(Color::Red); // Wrong flag
                } else if (visibleGrid[i][j] == 9) {
                    tileSprite.setColor(Color::Yellow); // Mine
                } else {
                    tileSprite.setColor(Color(200, 200, 200)); // Normal
                }

                // Set texture for tile (number, mine, flag, or hidden)
                int textureIndex = (visibleGrid[i][j] == 9) ? 9 : visibleGrid[i][j];
                tileSprite.setTextureRect(IntRect(
                    Vector2i(textureIndex * TILE_SIZE, 0),
                    Vector2i(TILE_SIZE, TILE_SIZE)
                ));

                // Apply animation if active
                if (tileAnimations[i][j].active) {
                    tileAnimations[i][j].timer += 1.0f / 60.0f;
                    float t = tileAnimations[i][j].timer / tileAnimationDuration;
                    if (t >= 1.0f) {
                        tileAnimations[i][j].active = false;
                        tileAnimations[i][j].scale = 1.0f;
                    } else {
                        if (visibleGrid[i][j] == 11 || visibleGrid[i][j] == 10) {
                            tileAnimations[i][j].scale = 1.0f + 0.2f * sin(t * 3.14159f); // Pulse for flags/hidden
                        } else {
                            tileAnimations[i][j].scale = t; // Grow for revealed
                        }
                    }
                    tileSprite.setScale(Vector2f(tileAnimations[i][j].scale, tileAnimations[i][j].scale));
                } else {
                    tileSprite.setScale(Vector2f(1.0f, 1.0f));
                }

                // Center sprite for scaling
                tileSprite.setPosition(Vector2f(
                    i * TILE_SIZE + GRID_OFFSET_X + (TILE_SIZE * (1.0f - tileAnimations[i][j].scale)) / 2,
                    j * TILE_SIZE + GRID_OFFSET_Y + (TILE_SIZE * (1.0f - tileAnimations[i][j].scale)) / 2
                ));
                window.draw(tileSprite);
            }
        }

        // Draw UI text
        scoreText.setString("Score: " + to_string(score));
        scoreText.setPosition(Vector2f(10.f, 10.f));
        window.draw(scoreText);

        minesText.setString("Mines: " + to_string(countRemainingMines()));
        minesText.setPosition(Vector2f(120.f, 10.f));
        window.draw(minesText);

        hintsText.setString("Hints: " + to_string(hintsAvailable));
        hintsText.setPosition(Vector2f(230.f, 10.f));
        window.draw(hintsText);

        elapsedTime = gameClock.getElapsedTime();
        timerText.setString("Time: " + to_string((int)(elapsedTime.asSeconds() + timeOffset)));
        timerText.setPosition(Vector2f(340.f, 10.f));
        window.draw(timerText);

        // Draw game over or win messages
        if (gameOver) {
            gameOverText.setPosition(Vector2f(20.f, 400.f));
            window.draw(gameOverText);
            restartText.setPosition(Vector2f(20.f, 440.f));
            window.draw(restartText);
        }
        if (gameWon) {
            winText.setPosition(Vector2f(20.f, 400.f));
            window.draw(winText);
            restartText.setPosition(Vector2f(20.f, 440.f));
            window.draw(restartText);

            // Draw win particles
            for (auto it = particles.begin(); it != particles.end();) {
                it->position += it->velocity;
                it->lifetime -= 1.0f / 60.0f;
                if (it->lifetime <= 0) {
                    it = particles.erase(it);
                } else {
                    RectangleShape particleShape(Vector2f(5.f, 5.f));
                    particleShape.setPosition(it->position);
                    particleShape.setFillColor(it->color);
                    window.draw(particleShape);
                    ++it;
                }
            }
        }

        // Draw flash and status message in default view
        window.setView(window.getDefaultView());
        if (flashDuration > 0) {
            window.draw(flashOverlay);
        }
        if (showingStatusMessage && statusMessageClock.getElapsedTime().asSeconds() < statusMessageDuration) {
            window.draw(statusMessage);
        } else {
            showingStatusMessage = false;
        }
        window.setView(window.getView());

        window.display();
    }

    // Draw menu screens (main, settings, pause)
    void drawMenu() {
        window.clear();
        drawMenuBackground(); // Draw gradient with particles

        // Draw semi-transparent menu background
        RectangleShape bg(Vector2f(350.f, 300.f));
        bg.setPosition(Vector2f(50.f, 100.f));
        bg.setFillColor(Color(0, 0, 0, 100));
        window.draw(bg);

        // Select appropriate menu items and title
        vector<Text>* items = nullptr;
        float startY = 150.f;
        string titleText;
        if (currentState == MENU) {
            items = &menuItems;
            titleText = "MINESWEEPER";
        } else if (currentState == SETTINGS) {
            items = &settingsItems;
            titleText = "SETTINGS";
        } else if (currentState == PAUSE) {
            items = &pauseItems;
            titleText = "PAUSED";
        }

        Text title(font, titleText, 36);
        title.setFillColor(currentState == MENU ? Color::Red : Color::Yellow);
        title.setPosition(Vector2f(100.f, 80.f));
        window.draw(title);

        // Update selection based on mouse hover
        int hoveredItem = getMenuItemAtMouse();
        static int lastHoveredItem = -1;
        if (hoveredItem != -1 && hoveredItem != selectedMenuItem) {
            selectedMenuItem = hoveredItem;
            sounds.clickSound.play();
        } else if (hoveredItem == -1 && lastHoveredItem != -1) {
            // Keep last selection
        }
        lastHoveredItem = hoveredItem;

        // Draw menu items with animations
        for (size_t i = 0; i < items->size(); i++) {
            (*items)[i].setFillColor(i == selectedMenuItem ? Color::Yellow : Color::White);
            float scale = (i == selectedMenuItem) ? menuAnimations[i].scale : 1.0f;
            (*items)[i].setScale(Vector2f(scale, scale));
            (*items)[i].setPosition(Vector2f(
                150.f + (30.f * (1.0f - scale)),
                startY + i * 60.f
            ));
            window.draw((*items)[i]);
            (*items)[i].setScale(Vector2f(1.0f, 1.0f));
        }

        // Draw status message if active
        if (showingStatusMessage && statusMessageClock.getElapsedTime().asSeconds() < statusMessageDuration) {
            window.draw(statusMessage);
        } else {
            showingStatusMessage = false;
        }

        window.display();
    }

    // Draw controls screen
    void drawControls() {
        window.clear();
        drawMenuBackground(); // Reuse menu background with particles

        Text title(font, "GAME CONTROLS", 30);
        title.setFillColor(Color::Yellow);
        float scale = 1.0f + 0.05f * sin(titleAnimationTimer * 2.0f); // Pulsing effect
        title.setScale(Vector2f(scale, scale));
        title.setPosition(Vector2f(50.f + (30.f * (1.0f - scale)), 30.f));
        window.draw(title);

        Text controlsText(font, "CONTROLS:\nLeft Click: Reveal tile\nRight Click: Place/remove flag\nMiddle Click: Chord (reveal adjacent)\nR: Restart game\nH: Get hint\nESC: Pause/Return to menu", 20);
        controlsText.setPosition(Vector2f(50.f, 80.f));
        window.draw(controlsText);

        Text backText(font, "Press ESC to return to menu", 20);
        backText.setPosition(Vector2f(50.f, 450.f));
        window.draw(backText);

        window.display();
    }

    // Draw credits screen
    void drawCredits() {
        window.clear();
        drawMenuBackground(); // Reuse menu background with particles

        Text title(font, "GAME CREDITS", 30);
        title.setFillColor(Color::Green);
        float scale = 1.0f + 0.05f * sin(titleAnimationTimer * 2.0f); // Pulsing effect
        title.setScale(Vector2f(scale, scale));
        title.setPosition(Vector2f(50.f + (30.f * (1.0f - scale)), 30.f));
        window.draw(title);

        Text creditsText(font, "CREDITS:\nCreated by Syed Ashar Tahir\nAhmed Nadir Shah\nRESOURCES:\nSound effects from sound resources\nSfML syntax from SFML/Documentation", 20);
        creditsText.setPosition(Vector2f(50.f, 80.f));
        window.draw(creditsText);

        Text backText(font, "Press ESC to return to menu", 20);
        backText.setPosition(Vector2f(50.f, 450.f));
        window.draw(backText);

        window.display();
    }

    // Draw difficulty selection screen
    void drawDifficulty() {
        window.clear();
        drawMenuBackground(); // Reuse menu background with particles

        Text title(font, "SELECT DIFFICULTY", 30);
        title.setFillColor(Color::Yellow);
        float scale = 1.0f + 0.05f * sin(titleAnimationTimer * 2.0f); // Pulsing effect
        title.setScale(Vector2f(scale, scale));
        title.setPosition(Vector2f(50.f + (30.f * (1.0f - scale)), 30.f));
        window.draw(title);

        // Update selection based on mouse hover
        int hoveredItem = getDifficultyItemAtMouse();
        static int lastHoveredItem = -1;
        if (hoveredItem != -1 && hoveredItem != selectedMenuItem) {
            selectedMenuItem = hoveredItem;
            sounds.clickSound.play();
        }
        lastHoveredItem = hoveredItem;

        // Draw difficulty items with animations
        for (size_t i = 0; i < difficultyItems.size(); i++) {
            int animationIndex = 13 + i; // Menu animations: 0-3 Main, 4-7 Settings, 8-12 Pause, 13-15 Difficulty
            difficultyItems[i].setFillColor(
                (MINE_PROBABILITY == (10 + i * 5)) ? Color::Green :
                (i == selectedMenuItem) ? Color::Yellow : Color::White
            );
            float scale = (i == selectedMenuItem) ? menuAnimations[animationIndex].scale : 1.0f;
            difficultyItems[i].setScale(Vector2f(scale, scale));
            difficultyItems[i].setPosition(Vector2f(
                100.f + (30.f * (1.0f - scale)),
                100.f + i * 50.f
            ));
            window.draw(difficultyItems[i]);
            difficultyItems[i].setScale(Vector2f(1.0f, 1.0f));
        }

        // Draw confirmation message if active
        if (showingConfirm && confirmClock.getElapsedTime().asSeconds() < confirmDuration) {
            difficultyConfirmText.setPosition(Vector2f(100.f, 250.f));
            window.draw(difficultyConfirmText);
        } else {
            showingConfirm = false;
        }

        Text backText(font, "Press ESC to return to menu", 20);
        backText.setPosition(Vector2f(50.f, 450.f));
        window.draw(backText);

        window.display();
    }

    // Draw high scores screen
    void drawHighScores() {
        window.clear();
        drawMenuBackground(); // Reuse menu background with particles

        Text title(font, "HIGH SCORES", 30);
        title.setFillColor(Color::Yellow);
        float scale = 1.0f + 0.05f * sin(titleAnimationTimer * 2.0f); // Pulsing effect
        title.setScale(Vector2f(scale, scale));
        title.setPosition(Vector2f(50.f + (30.f * (1.0f - scale)), 30.f));
        window.draw(title);

        highScoresText.setPosition(Vector2f(50.f, 80.f));
        window.draw(highScoresText);

        Text backText(font, "Press ESC to return to menu", 20);
        backText.setPosition(Vector2f(50.f, 450.f));
        window.draw(backText);

        window.display();
    }

    // Reset game to initial state
    void reset() {
        initializeGrid();
        gameOver = false;
        gameWon = false;
        score = 0;
        shakeActive = false;
        shakeTime = 0.f;
        flashDuration = 0.f;
        flashOverlay.setFillColor(Color(255, 255, 255, 0));
        gameClock.restart();
        timeOffset = 0.f;
        hintsAvailable = 3;
        hintsText.setString("Hints: " + to_string(hintsAvailable));
        sounds.startSound.play();
        for (int i = 1; i <= 10; i++) {
            for (int j = 1; j <= 10; j++) {
                tileAnimations[i][j] = TileAnimation();
            }
        }
        particles.clear();

        View view = window.getView();
        view.setCenter(originalViewCenter);
        window.setView(view);
    }

    // Handle keyboard input for menu navigation
    void handleMenuInput() {
        static Clock keyCooldown;
        const Time cooldownTime = milliseconds(100);

        if (keyCooldown.getElapsedTime() < cooldownTime) {
            return;
        }

        vector<Text>* items = (currentState == MENU) ? &menuItems : (currentState == SETTINGS) ? &settingsItems : &pauseItems;
        if (Keyboard::isKeyPressed(Keyboard::Scan::Up)) {
            selectedMenuItem = (selectedMenuItem - 1 + items->size()) % items->size();
            keyCooldown.restart();
            sounds.clickSound.play();
        }
        if (Keyboard::isKeyPressed(Keyboard::Scan::Down)) {
            selectedMenuItem = (selectedMenuItem + 1) % items->size();
            keyCooldown.restart();
            sounds.clickSound.play();
        }
        if (Keyboard::isKeyPressed(Keyboard::Scan::Enter)) {
            keyCooldown.restart();
            sounds.clickSound.play();
            activateMenuItem(selectedMenuItem);
        }
        if (Keyboard::isKeyPressed(Keyboard::Scan::Escape) && currentState == PAUSE) {
            startTransition(PLAYING);
            keyCooldown.restart();
            sounds.clickSound.play();
        }
    }

    // Handle keyboard input for difficulty selection
    void handleDifficultyInput() {
        static Clock keyCooldown;
        const Time cooldownTime = milliseconds(200);

        if (keyCooldown.getElapsedTime() < cooldownTime) return;

        if (Keyboard::isKeyPressed(Keyboard::Scan::Num1)) {
            activateDifficultyItem(0);
            keyCooldown.restart();
        } 
        else if (Keyboard::isKeyPressed(Keyboard::Scan::Num2)) {
            activateDifficultyItem(1);
            keyCooldown.restart();
        } 
        else if (Keyboard::isKeyPressed(Keyboard::Scan::Num3)) {
            activateDifficultyItem(2);
            keyCooldown.restart();
        }
    }

public:
    // Constructor: initialize game resources and UI
    MinesweeperGame() : window(VideoMode({450, 500}), "Minesweeper"),
                        tileSprite(tileTexture)
    {
        srand(static_cast<unsigned int>(time(0))); // Seed random number generator
        window.setView(View(FloatRect(Vector2f(0.f, 0.f), Vector2f(450.f, 500.f)))); // Set default view
        window.setFramerateLimit(60); // Cap at 60 FPS

        // Load essential resources
        if (!tileTexture.loadFromFile("images/tiles.jpg") || 
            !font.openFromFile("fonts/arial.ttf")) {
            throw runtime_error("Failed to load critical textures or font");
        }

        // Initialize main menu items
        vector<string> mainItems = {"Play", "Difficulty", "Settings", "Exit"};
        for (const auto& item : mainItems) {
            Text text(font, item, 24);
            menuItems.push_back(text);
            menuAnimations.push_back(MenuAnimation());
        }

        // Initialize settings menu items
        vector<string> settings = {"Controls", "Credits", "High Scores", "Back"};
        for (const auto& item : settings) {
            Text text(font, item, 24);
            settingsItems.push_back(text);
            menuAnimations.push_back(MenuAnimation());
        }

        // Initialize pause menu items
        vector<string> pause = {"Resume", "Save Game", "Load Game", "Get Hint", "Main Menu"};
        for (const auto& item : pause) {
            Text text(font, item, 24);
            pauseItems.push_back(text);
            menuAnimations.push_back(MenuAnimation());
        }

        // Initialize difficulty menu items
        vector<string> difficulties = {"1: Easy (10% mines)", "2: Medium (15% mines)", "3: Hard (20% mines)"};
        for (const auto& item : difficulties) {
            Text text(font, item, 25);
            difficultyItems.push_back(text);
            menuAnimations.push_back(MenuAnimation());
        }

        // Initialize overlays
        flashOverlay.setSize(Vector2f(450.f, 500.f));
        flashOverlay.setFillColor(Color(255, 255, 255, 0));
        flashOverlay.setPosition(Vector2f(0.f, 0.f));
        transitionOverlay.setSize(Vector2f(450.f, 500.f));
        transitionOverlay.setFillColor(Color(0, 0, 0, 0));
        transitionOverlay.setPosition(Vector2f(0.f, 0.f));
        originalViewCenter = Vector2f(GRID_OFFSET_X + 160.f, GRID_OFFSET_Y + 160.f); // Center of 10x10 grid

        // Initialize background
        background.setSize(Vector2f(466.f, 516.f)); // Slightly larger for shake effect
        background.setPosition(Vector2f(-8.f, -8.f));
        background.setFillColor(Color(200, 200, 200)); // Solid gray color

        // Set text properties
        gameOverText.setFillColor(Color::Black);
        gameOverText.setPosition(Vector2f(150.f, 400.f));
        winText.setFillColor(Color::Green);
        winText.setPosition(Vector2f(150.f, 400.f));
        scoreText.setFillColor(Color::Black);
        timerText.setFillColor(Color::Black);
        minesText.setFillColor(Color::Black);
        hintsText.setFillColor(Color::Black);
        restartText.setFillColor(Color::Black);
        restartText.setPosition(Vector2f(150.f, 440.f));
        statusMessage.setFillColor(Color::White);
        statusMessage.setPosition(Vector2f(100.f, 420.f));
        sounds.loadSounds();
        difficultyConfirmText.setFillColor(Color::Green);
        difficultyConfirmText.setPosition(Vector2f(100.f, 250.f));
        updateHighScoresText();

        // Log grid bounds for debugging
        cerr << "Grid bounds: x=[" << GRID_OFFSET_X << ", " << GRID_OFFSET_X + 320 << "], y=[" << GRID_OFFSET_Y << ", " << GRID_OFFSET_Y + 320 << "]" << endl;
    };

    // Main game loop
    void run() {
        // Start menu music if in a menu state
        if (currentState == MENU || currentState == SETTINGS || currentState == PAUSE || 
            currentState == DIFFICULTY || currentState == CONTROLS || currentState == CREDITS || 
            currentState == HIGH_SCORES) {
            if (sounds.menuMusic.getStatus() != sf::SoundSource::Status::Playing) {
                sounds.menuMusic.play();
            }
        }

        static Clock keyCooldown;
        const Time cooldownTime = milliseconds(200); // Cooldown for mouse/keyboard input

        while (window.isOpen()) {
            // Process SFML events
            optional<Event> event = window.pollEvent();
            while (event.has_value()) {
                const auto& e = event.value();
                if (e.is<Event::Closed>()) {
                    sounds.menuMusic.stop();
                    window.close();
                }
                if (currentState == PLAYING && !gameOver && !gameWon && e.is<Event::MouseButtonPressed>()) {
                    handleMouseClick(0, 0);
                }
                if (auto* mouseEvent = e.getIf<Event::MouseButtonPressed>()) {
                    if (mouseEvent->button == Mouse::Button::Left && 
                        (currentState == MENU || currentState == SETTINGS || currentState == PAUSE || currentState == DIFFICULTY) && 
                        keyCooldown.getElapsedTime() >= cooldownTime) {
                        int itemIndex = (currentState == DIFFICULTY) ? getDifficultyItemAtMouse() : getMenuItemAtMouse();
                        if (itemIndex != -1) {
                            selectedMenuItem = itemIndex;
                            if (currentState == DIFFICULTY) {
                                activateDifficultyItem(itemIndex);
                            } else {
                                activateMenuItem(itemIndex);
                            }
                            sounds.clickSound.play();
                            keyCooldown.restart();
                        }
                    }
                }
                if (auto* keyEvent = e.getIf<Event::KeyReleased>()) {
                    if (keyEvent->scancode == Keyboard::Scan::R && currentState == PLAYING) {
                        reset();
                    }
                    if (keyEvent->scancode == Keyboard::Scan::H && currentState == PLAYING) {
                        provideHint();
                    }
                    if (keyEvent->scancode == Keyboard::Scan::Escape) {
                        if (currentState == PLAYING) {
                            startTransition(PAUSE);
                            keyCooldown.restart();
                            sounds.clickSound.play();
                        } else if (currentState != MENU && currentState != PAUSE) {
                            startTransition(MENU);
                        }
                    }
                }
                event = window.pollEvent();
            }

            // Handle state transitions
            if (inTransition) {
                transitionAlpha = flashClock.getElapsedTime().asSeconds() / transitionDuration;
                if (transitionAlpha >= 1.0f) {
                    currentState = targetState;
                    inTransition = false;
                    transitionAlpha = 1.0f - transitionAlpha;
                }
                transitionOverlay.setFillColor(Color(0, 0, 0, static_cast<int>(255 * transitionAlpha)));
                window.setView(window.getDefaultView());
                window.draw(transitionOverlay);
                window.setView(window.getView());
            }

            // Update and render based on current state
            switch (currentState) {
                case MENU: updateEffects(); handleMenuInput(); drawMenu(); break;
                case PLAYING: updateEffects(); checkWinCondition(); drawGrid(); break;
                case CONTROLS: updateEffects(); drawControls(); break;
                case CREDITS: updateEffects(); drawCredits(); break;
                case DIFFICULTY: updateEffects(); handleDifficultyInput(); drawDifficulty(); break;
                case HIGH_SCORES: updateEffects(); drawHighScores(); break;
                case SETTINGS: updateEffects(); handleMenuInput(); drawMenu(); break;
                case PAUSE: updateEffects(); handleMenuInput(); drawMenu(); break;
            }
        }
    }
};

// Program entry point
int main() {
    try {
        MinesweeperGame game;
        game.run();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return -1;
    }
    return 0;
}
