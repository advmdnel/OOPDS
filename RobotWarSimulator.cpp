/**********|**********|**********|
Program: RobotWarSimulator.cpp
Course: Object-Oriented Programming and Data Structures
Trimester: 2510
Name: Adam Daniel Ali Bin Shamsul Azhar
ID: 243UC246GF
Lecture Section: TC1L
Tutorial Section: TT3L
Email: ADAM.DANIEL.ALI@student.mmu.edu.my
Phone: 012-9780090
**********|**********|**********/

#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <thread>
#include <chrono>
#include <limits>

const int MAP_HEIGHT = 10;
const int MAP_WIDTH = 20;
const int LONGSHOT_RANGE = 3;
const std::string FLOOR = ".";

enum Direction { NORTH, EAST, SOUTH, WEST };

// Forward declaration
class Robot;

class Roguelike {
private:
    std::vector<Robot*> robots;
    std::vector<std::vector<std::string>> map;
    int currentTurn;
    bool hasLooked, hasFired;
    int step;
    int robotCount;

    void initializeMap() { map.assign(MAP_HEIGHT, std::vector<std::string>(MAP_WIDTH, FLOOR)); }

    void placeRobot(Robot* robot);
    void respawnRobot(Robot* robot);
    void respawnAll();
    void checkSelfDestruct(Robot* robot);

public:
    bool isOccupied(int x, int y) const;
    void genericFire(Robot* shooter, int targetX, int targetY, int range);
    void look(Robot* robot);
    void scoutFor(Robot* scout);
    void moveRobot(Robot* robot, int newX, int newY, bool isJump = false);
    void offerUpgrade(Robot* robot);
    void logMove(const std::string& action, int fromX, int fromY, int toX, int toY, const std::string& additionalLog = "");
    std::string dirToString(Direction dir);
    void advanceTurn();

    bool getFireTarget(int robotX, int robotY, int dxForward, int dxLeft, int dyForward, int dyLeft, int& targetX, int& targetY);

public:
    Roguelike();
    ~Roguelike();
    void display();
    void moveRobot(char input);
    bool isGameOver() const;
    std::string getWinner() const;
};

// Robot class and subclasses
class Robot {
protected:
    int x, y, lives, shells, jumpCount, scoutCount;
    Direction dir;
    std::string name;
    std::string symbols[4]; // Symbols for each direction (colored)

public:
    Robot(int x_, int y_, const std::string& name_, const std::string symbol[4])
        : x(x_), y(y_), lives(3), shells(10), jumpCount(0), scoutCount(0), dir(NORTH), name(name_) {
        for (int i = 0; i < 4; ++i) symbols[i] = symbol[i];
    }
    virtual ~Robot() = default;

    // Getters
    int getX() const { return x; }
    int getY() const { return y; }
    int getLives() const { return lives; }
    int getShells() const { return shells; }
    Direction getDir() const { return dir; }
    std::string getSymbol() const { return symbols[dir]; }
    std::string getName() const { return name; }
    bool isAlive() const { return lives > 0; }

    // Setters
    void setPosition(int x_, int y_) { x = x_; y = y_; }
    void setDir(Direction d) { dir = d; }
    void reduceLives() { lives--; }
    void reduceShells() { shells--; }
    void setLives(int l) { lives = l; }

    // Pure virtual functions
    virtual void specialAction(Roguelike& game) = 0;
    virtual void fire(int targetX, int targetY, Roguelike& game) = 0;
    virtual void move(char input, Roguelike& game) = 0;
    virtual std::string getType() const = 0;

    // Operator overloading
    virtual Robot& operator+(const std::pair<int, int>& delta) = 0;
    friend std::ostream& operator<<(std::ostream& os, const Robot& robot);
};

class GenericRobot : public Robot {
public:
    GenericRobot(int x_, int y_, const std::string& name_, const std::string symbol[4])
        : Robot(x_, y_, name_, symbol) {}
    void specialAction(Roguelike& game) override {}
    void fire(int targetX, int targetY, Roguelike& game) override {
        game.genericFire(this, targetX, targetY, 1);
    }
    void move(char input, Roguelike& game) override;
    std::string getType() const override { return "GenericRobot"; }
    Robot& operator+(const std::pair<int, int>& delta) override {
        x += delta.first;
        y += delta.second;
        return *this;
    }
};

