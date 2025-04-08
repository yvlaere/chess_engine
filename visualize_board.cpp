#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <map>

const int TILE_SIZE = 64;  // Size of each tile (64x64 pixels)
const int BOARD_SIZE = 8;   // 8x8 chess board

// Structure to store the piece names (images)
enum class Piece { Empty, w_pawn, w_knight, w_bishop, w_rook, w_queen, w_king,
                   b_pawn, b_knight, b_bishop, b_rook, b_queen, b_king };

// A map to associate pieces with their image file paths
std::map<Piece, std::string> pieceImages = {
    {Piece::w_pawn, "assets/pieces/white_pawn.png"},
    {Piece::w_knight, "assets/pieces//white_knight.png"},
    {Piece::w_bishop, "assets/pieces/white_bishop.png"},
    {Piece::w_rook, "assets/pieces/white_rook.png"},
    {Piece::w_queen, "assets/pieces/white_queen.png"},
    {Piece::w_king, "assets/pieces/white_king.png"},
    {Piece::b_pawn, "assets/pieces/black_pawn.png"},
    {Piece::b_knight, "assets/pieces/black_knight.png"},
    {Piece::b_bishop, "assets/pieces/black_bishop.png"},
    {Piece::b_rook, "assets/pieces/black_rook.png"},
    {Piece::b_queen, "assets/pieces/black_queen.png"},
    {Piece::b_king, "assets/pieces/black_king.png"},
    {Piece::Empty, ""}
};

// A 2D array representing the chess board
Piece board[BOARD_SIZE][BOARD_SIZE] = {
    {Piece::BRook, Piece::BKnight, Piece::BBishop, Piece::BQueen, Piece::BKing, Piece::BBishop, Piece::BKnight, Piece::BRook},
    {Piece::BPawn, Piece::BPawn, Piece::BPawn, Piece::BPawn, Piece::BPawn, Piece::BPawn, Piece::BPawn, Piece::BPawn},
    {Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty},
    {Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty},
    {Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty},
    {Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty, Piece::Empty},
    {Piece::WPawn, Piece::WPawn, Piece::WPawn, Piece::WPawn, Piece::WPawn, Piece::WPawn, Piece::WPawn, Piece::WPawn},
    {Piece::WRook, Piece::WKnight, Piece::WBishop, Piece::WQueen, Piece::WKing, Piece::WBishop, Piece::WKnight, Piece::WRook}
};

// Function to render the chess board
void renderBoard(sf::RenderWindow& window) {
    sf::Texture whiteTexture, blackTexture;
    sf::Sprite whiteTile, blackTile;
    whiteTexture.loadFromFile("resources/white_square.png");
    blackTexture.loadFromFile("resources/black_square.png");

    // Loop through the board and draw the squares
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            sf::Sprite tile;
            if ((row + col) % 2 == 0) {
                tile.setTexture(whiteTexture);
            } else {
                tile.setTexture(blackTexture);
            }

            tile.setPosition(col * TILE_SIZE, row * TILE_SIZE);
            window.draw(tile);

            // Draw the piece image if there is a piece at this location
            Piece piece = board[row][col];
            if (piece != Piece::Empty) {
                sf::Texture pieceTexture;
                if (pieceImages.count(piece)) {
                    pieceTexture.loadFromFile(pieceImages[piece]);
                    sf::Sprite pieceSprite(pieceTexture);
                    pieceSprite.setPosition(col * TILE_SIZE, row * TILE_SIZE);
                    window.draw(pieceSprite);
                }
            }
        }
    }
}

int main() {
    // Create a window to display the chess board
    sf::RenderWindow window(sf::VideoMode(BOARD_SIZE * TILE_SIZE, BOARD_SIZE * TILE_SIZE), "Chess Board");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Clear the window
        window.clear();

        // Render the chess board
        renderBoard(window);

        // Display the rendered content
        window.display();
    }

    return 0;
}