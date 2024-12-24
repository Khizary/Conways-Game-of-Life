#include <iostream>
#include <thread>
#include <chrono>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <stdexcept>
#include <SFML/Audio.hpp>

using namespace std;
// We could use an extra 2 cells on each dimension to make update logic simpler
// but that would require extra memory which scales quadratically with size
// i.e if size is 1000, setting it to 1002 would mean 4004 extra cells,
// which is not that much in contrast to the 1,000,000 cells being used.

const int RAW_SIZE = 250;      // creates n x n grid.
const int SIZE = RAW_SIZE + 2; // formal size with zero value borders for simpler compute.
const int FIRST = 1;
const int LAST = SIZE - 1;
const int INIT_ALIVE = RAW_SIZE * RAW_SIZE * 0.05; // number of initially alive cells, scales with grid size
const float RS_FLOAT = RAW_SIZE;
const float CELLSIZE = 7.f * (1 / (RS_FLOAT / 100)); // made this formula with trial and error to keep window size constant regardless of grid size.
const int HEAT_BLOCKSIZE = RAW_SIZE / 25;
const int HEAT_CELLSIZE = CELLSIZE * HEAT_BLOCKSIZE;
const int HEAT_RAW_SIZE = RAW_SIZE / HEAT_BLOCKSIZE;
const int HEAT_SIZE = HEAT_RAW_SIZE + 2;
const int HEAT_FIRST = 1;
const int HEAT_QUAD_FACTOR = 4;
const int HEAT_LAST = HEAT_SIZE - 1;
const float HEAT_CELL_SCALE = 1;
const int NUKE_RADIUS = 20;

void drawGrid(sf::RenderWindow &window, int grid[SIZE][SIZE], float cellSize)
{
    // using vertex arrays to reduce window.draw calls since window.draw seems to slow down sfml when done for the entire grid.
    // this should increase draw speed significantly
    sf::VertexArray vertices(sf::Quads, SIZE * SIZE * 4);
    int index = 0;

    for (int i = FIRST; i <= LAST; ++i)
    {
        for (int j = FIRST; j <= LAST; ++j)
        {
            float x = j * cellSize - cellSize;
            float y = i * cellSize - cellSize;
            // handling 4 vertices for each "rectangle" hence elementwise declrations
            vertices[index].position = sf::Vector2f(x, y);
            vertices[index + 1].position = sf::Vector2f(x + cellSize, y);
            vertices[index + 2].position = sf::Vector2f(x + cellSize, y + cellSize);
            vertices[index + 3].position = sf::Vector2f(x, y + cellSize);

            sf::Color color = (grid[i][j] == 1) ? sf::Color::Blue : sf::Color::Black;
            vertices[index].color = color;
            vertices[index + 1].color = color;
            vertices[index + 2].color = color;
            vertices[index + 3].color = color;
            index += 4;
        }
    }
    window.draw(vertices);
}

void drawHeatMap(sf::RenderWindow &window, int heatmap[HEAT_SIZE][HEAT_SIZE], float cellSize)
{
    sf::VertexArray vertices(sf::Quads, (HEAT_SIZE * HEAT_SIZE) * HEAT_QUAD_FACTOR);
    int index = 0;

    for (int i = HEAT_FIRST; i <= HEAT_LAST; ++i)
    {
        for (int j = HEAT_FIRST; j <= HEAT_LAST; ++j)
        {
            float x = j * cellSize - cellSize;
            float y = i * cellSize - cellSize;
            vertices[index].position = sf::Vector2f(x, y);
            vertices[index + 1].position = sf::Vector2f(x + cellSize * HEAT_CELL_SCALE, y);
            vertices[index + 2].position = sf::Vector2f(x + cellSize * HEAT_CELL_SCALE, y + cellSize * HEAT_CELL_SCALE);
            vertices[index + 3].position = sf::Vector2f(x, y + cellSize * HEAT_CELL_SCALE);

            float ratio = static_cast<float>(abs(heatmap[i][j] - 1)) / (HEAT_BLOCKSIZE * HEAT_BLOCKSIZE - 1);
            sf::Color color(
                static_cast<sf::Uint8>(ratio * 255), // Red component increases with ratio
                static_cast<sf::Uint8>(ratio * 100),
                0);
            vertices[index].color = color;
            vertices[index + 1].color = color;
            vertices[index + 2].color = color;
            vertices[index + 3].color = color;

            index += 4;
        }
    }
    window.draw(vertices);
}

void initDead(int (&grid)[SIZE][SIZE])
{
    // function to initialize 2d array to 0 values basically
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            grid[i][j] = 0;
        }
    }
}

void initDeadHeat(int (&heatmap)[HEAT_SIZE][HEAT_SIZE])
{
    // function to initialize 2d array to 0 values basically
    for (int i = 0; i < HEAT_SIZE; i++)
    {
        for (int j = 0; j < HEAT_SIZE; j++)
        {
            heatmap[i][j] = 0;
        }
    }
}