class JumpBot : public Robot {
public:
    JumpBot(int x_, int y_, const std::string& name_, const std::string symbol[4])
        : Robot(x_, y_, name_, symbol) {
        jumpCount = 3;
    }
    void specialAction(Roguelike& game) override;
    void fire(int targetX, int targetY, Roguelike& game) override {
        game.genericFire(this, targetX, targetY, 1);
    }
    void move(char input, Roguelike& game) override;
    std::string getType() const override { return "JumpBot"; }
    Robot& operator+(const std::pair<int, int>& delta) override {
        x += delta.first;
        y += delta.second;
        return *this;
    }
};

class LongShotBot : public Robot {
public:
    LongShotBot(int x_, int y_, const std::string& name_, const std::string symbol[4])
        : Robot(x_, y_, name_, symbol) {}
    void specialAction(Roguelike& game) override {}
    void fire(int targetX, int targetY, Roguelike& game) override {
        game.genericFire(this, targetX, targetY, LONGSHOT_RANGE);
    }
    void move(char input, Roguelike& game) override;
    std::string getType() const override { return "LongShotBot"; }
    Robot& operator+(const std::pair<int, int>& delta) override {
        x += delta.first;
        y += delta.second;
        return *this;
    }
};

class ScoutBot : public Robot {
public:
    ScoutBot(int x_, int y_, const std::string& name_, const std::string symbol[4])
        : Robot(x_, y_, name_, symbol) {
        scoutCount = 3;
    }
    void specialAction(Roguelike& game) override;
    void fire(int targetX, int targetY, Roguelike& game) override {
        game.genericFire(this, targetX, targetY, 1);
    }
    void move(char input, Roguelike& game) override;
    std::string getType() const override { return "ScoutBot"; }
    Robot& operator+(const std::pair<int, int>& delta) override {
        x += delta.first;
        y += delta.second;
        return *this;
    }
};

// Implementations of Roguelike methods
void Roguelike::placeRobot(Robot* robot) {
    int x, y, attempts = 0, maxAttempts = 100;
    do {
        x = std::rand() % MAP_WIDTH;
        y = std::rand() % MAP_HEIGHT;
        attempts++;
        if (attempts > maxAttempts) break;
    } while (map[y][x] != FLOOR || isOccupied(x, y));
    robot->setPosition(x, y);
    robot->setDir(static_cast<Direction>(std::rand() % 4));
    map[y][x] = robot->getSymbol();
}

void Roguelike::respawnRobot(Robot* robot) {
    if (!robot->isAlive()) {
        map[robot->getY()][robot->getX()] = FLOOR;
        std::cout << robot->getName() << " has been eliminated!\n";
        return;
    }
    int prevX = robot->getX(), prevY = robot->getY();
    map[prevY][prevX] = FLOOR;
    placeRobot(robot);
    robot->reduceLives();
    std::cout << robot->getName() << " respawned to (" << robot->getX() << ", " << robot->getY() << ") with " << robot->getLives() << " lives left.\n";
}

void Roguelike::respawnAll() {
    for (auto& robot : robots) {
        if (robot->isAlive()) {
            map[robot->getY()][robot->getX()] = FLOOR;
            placeRobot(robot);
        }
    }
}

void Roguelike::checkSelfDestruct(Robot* robot) {
    if (robot->getShells() <= 0) {
        map[robot->getY()][robot->getX()] = FLOOR;
        robot->setLives(0);
        std::cout << robot->getName() << " has run out of shells and self-destructed!\n";
    }
}

bool Roguelike::isOccupied(int x, int y) const {
    for (const auto& robot : robots) {
        if (robot->isAlive() && robot->getX() == x && robot->getY() == y) return true;
    }
    return false;
}

