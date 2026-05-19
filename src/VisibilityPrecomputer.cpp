#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>


#define PI 3.14159f


void computeVisibility(int mapW, int mapH, const std::vector<std::string>& grid, const std::string& outputFile) {
    std::ofstream fout(outputFile);
    if (!fout.is_open()) {
        std::cerr << "Error: Could not open output file." << std::endl;
        return;
    }

    // Save dimensions
    fout << mapW << " " << mapH << "\n";

    // Stratified sampling instead of random rays
    int N_ORIGINS = 50;                                                 // Number of random origin points per cell
    int N_ANGLES = 360 * 0.5;                                           // 0.5 degree steps to guarantee no gaps at large distances
    std::mt19937 rng(42);                                               // Fixed seed for reproducibility
    std::uniform_real_distribution<float> pos_dist(0.01f, 0.99f);       // Prevents floating-point anomalies
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * PI);

    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            // Wall
            if (grid[y][x] == '1') {
                fout << x << " " << y << " 0\n";
                continue;
            }

            std::set<std::pair<int, int>> visible;
            visible.insert({x, y}); // cell always sees itself
            
            for (int o = 0; o < N_ORIGINS; ++o) {
                // Pick a random origin once for this batch of rays
                float startX = x + pos_dist(rng);
                float startY = y + pos_dist(rng);

                for (int a = 0; a < N_ANGLES; ++a) {
                    // Sweep in a uniform circle
                    float angle = a * (2.0f * PI / N_ANGLES);
                    
                    float dirX = cos(angle);
                    float dirY = sin(angle);
                    
                    int cx = floor(startX);
                    int cy = floor(startY);

                    // 2D ray grid traversal (Supercover Bresenham equivalent)
                    int stepX = (dirX > 0) ? 1 : ((dirX < 0) ? -1 : 0);
                    int stepY = (dirY > 0) ? 1 : ((dirY < 0) ? -1 : 0);

                    float tMaxX = (stepX != 0) ? (floor(startX + (stepX > 0 ? 1 : 0)) - startX) / dirX : INFINITY;
                    float tMaxY = (stepY != 0) ? (floor(startY + (stepY > 0 ? 1 : 0)) - startY) / dirY : INFINITY;

                    float tDeltaX = (stepX != 0) ? abs(1.0f / dirX) : INFINITY;
                    float tDeltaY = (stepY != 0) ? abs(1.0f / dirY) : INFINITY;

                    while (cx >= 0 && cx < mapW && cy >= 0 && cy < mapH) {
                        visible.insert({cx, cy});
                        
                        // Wall
                        if (grid[cy][cx] == '1') {
                            break;
                        }

                        if (tMaxX < tMaxY) {
                            tMaxX += tDeltaX;
                            cx += stepX;
                        } else {
                            tMaxY += tDeltaY;
                            cy += stepY;
                        }
                    }
                }
            }

            // Visibility dilation (if a cell is visible, mark its 8 neighbors visible too)
            std::set<std::pair<int, int>> final_visible;
            for (auto& cell : visible) {
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = cell.first + dx;
                        int ny = cell.second + dy;
                        if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH) {
                            final_visible.insert({nx, ny});
                        }
                    }
                }
            }

            // Write to file (x y num_visible vx1 vy1 vx2 vy2 ...)
            fout << x << " " << y << " " << final_visible.size();
            for (auto& cell : final_visible) {
                fout << " " << cell.first << " " << cell.second;
            }
            fout << "\n";
        }
    }
    
    fout.close();
    std::cout << "Visibility precomputation successfully saved to " << outputFile << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <map_file.txt> <output_visibility.txt>\n";
        return 1;
    }

    std::ifstream fin(argv[1]);
    if (!fin.is_open()) {
        std::cerr << "Error: Could not open map file." << std::endl;
        return 1;
    }

    int mapW, mapH;
    fin >> mapW >> mapH;
    
    std::vector<std::string> grid(mapH);
    for (int y = 0; y < mapH; ++y) {
        fin >> grid[y];
    }
    fin.close();

    computeVisibility(mapW, mapH, grid, argv[2]);

    return 0;
}