void initRandom(int (&grid)[SIZE][SIZE], const int repetition)
{
    for (int r = 0; r < repetition; r++)
    {
        // function to initialize some "seed" cells
        int initial_alive_x = (rand() % LAST) + FIRST;
        int initial_alive_y = (rand() % LAST) + FIRST;
        // cout << SIZE ;
        // cout <<" --" << SIZE << "-- " << initial_alive_x << " : " << initial_alive_y;
        // cout << "X: " << initial_alive_x << " Y: " <<initial_alive_y << endl;
        grid[initial_alive_x][initial_alive_y] = 1;
    }
}


void updateGrid(int grid[SIZE][SIZE])
{
    // function to update grid according to rules of underpopulation, overpopulation and spawn.
    int tempGrid[SIZE][SIZE] = {0};
    for (int i = 1; i < SIZE - 1; i++)
    {
        for (int j = 1; j < SIZE - 1; j++)
        {
            int liveNeighbors =
                grid[i - 1][j - 1] +
                grid[i - 1][j] +
                grid[i - 1][j + 1] +
                grid[i][j - 1] +
                grid[i][j + 1] +
                grid[i + 1][j - 1] +
                grid[i + 1][j] +
                grid[i + 1][j + 1];

            if (grid[i][j] == 1)
            {
                if (liveNeighbors < 2 || liveNeighbors > 3)
                {
                    tempGrid[i][j] = 0;
                }
                else
                {
                    tempGrid[i][j] = 1;
                }
            }
            else
            {
                if (liveNeighbors == 3)
                {
                    tempGrid[i][j] = 1;
                }
            }
        }
    }

    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            grid[i][j] = tempGrid[i][j];
        }
    }
}

void updateHeatMap(int (&grid)[SIZE][SIZE], int (&heatmap)[HEAT_SIZE][HEAT_SIZE])
{
    int row = 1;
    for (int i = FIRST; i <= LAST - HEAT_BLOCKSIZE; i += HEAT_BLOCKSIZE)
    {
        int col = 1;
        for (int j = FIRST; j <= LAST - HEAT_BLOCKSIZE; j += HEAT_BLOCKSIZE)
        {
            int sum = 0;
            for (int x = 0; x < HEAT_BLOCKSIZE; x++)
            {
                for (int y = 0; y < HEAT_BLOCKSIZE; y++)
                {
                    sum += grid[i + x][j + y];
                }
            }

            heatmap[row][col] = sum;
            col++;
        }
        row++;
    }
}

void toggle(bool &inputbool)
{
    inputbool = !inputbool;
}

bool isMouseOverButton(const sf::RectangleShape &button, sf::RenderWindow &window)
{
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f buttonPos = button.getPosition();
    sf::Vector2f buttonSize = button.getSize();

    return (mousePos.x >= buttonPos.x && mousePos.x <= buttonPos.x + buttonSize.x &&
            mousePos.y >= buttonPos.y && mousePos.y <= buttonPos.y + buttonSize.y);
}

void nuke(int (&grid)[SIZE][SIZE], sf::CircleShape &nukeCircle, int x, int y, sf::Sound &nukesound)
{
    nukesound.play();

    int nuke_centre_x = x / CELLSIZE - FIRST;
    int nuke_centre_y = y / CELLSIZE - FIRST;
    cout << "Nuked " << nuke_centre_x << " : " << nuke_centre_y << endl;
    // two approaches possible here;
    //  1. cut out around centre = very arduous to check validitiy of cels and rougher shape
    // 2. check all cells to see if within circle = less computation, easier to implement
    nukeCircle.setPosition(static_cast<float>(nuke_centre_y * CELLSIZE) - nukeCircle.getRadius(), static_cast<float>(nuke_centre_x * CELLSIZE) - nukeCircle.getRadius());
    nukeCircle.setFillColor(sf::Color::Transparent);
    nukeCircle.setOutlineColor(sf::Color::Red);
    nukeCircle.setOutlineThickness(2.f);

    for (int i = FIRST; i <= LAST; ++i)
    {
        for (int j = FIRST; j <= LAST; ++j)
        {
            float dx = i - nuke_centre_x;
            float dy = j - nuke_centre_y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance <= NUKE_RADIUS)
            {
                grid[i][j] = 0;
            }
        }
    }
}

void repopulate(int (&grid)[SIZE][SIZE], sf::CircleShape &nukeCircle, int x, int y, sf::Sound &revivesound)
{
    revivesound.play();
    int repop_centre_x = x / CELLSIZE - FIRST;
    int repop_centre_y = y / CELLSIZE - FIRST;
    cout << "Repopulated " << repop_centre_x << " : " << repop_centre_y << endl;
    // two approaches possible here;
    //  1. cut out around centre = very arduous to check validitiy of cels and rougher shape
    // 2. check all cells to see if within circle = less computation, easier to implement
    nukeCircle.setPosition(static_cast<float>(repop_centre_y * CELLSIZE) - nukeCircle.getRadius(), static_cast<float>(repop_centre_x * CELLSIZE) - nukeCircle.getRadius());
    nukeCircle.setFillColor(sf::Color::Transparent);
    nukeCircle.setOutlineColor(sf::Color::Green);
    nukeCircle.setOutlineThickness(2.f);

    for (int i = FIRST; i <= LAST; ++i)
    {
        for (int j = FIRST; j <= LAST; ++j)
        {
            float dx = i - repop_centre_x;
            float dy = j - repop_centre_y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance <= NUKE_RADIUS && (rand() % 2) == 0)
            {
                grid[i][j] = 1;
            }
        }
    }
}