void Roguelike::genericFire(Robot* shooter, int targetX, int targetY, int range) {
    int robotX = shooter->getX(), robotY = shooter->getY();
    std::string fireLog;

    if (hasFired) { std::cout << "Fire already used this turn!\n"; return; }
    if (shooter->getShells() <= 0) {
        shooter->setLives(0);
        map[robotY][robotX] = FLOOR;
        std::cout << shooter->getName() << " has run out of shells and self-destructed!\n";
        return;
    }

    hasFired = true;
    shooter->reduceShells();

    int dx = targetX - robotX;
    int dy = targetY - robotY;
    int steps = std::max(std::abs(dx), std::abs(dy));
    int dxStep = (steps == 0) ? 0 : dx / steps;
    int dyStep = (steps == 0) ? 0 : dy / steps;
    bool hit = false;

    if (targetX < 0 || targetX >= MAP_WIDTH || targetY < 0 || targetY >= MAP_HEIGHT) {
        std::cout << shooter->getName() << " fired out of bounds at (" << targetX << ", " << targetY << ")!\n";
        fireLog += shooter->getName() + " fired out of bounds at (" + std::to_string(targetX) + ", " + std::to_string(targetY) + ")\n";
        logMove(shooter->getName() + " fired", robotX, robotY, robotX, robotY, fireLog);
        std::cout << shooter->getName() << " has " << shooter->getShells() << " shells left.\n";
        checkSelfDestruct(shooter);
        return;
    }

    for (int i = 1; i <= range && !hit; i++) {
        int checkX = robotX + i * dxStep;
        int checkY = robotY + i * dyStep;
        if (checkX < 0 || checkX >= MAP_WIDTH || checkY < 0 || checkY >= MAP_HEIGHT) {
            std::cout << shooter->getName() << " fired out of bounds at range " << i << "!\n";
            fireLog += shooter->getName() + " fired out of bounds at range " + std::to_string(i) + "\n";
            break;
        }
        for (size_t j = 0; j < robots.size(); ++j) {
            if (static_cast<int>(j) == currentTurn || !robots[j]->isAlive()) continue;
            if (checkX == robots[j]->getX() && checkY == robots[j]->getY()) {
                if (shooter->getType() == "GenericRobot" && (std::rand() % 100 >= 70)) {
                    std::cout << shooter->getName() << " fired at range " << i << " but missed.\n";
                    fireLog += shooter->getName() + " fired at range " + std::to_string(i) + " but missed\n";
                } else {
                    robots[j]->reduceLives();
                    std::cout << shooter->getName() << " fired at range " << i << " and hit " << robots[j]->getName() << "! Lives left: " << robots[j]->getLives() << "\n";
                    fireLog += shooter->getName() + " fired at range " + std::to_string(i) + " and hit " + robots[j]->getName() + "\n";
                    offerUpgrade(shooter);
                    if (robots[j]->getLives() <= 0) {
                        if (robots[j]->isAlive()) {
                            respawnRobot(robots[j]);
                        } else {
                            map[robots[j]->getY()][robots[j]->getX()] = FLOOR;
                        }
                    }
                }
                hit = true;
                break;
            }
        }
    }

    if (!hit) {
        std::cout << shooter->getName() << " fired at range " << range << " but no target found.\n";
        fireLog += shooter->getName() + " fired at range " + std::to_string(range) + " but no target found\n";
    }

    logMove(shooter->getName() + " fired", robotX, robotY, robotX, robotY, fireLog);
    std::cout << shooter->getName() << " has " << shooter->getShells() << " shells left.\n";
    checkSelfDestruct(shooter);
}

void Roguelike::look(Robot* robot) {
    int robotX = robot->getX(), robotY = robot->getY();
    std::string lookLog;

    if (hasLooked) { 
        std::cout << "Look already used this turn!\n"; 
        return; 
    }

    hasLooked = true;
    std::cout << robot->getName() << " is looking at the 8 surrounding positions:\n";

    int offsets[8][2] = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1, 0},  {1, 0},
        {-1, 1},  {0, 1},  {1, 1}
    };

    bool enemyFound = false;
    for (int i = 0; i < 8; i++) {
        int x = robotX + offsets[i][0];
        int y = robotY + offsets[i][1];
        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) continue;
        for (size_t j = 0; j < robots.size(); ++j) {
            if (static_cast<int>(j) == currentTurn || !robots[j]->isAlive()) continue;
            if (x == robots[j]->getX() && y == robots[j]->getY()) {
                std::cout << "There's enemy at (" << x << ", " << y << ")\n";
                lookLog += "There's enemy at (" + std::to_string(x) + ", " + std::to_string(y) + ")\n";
                enemyFound = true;
            }
        }
    }

    if (!enemyFound) {
        std::cout << "No enemies found in the surrounding positions.\n";
        lookLog += "No enemies found in the surrounding positions\n";
    }

    logMove(robot->getName() + " looked", robotX, robotY, robotX, robotY, lookLog);

    std::cout << "Pausing for 10 seconds to review the look results...\n";
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

