#include "move_generation.h"
#include <limits>
#include <string>

// plain negamax with alpha-beta pruning

constexpr int INF = std::numeric_limits<int>::max() / 2;

// to remove visualization, only for debugging purposes
void visualize_game_state(const game_state& state) {
    const char* piece_symbols = "PNBRQKpnbrqk";

    char board[8][8] = {};

    // Initialize board with empty spaces
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            board[r][c] = '.';

    // Fill the board with pieces
    for (int i = 0; i < 12; ++i) {
        U64 bitboard = state.piece_bitboards[i];
        for (int square = 0; square < 64; ++square) {
            if (bitboard & (1ULL << square)) {
                int row = 7 - (square / 8);
                int col = square % 8;
                board[row][col] = piece_symbols[i];
            }
        }
    }

    // Mark en passant squares
    for (int i = 0; i < 2; ++i) {
        U64 ep_board = state.en_passant_bitboards[i];
        for (int square = 0; square < 64; ++square) {
            if (ep_board & (1ULL << square)) {
                int row = 7 - (square / 8);
                int col = square % 8;
                board[row][col] = '*';  // En passant target square
            }
        }
    }

    // Print the board
    std::cout << "  a b c d e f g h\n";
    std::cout << "  ----------------\n";
    for (int r = 0; r < 8; ++r) {
        std::cout << (8 - r) << "| ";
        for (int c = 0; c < 8; ++c) {
            std::cout << board[r][c] << ' ';
        }
        std::cout << "|\n";
    }
    std::cout << "  ----------------\n";

    // Print castling rights
    std::cout << "Castling rights: "
              << (state.w_long_castle ? "Q" : "-")
              << (state.w_short_castle ? "K" : "-")
              << (state.b_long_castle ? "q" : "-")
              << (state.b_short_castle ? "k" : "-") << "\n";
}