int main()
{
    // music
    cout << "Conway's Game of Life" << endl;
    cout << "By Khizar and Harris" << endl
         << endl;

    sf::SoundBuffer soundBuffer;
    sf::SoundBuffer revsoundBuffer;
    sf::Sound nukesound;
    sf::Sound revivesound;
    sf::Music backgroundMusic;
    if (!soundBuffer.loadFromFile("../resources/nuke2.wav"))
    {
        cout << "error loading nuke effect" << endl;
    }
    if (!revsoundBuffer.loadFromFile("../resources/revive.wav"))
    {
        cout << "error loading revive effect" << endl;
    }


    nukesound.setBuffer(soundBuffer);
    revivesound.setBuffer(revsoundBuffer);
    nukesound.setVolume(30.f);
    revivesound.setVolume(15.f);

    if (!backgroundMusic.openFromFile("../resources/backgroud_music.ogg"))
    {
        cout << "error loading background music" << endl;
    }

    backgroundMusic.setLoop(true); // Loop the music
    backgroundMusic.play();

    bool heatmapstate = false;
    int framecount = 0;
    int grid[SIZE][SIZE];
    int heatmap[HEAT_SIZE][HEAT_SIZE];

    initDead(grid);
    initDeadHeat(heatmap);
    initRandom(grid, INIT_ALIVE);

    sf::RenderWindow window(sf::VideoMode(2 * CELLSIZE * SIZE, 1.2 * CELLSIZE * SIZE), "My window");

    sf::Font font;
    if (!font.loadFromFile("../resources/font2.ttf"))
    {
        std::cout << "error loading font\n";
        return -1;
    }

    // heatmap button

    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(8 * CELLSIZE);
    text.setFillColor(sf::Color::White);
    text.setPosition(CELLSIZE * (SIZE + 10), 35 * CELLSIZE);

    sf::RectangleShape button(sf::Vector2f(60 * CELLSIZE,15 * CELLSIZE));
    button.setPosition((SIZE + 10) * CELLSIZE, 2 * CELLSIZE);
    button.setFillColor(sf::Color::Blue);

    sf::Text buttonText;
    buttonText.setFont(font);
    buttonText.setString("Toggle Heatmap");
    buttonText.setCharacterSize(8 * CELLSIZE);
    buttonText.setFillColor(sf::Color::White);
    buttonText.setPosition((SIZE + 11) * CELLSIZE, 3 * CELLSIZE);

    // nuke text

    sf::Text nukeText;
    nukeText.setFont(font);
    nukeText.setString("LEFT CLICK TO NUKE!\nRIGHT CLICK TO SPAWN!");
    nukeText.setCharacterSize(8 * CELLSIZE);
    nukeText.setFillColor(sf::Color::White);
    nukeText.setPosition(CELLSIZE * (SIZE + 10), SIZE * CELLSIZE * 0.2);
    ;

    // nuke circle
    sf::CircleShape nukeCircle(static_cast<float>(NUKE_RADIUS * CELLSIZE));
    bool showNukeCircle = false;
    int currentframe = framecount;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    if (isMouseOverButton(button, window))
                    {
                        toggle(heatmapstate);
                        cout << "Toggled heatmap" << endl;
                    }
                    sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
                    if (mousePosition.x <= LAST * CELLSIZE && mousePosition.y <= LAST * CELLSIZE)
                    {
                        nuke(grid, nukeCircle, mousePosition.y, mousePosition.x, nukesound);
                        showNukeCircle = true;
                        currentframe = framecount;
                    }
                }
                if (event.mouseButton.button == sf::Mouse::Right)
                {
                    sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
                    if (mousePosition.x <= LAST * CELLSIZE && mousePosition.y <= LAST * CELLSIZE)
                    {
                        repopulate(grid, nukeCircle, mousePosition.y, mousePosition.x, revivesound);
                        showNukeCircle = true;
                        currentframe = framecount;
                    }
                }
            }
        }
        if (abs(framecount - currentframe) > 1 && framecount > 5)
        {
            showNukeCircle = false;
        }
        framecount++;
        text.setString("GENERATION: " + std::to_string(framecount));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        window.clear(sf::Color::Black);

        updateGrid(grid);
        if (heatmapstate == false)
        {
            // nukeCircle.setOutlineColor(sf::Color::Red);
            drawGrid(window, grid, CELLSIZE);
        }
        else
        {
            updateHeatMap(grid, heatmap);
            // printHeatMap(heatmap);
            nukeCircle.setOutlineColor(sf::Color::White);
            drawHeatMap(window, heatmap, HEAT_CELLSIZE);
            // cout << "ran" << endl;
        }

        if (showNukeCircle)
        {
            window.draw(nukeCircle);
        }
        window.draw(text);
        window.draw(button);
        window.draw(buttonText);
        window.draw(nukeText);

        window.display();
    }

    return 0;
}