void Roguelike::scoutFor(Robot* scout) {
    for (size_t i = 0; i < robots.size(); ++i) {
        if (static_cast<int>(i) != currentTurn && robots[i]->isAlive()) {
            std::cout << robots[i]->getName() << " at (" << robots[i]->getX() << ", " << robots[i]->getY() << ")\n";
        }
    }
}

void Roguelike::moveRobot(Robot* robot, int newX, int newY, bool isJump) {
    if (newX < 0 || newX >= MAP_WIDTH || newY < 0 || newY >= MAP_HEIGHT) {
        std::cout << "Out of bounds!\n";
        return;
    }
    if (!isJump && isOccupied(newX, newY)) {
        std::cout << "Cannot move to another robot's position!\n";
        return;
    }
    int oldX = robot->getX(), oldY = robot->getY();
    map[oldY][oldX] = FLOOR;
    robot->setPosition(newX, newY);
    map[newY][newX] = robot->getSymbol();
    if (isJump) {
        std::cout << robot->getName() << " jumped to (" << newX << ", " << newY << ").\n";
    } else {
        std::cout << robot->getName() << " moved to (" << newX << ", " << newY << ").\n";
    }
    logMove(robot->getName(), oldX, oldY, newX, newY);
}

void Roguelike::offerUpgrade(Robot* robot) {
    int choice = (std::rand() % 3) + 1;
    int x = robot->getX(), y = robot->getY();
    std::string name = robot->getName();
    const std::string* symbol = nullptr;
    size_t index = currentTurn;
    if (name == "robot1 (red)") symbol = new std::string[4]{"\033[31m^\033[0m", "\033[31m>\033[0m", "\033[31mv\033[0m", "\033[31m<\033[0m"};
    else if (name == "robot2 (blue)") symbol = new std::string[4]{"\033[34m^\033[0m", "\033[34m>\033[0m", "\033[34mv\033[0m", "\033[34m<\033[0m"};
    else if (name == "robot3 (green)") symbol = new std::string[4]{"\033[32m^\033[0m", "\033[32m>\033[0m", "\033[32mv\033[0m", "\033[32m<\033[0m"};
    else if (name == "robot4 (yellow)") symbol = new std::string[4]{"\033[33m^\033[0m", "\033[33m>\033[0m", "\033[33mv\033[0m", "\033[33m<\033[0m"};
    else if (name == "robot5 (magenta)") symbol = new std::string[4]{"\033[35m^\033[0m", "\033[35m>\033[0m", "\033[35mv\033[0m", "\033[35m<\033[0m"};
    else if (name == "robot6 (cyan)") symbol = new std::string[4]{"\033[36m^\033[0m", "\033[36m>\033[0m", "\033[36mv\033[0m", "\033[36m<\033[0m"};
    
    delete robots[index];
    switch (choice) {
        case 1: robots[index] = new JumpBot(x, y, name, symbol); std::cout << name << " upgraded to JumpBot!\n"; break;
        case 2: robots[index] = new LongShotBot(x, y, name, symbol); std::cout << name << " upgraded to LongShotBot!\n"; break;
        case 3: robots[index] = new ScoutBot(x, y, name, symbol); std::cout << name << " upgraded to ScoutBot!\n"; break;
    }
    map[y][x] = robots[index]->getSymbol();
}

void Roguelike::logMove(const std::string& action, int fromX, int fromY, int toX, int toY, const std::string& additionalLog) {
    std::ofstream logFile("game_log.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << "Step: " << step << "\n";
        for (const auto& robot : robots) {
            if (robot->isAlive()) {
                logFile << robot->getType() << " " << robot->getName() << " (" << robot->getX() << ", " << robot->getY() << ")\n";
            } else {
                logFile << "Dead " << robot->getName() << "\n";
            }
        }
        logFile << action;
        if (fromX != toX || fromY != toY) {
            logFile << " moved from (" << fromX << ", " << fromY << ") to (" << toX << ", " << toY << ")\n";
        } else {
            logFile << "\n";
        }
        if (!additionalLog.empty()) logFile << additionalLog;
        logFile << "\n";
        logFile.close();
    }
}

std::string Roguelike::dirToString(Direction dir) {
    return dir == NORTH ? "North" : dir == EAST ? "East" : dir == SOUTH ? "South" : "West";
}

void Roguelike::advanceTurn() {
    int initialTurn = currentTurn;
    do {
        currentTurn = (currentTurn + 1) % robots.size();
        if (!robots[currentTurn]->isAlive()) continue;
        break;
    } while (currentTurn != initialTurn);
}