int evaluation(game_state &state) {
    // basic evaluation with piece square tables, based on the simplified evaluation function on chessprogramming.org

    // piece values in centipawns
    int pawn_value = 100;
    int knight_value = 320;
    int bishop_value = 330;
    int rook_value = 500;
    int queen_value = 900;
    int king_value = 20000;

    std::array<int, 6> piece_values = {pawn_value, knight_value, bishop_value, rook_value, queen_value, king_value};

    // piece square tables
    std::array<int, 64> pawn_square_table = {0, 0, 0, 0, 0, 0, 0, 0, 5, 10, 10, -20, -20, 10, 10, 5, 5, -5, -10, 0, 0, -10, -5, 5, 0, 0, 0, 20, 20, 0, 0, 0, 5, 5, 10, 25, 25, 10, 5, 5, 10, 10, 20, 30, 30, 20, 10, 10, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
    std::array<int, 64> knight_square_table = {-50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0, 5, 5, 0, -20, -40, -30, 5, 10, 15, 15, 10, 5, -30, -30, 0, 15, 20, 20, 15, 0, -30, -30, 5, 15, 20, 20, 15, 5, -30, -30, 0, 10, 15, 15, 10, 0, -30, -40, -20, 0, 0, 0, 0, -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};
    std::array<int, 64> bishop_square_table = {-20, -10, -10, -10, -10, -10, -10, -20, -10, 5, 0, 0, 0, 0, 5, -10, -10, 10, 10, 10, 10, 10, 10, -10, -10, 0, 10, 10, 10, 10, 0, -10, -10, 5, 5, 10, 10, 5, 5, -10, -10, 0, 5, 10, 10, 5, 0, -10, -10, 0, 0, 0, 0, 0, 0, -10, -20, -10, -10, -10, -10, -10, -10, -20};
    std::array<int, 64> rook_square_table = {0, 0, 0, 5, 5, 0, 0, 0, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, 5, 10, 10, 10, 10, 10, 10, 5, 0, 0, 0, 0, 0, 0, 0, 0};
    std::array<int, 64> queen_square_table = {-20, -10, -10, -5, -5, -10, -10, -20, -10, 0, 5, 0, 0, 0, 0, -10, -10, 5, 5, 5, 5, 5, 0, -10, 0, 0, 5, 5, 5, 5, 0, -5, -5, 0, 5, 5, 5, 5, 0, -5, -10, 0, 5, 5, 5, 5, 0, -5, -10, 0, 0, 0, 0, 0, 0, -10, -20, -10, -10, -5, -5, -10, -10, -20};
    std::array<int, 64> king_square_table = {20, 30, 10, 0, 0, 10, 30, 20, 20, 20, 0, 0, 0, 0, 20, 20, -10, -20, -20, -20, -20, -20, -20, -10, -20, -30, -30, -40, -40, -30, -30, -20, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30};
    
    std::array<std::array<int, 64>, 6> piece_square_tables = {pawn_square_table, knight_square_table, bishop_square_table, rook_square_table, queen_square_table, king_square_table};

    std::array<int, 64> endgame_king_square_table = {-50, -30, -30, -30, -30, -30, -30, -50, -30, -30, 0, 0, 0, 0, -30, -30, -30, -10, 20, 30, 30, 20, -10, -30, -30, -10, 30, 40, 40, 30, -10, -30, -30, -10, 30, 40, 40, 30, -10, -30, -30, -10, 20, 30, 30, 20, -10, -30, -30, -20, -10, 0, 0, -10, -20, -30, -50, -40, -30, -20, -20, -30, -40, -50};
    
    int score = 0;

    for (int piece_index = 0; piece_index < 12; ++piece_index) {
        U64 bb = state.piece_bitboards[piece_index];
        while (bb) {
            int square = __builtin_ctzll(bb);
            
            // black pieces
            if (piece_index >= 6) {
                score -= piece_values[piece_index - 6];
                score -= piece_square_tables[piece_index - 6][63 - square];
            }

            // white pieces
            else {
                score += piece_values[piece_index];
                score += piece_square_tables[piece_index][square];
            }
            
            bb &= bb - 1;
        }
    }

    return score;

}

// fast negamax search with alpha-beta pruning.
// 'depth' is the remaining search depth, and alpha-beta parameters prune branches.
int negamax(game_state &state, int depth, int alpha, int beta, bool color, 
    std::array<std::array<U64, 64>, 2>& pawn_move_lookup_table, 
    std::array<std::array<U64, 64>, 2>& pawn_attack_lookup_table, 
    std::array<U64, 64>& knight_lookup_table, 
    std::array<U64, 64>& bishop_magics, 
    std::array<U64, 64>& bishop_mask_lookup_table, 
    std::array<std::array<U64, 4096>, 64>& bishop_attack_lookup_table, 
    std::array<U64, 64>& rook_magics, 
    std::array<U64, 64>& rook_mask_lookup_table,
    std::array<std::array<U64, 4096>, 64>& rook_attack_lookup_table, 
    std::array<U64, 64>& king_lookup_table,
    const U64& occupancy_bitboard) {

    if (depth == 0) {
        int eval = evaluation(state);
        return color ? -eval : eval;
    }
    
    // generate moves from the current position.
    // generate pseudo-legal moves
    std::array<move, 256> moves;
    int move_count = pseudo_legal_move_generator(
        moves, state, color, pawn_move_lookup_table, pawn_attack_lookup_table, 
        knight_lookup_table, bishop_magics, bishop_mask_lookup_table, 
        bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, 
        rook_attack_lookup_table, king_lookup_table, 
        occupancy_bitboard);

    int max_score = -INF;
    int legal_moves = 0;

    // iterate over all pseudo-legal moves
    for (int i = 0; i < move_count; i++) {

        game_state new_state = apply_move(state, moves[i]);
        U64 new_occupancy = get_occupancy(new_state.piece_bitboards);

        // ensure move is legal (not putting king in check)
        if (pseudo_to_legal(new_state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, new_occupancy)) {
            // apply negamax
            int score = -negamax(new_state, depth - 1, -beta, -alpha, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, new_occupancy);
            legal_moves++;

            if (score > max_score) {
                max_score = score;
            }
            if (score > alpha) {
                alpha = score;
            }
            if (alpha >= beta) {
               break;
            }
        }

        // undo the move
        //state = undo_move();
    }

    // terminal node: checkmate or stalemate.
    if (legal_moves == 0) {
        // king is attacked: checkmate
        if (pseudo_to_legal(state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, occupancy_bitboard)) {
            // stalemate
            return 0;
        }
        else {
            // checkmate
            return -INF;
        }
    }
    
    return max_score;
}

std::string index_to_chess(int index) {
    if (index < 0 || index >= 64) {
        return ""; // or handle the error as appropriate
    }
    char file = 'a' + (index % 8);      // Files: a-h
    int rank = (index / 8) + 1;           // Ranks: 1-8
    return std::string(1, file) + std::to_string(rank);
}

int main() {
    // initial game state
    // convention: least significant bit (rightmost bit) is A1

    U64 w_pawn = 65280;
    U64 w_knight = 66;
    U64 w_bishop = 36;
    U64 w_rook = 129;
    U64 w_queen = 8;
    U64 w_king = 16;
    U64 b_pawn = 71776119061217280;
    U64 b_knight = 4755801206503243776;
    U64 b_bishop = 2594073385365405696;
    U64 b_rook = 9295429630892703744;
    U64 b_queen = 576460752303423488;
    U64 b_king = 1152921504606846976;

    U64 w_en_passant = 0;
    U64 b_en_passant = 0;

    bool w_long_castle = true;
    bool w_short_castle = true;
    bool b_long_castle = true;
    bool b_short_castle = true;

    // initialize game state
    std::array<U64, 12> piece_bitboards = {w_pawn, w_knight, w_bishop, w_rook, w_queen, w_king, b_pawn, b_knight, b_bishop, b_rook, b_queen, b_king};
    std::array<U64, 2> en_passant_bitboards = {w_en_passant, b_en_passant};
    game_state initial_game_state(piece_bitboards, en_passant_bitboards, w_long_castle, w_short_castle, b_long_castle, b_short_castle);

    // create lookup tables
    std::array<std::array<U64, 64>, 2> pawn_move_lookup_table;
    std::array<std::array<U64, 64>, 2> pawn_attack_lookup_table;
    std::array<U64, 64> knight_lookup_table;
    std::array<U64, 64> bishop_magics;
    std::array<U64, 64> bishop_mask_lookup_table;
    std::array<std::array<U64, 4096>, 64> bishop_attack_lookup_table;
    std::array<U64, 64> rook_magics;
    std::array<U64, 64> rook_mask_lookup_table;
    std::array<std::array<U64, 4096>, 64> rook_attack_lookup_table;
    std::array<U64, 64> king_lookup_table;

    generate_lookup_tables( pawn_move_lookup_table, pawn_attack_lookup_table, 
        knight_lookup_table, bishop_magics, bishop_mask_lookup_table, 
        bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table,
        rook_attack_lookup_table, king_lookup_table);

    // initialize
    game_state state = initial_game_state;
    U64 occupancy_bitboard = get_occupancy(state.piece_bitboards);
    int best_score;
    int best_move_index = 0;
    bool color = false;

    //std::cout << "evaluation" << std::endl;
    //std::cout << evaluation(initial_game_state) << std::endl;

    //int score = negamax(initial_game_state, 2, -INF, INF, color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, occupancy_bitboard);
    //std::cout << score << std::endl;

    // game loop
    while (true) {
        
        int best_score = -INF;
        int best_move_index = 0;

        // generate pseudo-legal moves
        std::array<move, 256> moves;
        int move_count = pseudo_legal_move_generator(
            moves, state, color, pawn_move_lookup_table, pawn_attack_lookup_table, 
            knight_lookup_table, bishop_magics, bishop_mask_lookup_table, 
            bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, 
            rook_attack_lookup_table, king_lookup_table, 
            occupancy_bitboard);

        // iterate over all pseudo-legal moves
        for (int i = 0; i < move_count; i++) {

            game_state new_state = apply_move(state, moves[i]);
            U64 new_occupancy = get_occupancy(new_state.piece_bitboards);

            // ensure move is legal (not putting king in check)
            if (pseudo_to_legal(new_state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, new_occupancy)) {
                // apply negamax !!!from perspective of opponent, so score needs to be negated
                
                //std::cout << !color << std::endl;
                int score = -negamax(new_state, 4, -INF, INF, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, new_occupancy);
                std::cout << score << std::endl;

                // check if it's best move
                if (score > best_score) {
                    best_score = score;
                    best_move_index = i;
                }
            }
        }
        std::cout << best_score << std::endl;
        game_state best_state = apply_move(state, moves[best_move_index]);
        visualize_game_state(best_state);
        std::cout << "move" << std::endl;
        std::cout << moves[best_move_index].piece_index << " " << moves[best_move_index].from_position << " " << moves[best_move_index].to_position << std::endl;
        
        // update state
        color = !color;
        state = best_state;
        occupancy_bitboard = get_occupancy(state.piece_bitboards);

        // other player plays
        // generate pseudo-legal moves
        std::array<move, 256> moves2;
        int move_count2 = pseudo_legal_move_generator(
            moves2, state, color, pawn_move_lookup_table, pawn_attack_lookup_table, 
            knight_lookup_table, bishop_magics, bishop_mask_lookup_table, 
            bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, 
            rook_attack_lookup_table, king_lookup_table, 
            occupancy_bitboard);

        // iterate over all pseudo-legal moves
        for (int i = 0; i < move_count2; i++) {

            game_state new_state = apply_move(state, moves2[i]);
            U64 new_occupancy = get_occupancy(new_state.piece_bitboards);

            // ensure move is legal (not putting king in check)
            if (pseudo_to_legal(new_state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, new_occupancy)) {
                std::cout << "from: " << index_to_chess(moves2[i].from_position) << " to: " << index_to_chess(moves2[i].to_position) << " move index: " << i << std::endl;
            }
        }

        // ask for move
        int player_move;
        std::cout << "Move: ";
        std::cin >> player_move;

        game_state best_state2 = apply_move(state, moves2[player_move]);
        visualize_game_state(best_state2);

        // update state
        color = !color;
        state = best_state2;
        occupancy_bitboard = get_occupancy(state.piece_bitboards);

    }

    return 0;
}