bool Roguelike::getFireTarget(int robotX, int robotY, int dxForward, int dxLeft, int dyForward, int dyLeft, int& targetX, int& targetY) {
    std::cout << "Enter Location (AQWESDZC): ";
    std::string targetInput;
    std::getline(std::cin, targetInput);

    if (targetInput.length() != 1 || std::cin.fail()) {
        std::cout << "Invalid location! Please use AQWESDZC.\nEnter move: ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return false;
    }

    char targetDir = std::toupper(targetInput[0]);
    targetX = robotX;
    targetY = robotY;

    switch (targetDir) {
        case 'A': targetX += dxLeft; targetY += dyLeft; break;
        case 'Q': targetX += dxForward + dxLeft; targetY += dyForward + dyLeft; break;
        case 'W': targetX += dxForward; targetY += dyForward; break;
        case 'E': targetX += dxForward - dxLeft; targetY += dyForward - dyLeft; break;
        case 'D': targetX -= dxLeft; targetY -= dyLeft; break;
        case 'C': targetX -= dxForward - dxLeft; targetY -= dyForward - dyLeft; break;
        case 'Z': targetX -= dxForward + dxLeft; targetY -= dyForward + dyLeft; break;
        case 'S': targetX -= dxForward; targetY -= dyForward; break;
        default:
            std::cout << "Invalid direction! Please use AQWESDZC.\nEnter move: ";
            return false;
    }
    return true;
}

Roguelike::Roguelike() : currentTurn(0), hasLooked(false), hasFired(false), step(0), robotCount(3) {
    std::ofstream logFile("game_log.txt", std::ios::trunc);
    if (logFile.is_open()) logFile.close();
    initializeMap();
    const std::string robot1Symbol[4] = {"\033[31m^\033[0m", "\033[31m>\033[0m", "\033[31mv\033[0m", "\033[31m<\033[0m"};
    const std::string robot2Symbol[4] = {"\033[34m^\033[0m", "\033[34m>\033[0m", "\033[34mv\033[0m", "\033[34m<\033[0m"};
    const std::string robot3Symbol[4] = {"\033[32m^\033[0m", "\033[32m>\033[0m", "\033[32mv\033[0m", "\033[32m<\033[0m"};
    const std::string robot4Symbol[4] = {"\033[33m^\033[0m", "\033[33m>\033[0m", "\033[33mv\033[0m", "\033[33m<\033[0m"};
    const std::string robot5Symbol[4] = {"\033[35m^\033[0m", "\033[35m>\033[0m", "\033[35mv\033[0m", "\033[35m<\033[0m"};
    const std::string robot6Symbol[4] = {"\033[36m^\033[0m", "\033[36m>\033[0m", "\033[36mv\033[0m", "\033[36m<\033[0m"};
    robots.push_back(new GenericRobot(0, 0, "robot1 (red)", robot1Symbol));
    robots.push_back(new GenericRobot(0, 0, "robot2 (blue)", robot2Symbol));
    robots.push_back(new GenericRobot(0, 0, "robot3 (green)", robot3Symbol));
    robots.push_back(new GenericRobot(0, 0, "robot4 (yellow)", robot4Symbol));
    robots.push_back(new GenericRobot(0, 0, "robot5 (magenta)", robot5Symbol));
    robots.push_back(new GenericRobot(0, 0, "robot6 (cyan)", robot6Symbol));
    for (size_t i = 0; i < 3; ++i) placeRobot(robots[i]);
    for (size_t i = 3; i < robots.size(); ++i) robots[i]->setLives(0);
    std::srand(std::time(0));
}

Roguelike::~Roguelike() {
    for (auto robot : robots) delete robot;
}

void Roguelike::display() {
    system("cls");
    std::vector<std::vector<std::string>> displayMap(MAP_HEIGHT, std::vector<std::string>(MAP_WIDTH, FLOOR));
    if (robots[currentTurn]->isAlive()) {
        displayMap[robots[currentTurn]->getY()][robots[currentTurn]->getX()] = robots[currentTurn]->getSymbol();
    }

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            std::cout << displayMap[y][x] << ' ' << (x == MAP_WIDTH - 1 ? "\n" : "");
        }
    }

    std::cout << "Lives: ";
    for (const auto& robot : robots) {
        std::cout << robot->getName() << ": " << robot->getLives() << (robot == robots.back() ? "\n" : ", ");
    }
    std::cout << "Shells: ";
    for (const auto& robot : robots) {
        std::cout << robot->getName() << ": " << robot->getShells() << (robot == robots.back() ? "\n" : ", ");
    }
    std::cout << "Upgrades: ";
    for (const auto& robot : robots) {
        std::cout << robot->getName() << ": " << robot->getType() << (robot == robots.back() ? "\n" : ", ");
    }

    std::string commandPrompt = robots[currentTurn]->getName() + "'s turn. W/S/A/D/Q/E/Z/C (move), L (look), F (fire), S (reverse)";
    if (robots[currentTurn]->getType() == "JumpBot") commandPrompt += ", J (jump)";
    else if (robots[currentTurn]->getType() == "ScoutBot") commandPrompt += ", K (scout)";
    commandPrompt += ", N (spawn new robot), X (self-destruct), P (quit).\n";
    std::cout << commandPrompt;
}

void Roguelike::moveRobot(char input) {
    step++;
    Robot* currentRobot = robots[currentTurn];
    std::string inputStr;
    std::getline(std::cin, inputStr);
    if (inputStr.length() != 1 || std::cin.fail()) {
        std::cout << "Invalid input! Please enter a single valid command: W/S/A/D/Q/E/Z/C (move), L (look), F (fire), S (reverse)";
        if (currentRobot->getType() == "JumpBot") std::cout << ", J (jump)";
        else if (currentRobot->getType() == "ScoutBot") std::cout << ", K (scout)";
        std::cout << ", N (spawn new robot), X (self-destruct), P (quit).\nEnter move: ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    input = std::toupper(inputStr[0]);

    int dxForward = 0, dyForward = 0, dxLeft = 0, dyLeft = 0;
    switch (currentRobot->getDir()) {
        case NORTH: dyForward = -1; dxLeft = -1; dyLeft = 0; break;
        case EAST: dxForward = 1; dxLeft = 0; dyLeft = -1; break;
        case SOUTH: dyForward = 1; dxLeft = 1; dyLeft = 0; break;
        case WEST: dxForward = -1; dxLeft = 0; dyLeft = 1; break;
    }

    switch (input) {
        case 'W': case 'S': case 'A': case 'D': case 'Q': case 'E': case 'Z': case 'C':
            currentRobot->move(input, *this);
            break;
        case 'L':
            look(currentRobot);
            display();
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            advanceTurn();
            hasLooked = hasFired = false;
            return;
        case 'F': {
            int targetX, targetY;
            if (getFireTarget(currentRobot->getX(), currentRobot->getY(), dxForward, dxLeft, dyForward, dyLeft, targetX, targetY)) {
                currentRobot->fire(targetX, targetY, *this);
                display();
                advanceTurn();
                hasLooked = hasFired = false;
            }
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        case 'J':
            if (currentRobot->getType() == "JumpBot") {
                currentRobot->specialAction(*this);
            } else {
                std::cout << "Jump command only available for JumpBot upgrades!\n";
            }
            advanceTurn();
            hasLooked = hasFired = false;
            return;
        case 'K':
            if (currentRobot->getType() == "ScoutBot") {
                currentRobot->specialAction(*this);
            } else {
                std::cout << "Scout command only available for ScoutBot upgrades!\n";
            }
            advanceTurn();
            hasLooked = hasFired = false;
            return;
        case 'N':
            if (robotCount >= 6) {
                std::cout << "Maximum of 3 additional robots already spawned!\n";
            } else {
                robots[robotCount]->setLives(3);
                robots[robotCount]->reduceShells();
                placeRobot(robots[robotCount]);
                std::cout << robots[robotCount]->getName() << " spawned at (" << robots[robotCount]->getX() << ", " << robots[robotCount]->getY() << ") with 3 lives and 10 shells.\n";
                logMove(robots[robotCount]->getName() + " spawned", robots[robotCount]->getX(), robots[robotCount]->getY(), robots[robotCount]->getX(), robots[robotCount]->getY());
                robotCount++;
            }
            advanceTurn();
            hasLooked = hasFired = false;
            return;
        case 'X':
            std::cout << currentRobot->getName() << " has self-destructed!\n";
            currentRobot->setLives(0);
            map[currentRobot->getY()][currentRobot->getX()] = FLOOR;
            logMove(currentRobot->getName() + " self-destructed", currentRobot->getX(), currentRobot->getY(), currentRobot->getX(), currentRobot->getY());
            advanceTurn();
            hasLooked = hasFired = false;
            return;
        case 'P':
            exit(0);
        default:
            std::cout << "Invalid command! Please enter a single valid command: W/S/A/D/Q/E/Z/C (move), L (look), F (fire), S (reverse)";
            if (currentRobot->getType() == "JumpBot") std::cout << ", J (jump)";
            else if (currentRobot->getType() == "ScoutBot") std::cout << ", K (scout)";
            std::cout << ", N (spawn new robot), X (self-destruct), P (quit).\nEnter move: ";
            return;
    }
    advanceTurn();
    hasLooked = hasFired = false;
}

bool Roguelike::isGameOver() const {
    int aliveCount = 0;
    for (const auto& robot : robots) {
        if (robot->isAlive()) aliveCount++;
    }
    return aliveCount <= 1;
}

std::string Roguelike::getWinner() const {
    for (const auto& robot : robots) {
        if (robot->isAlive()) return robot->getName();
    }
    return "No winner";
}

void GenericRobot::move(char input, Roguelike& game) {
    int newX = x, newY = y;
    int dxForward = 0, dyForward = 0, dxLeft = 0, dyLeft = 0;
    switch (dir) {
        case NORTH: dyForward = -1; dxLeft = -1; dyLeft = 0; break;
        case EAST: dxForward = 1; dxLeft = 0; dyLeft = -1; break;
        case SOUTH: dyForward = 1; dxLeft = 1; dyLeft = 0; break;
        case WEST: dxForward = -1; dxLeft = 0; dyLeft = 1; break;
    }
    switch (input) {
        case 'W': newX += dxForward; newY += dyForward; break;
        case 'S':
            dir = (dir == NORTH) ? SOUTH : (dir == SOUTH) ? NORTH : (dir == EAST) ? WEST : EAST;
            newX -= dxForward; newY -= dyForward;
            break;
        case 'A': newX += dxLeft; newY += dyLeft; break;
        case 'D': newX -= dxLeft; newY -= dyLeft; break;
        case 'Q': newX += dxForward + dxLeft; newY += dyForward + dyLeft; break;
        case 'E': newX += dxForward - dxLeft; newY += dyForward - dyLeft; break;
        case 'Z': newX -= dxForward + dxLeft; newY -= dyForward + dyLeft; break;
        case 'C': newX -= dxForward - dxLeft; newY -= dyForward - dyLeft; break;
    }
    game.moveRobot(this, newX, newY);
}

void JumpBot::specialAction(Roguelike& game) {
    if (jumpCount <= 0) {
        std::cout << name << " has no jumps left!\n";
        return;
    }
    int newX, newY, attempts = 0, maxAttempts = 100;
    do {
        newX = std::rand() % MAP_WIDTH;
        newY = std::rand() % MAP_HEIGHT;
        attempts++;
        if (attempts > maxAttempts) {
            std::cout << "Failed to find a valid jump position!\n";
            return;
        }
    } while (game.isOccupied(newX, newY));
    game.moveRobot(this, newX, newY, true);
    jumpCount--;
    std::cout << name << " jumped to (" << x << ", " << y << "). Jumps left: " << jumpCount << "\n";
}

void JumpBot::move(char input, Roguelike& game) {
    int newX = x, newY = y;
    int dxForward = 0, dyForward = 0, dxLeft = 0, dyLeft = 0;
    switch (dir) {
        case NORTH: dyForward = -1; dxLeft = -1; dyLeft = 0; break;
        case EAST: dxForward = 1; dxLeft = 0; dyLeft = -1; break;
        case SOUTH: dyForward = 1; dxLeft = 1; dyLeft = 0; break;
        case WEST: dxForward = -1; dxLeft = 0; dyLeft = 1; break;
    }
    switch (input) {
        case 'W': newX += dxForward; newY += dyForward; break;
        case 'S':
            dir = (dir == NORTH) ? SOUTH : (dir == SOUTH) ? NORTH : (dir == EAST) ? WEST : EAST;
            newX -= dxForward; newY -= dyForward;
            break;
        case 'A': newX += dxLeft; newY += dyLeft; break;
        case 'D': newX -= dxLeft; newY -= dyLeft; break;
        case 'Q': newX += dxForward + dxLeft; newY += dyForward + dyLeft; break;
        case 'E': newX += dxForward - dxLeft; newY += dyForward - dyLeft; break;
        case 'Z': newX -= dxForward + dxLeft; newY -= dyForward + dyLeft; break;
        case 'C': newX -= dxForward - dxLeft; newY -= dyForward - dyLeft; break;
    }
    game.moveRobot(this, newX, newY);
}

void LongShotBot::move(char input, Roguelike& game) {
    int newX = x, newY = y;
    int dxForward = 0, dyForward = 0, dxLeft = 0, dyLeft = 0;
    switch (dir) {
        case NORTH: dyForward = -1; dxLeft = -1; dyLeft = 0; break;
        case EAST: dxForward = 1; dxLeft = 0; dyLeft = -1; break;
        case SOUTH: dyForward = 1; dxLeft = 1; dyLeft = 0; break;
        case WEST: dxForward = -1; dxLeft = 0; dyLeft = 1; break;
    }
    switch (input) {
        case 'W': newX += dxForward; newY += dyForward; break;
        case 'S':
            dir = (dir == NORTH) ? SOUTH : (dir == SOUTH) ? NORTH : (dir == EAST) ? WEST : EAST;
            newX -= dxForward; newY -= dyForward;
            break;
        case 'A': newX += dxLeft; newY += dyLeft; break;
        case 'D': newX -= dxLeft; newY -= dyLeft; break;
        case 'Q': newX += dxForward + dxLeft; newY += dyForward + dyLeft; break;
        case 'E': newX += dxForward - dxLeft; newY += dyForward - dyLeft; break;
        case 'Z': newX -= dxForward + dxLeft; newY -= dyForward + dyLeft; break;
        case 'C': newX -= dxForward - dxLeft; newY -= dyForward - dyLeft; break;
    }
    game.moveRobot(this, newX, newY);
}

void ScoutBot::specialAction(Roguelike& game) {
    if (scoutCount <= 0) {
        std::cout << name << " has no scouts left!\n";
        return;
    }
    scoutCount--;
    std::cout << "Scouting entire battlefield (Scouts left: " << scoutCount << "):\n";
    game.scoutFor(this);
}

void ScoutBot::move(char input, Roguelike& game) {
    int newX = x, newY = y;
    int dxForward = 0, dyForward = 0, dxLeft = 0, dyLeft = 0;
    switch (dir) {
        case NORTH: dyForward = -1; dxLeft = -1; dyLeft = 0; break;
        case EAST: dxForward = 1; dxLeft = 0; dyLeft = -1; break;
        case SOUTH: dyForward = 1; dxLeft = 1; dyLeft = 0; break;
        case WEST: dxForward = -1; dxLeft = 0; dyLeft = 1; break;
    }
    switch (input) {
        case 'W': newX += dxForward; newY += dyForward; break;
        case 'S':
            dir = (dir == NORTH) ? SOUTH : (dir == SOUTH) ? NORTH : (dir == EAST) ? WEST : EAST;
            newX -= dxForward; newY -= dyForward;
            break;
        case 'A': newX += dxLeft; newY += dyLeft; break;
        case 'D': newX -= dxLeft; newY -= dyLeft; break;
        case 'Q': newX += dxForward + dxLeft; newY += dyForward + dyLeft; break;
        case 'E': newX += dxForward - dxLeft; newY += dyForward - dyLeft; break;
        case 'Z': newX -= dxForward + dxLeft; newY -= dyForward + dyLeft; break;
        case 'C': newX -= dxForward - dxLeft; newY -= dyForward - dyLeft; break;
    }
    game.moveRobot(this, newX, newY);
}

std::ostream& operator<<(std::ostream& os, const Robot& robot) {
    os << robot.name << " (" << robot.getType() << ") at (" << robot.x << ", " << robot.y << "), "
       << "Lives: " << robot.lives << ", Shells: " << robot.shells;
    if (robot.getType() == "JumpBot") os << ", Jumps: " << robot.jumpCount;
    if (robot.getType() == "ScoutBot") os << ", Scouts: " << robot.scoutCount;
    return os;
}

int main() {
    Roguelike game;
    char input;
    while (!game.isGameOver()) {
        game.display();
        std::cout << "Enter move: ";
        game.moveRobot(input);
    }
    system("cls");
    game.display();
    std::cout << "Game Over! Winner: " << game.getWinner() << std::endl;
    return 0;
}
