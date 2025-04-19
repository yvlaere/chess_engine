#include "move_generation.h"
#include <limits>
#include <string>
#include <chrono>

// plain negamax with alpha-beta pruning

constexpr int INF = std::numeric_limits<int>::max() / 2;
constexpr size_t TT_SIZE = 1 << 20;

// temp vars
int tt_hits = 0;
int tt_miss = 0;
int tt_score = 0;
int tt_cutoff = 0;

std::string index_to_chess(int index) {
    if (index < 0 || index >= 64) {
        return ""; // or handle the error as appropriate
    }
    char file = 'a' + (index % 8);      // Files: a-h
    int rank = (index / 8) + 1;           // Ranks: 1-8
    return std::string(1, file) + std::to_string(rank);
}

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

// evaluation

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

// transposition tables

struct transposition_table_entry {
    U64 hash;
    int depth;
    int score;
    int flag; // 0: exact, 1: alpha, 2: beta
    move best_move;
};

// search algorithm

// fast negamax search with alpha-beta pruning.
// 'depth' is the remaining search depth, and alpha-beta parameters prune branches.
int negamax(game_state &state, int depth, int alpha, int beta, bool color, 
    std::array<U64, 128>& pawn_move_lookup_table, 
    std::array<U64, 128>& pawn_attack_lookup_table, 
    std::array<U64, 64>& knight_lookup_table, 
    std::array<U64, 64>& bishop_magics, 
    std::array<U64, 64>& bishop_mask_lookup_table, 
    std::array<U64, 64>& bishop_mask_bit_count,
    std::array<U64, 262144>& bishop_attack_lookup_table, 
    std::array<U64, 64>& rook_magics, 
    std::array<U64, 64>& rook_mask_lookup_table,
    std::array<U64, 64>& rook_mask_bit_count,
    std::array<U64, 262144>& rook_attack_lookup_table, 
    std::array<U64, 64>& king_lookup_table,
    const U64& occupancy_bitboard, int current_depth,
    zobrist_randoms& zobrist, U64& zobrist_hash,
    std::array<std::array<move, 256>, 256>& moves_stack, 
    std::array<move_undo, 256>& undo_stack,
    std::vector<transposition_table_entry>& transposition_table) {

    if (depth == 0) {
        int eval = evaluation(state);
        return color ? -eval : eval;
    }

    // check transposition table for pruning
    U64 TT_index = zobrist_hash & (TT_SIZE - 1);
    transposition_table_entry& entry = transposition_table[TT_index];
    if (entry.hash == zobrist_hash && entry.depth >= depth) {
        tt_hits++;
        if (entry.flag == 0) {
            tt_score++;
            return entry.score;
        }
        else if (entry.flag == 1) {
            alpha = std::max(alpha, entry.score);
        }
        else if (entry.flag == 2) {
            beta = std::min(beta, entry.score);
        }
        if (alpha >= beta) {
            tt_cutoff++;
            return entry.score;
        }
    }
    else {
        tt_miss++;
    }
    
    // generate moves from the current position.
    // generate pseudo-legal moves
    std::array<move, 256>& moves = moves_stack[current_depth];
    int move_count = pseudo_legal_move_generator(
        moves, state, color, pawn_move_lookup_table, pawn_attack_lookup_table, 
        knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count,
        bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count,
        rook_attack_lookup_table, king_lookup_table, 
        occupancy_bitboard);

    int max_score = -INF;
    int best_move_index = -1;
    int legal_moves = 0;

    // iterate over all pseudo-legal moves
    for (int i = 0; i < move_count; i++) {

        move_undo& undo = undo_stack[current_depth];
        apply_move(state, moves[i], zobrist_hash, zobrist, undo);
        U64 new_occupancy = get_occupancy(state.piece_bitboards);

        // ensure move is legal (not putting king in check)
        if (pseudo_to_legal(state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count, rook_attack_lookup_table, king_lookup_table, new_occupancy)) {
            // apply negamax
            int score = -negamax(state, depth - 1, -beta, -alpha, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count, rook_attack_lookup_table, king_lookup_table, new_occupancy, current_depth + 1, zobrist, zobrist_hash, moves_stack, undo_stack, transposition_table);
            legal_moves++;

            if (score > max_score) {
                max_score = score;
                best_move_index = i;
            }
            if (score > alpha) {
                alpha = score;
            }
            if (alpha >= beta) {
                // Undo the move
                undo_move(state, moves[i], zobrist_hash, zobrist, undo);
                break;
            }
        }

        // Undo the move
        undo_move(state, moves[i], zobrist_hash, zobrist, undo);
    }

    // store the result in the transposition table
    entry.hash = zobrist_hash;
    entry.depth = depth;
    entry.score = max_score;
    entry.best_move = moves[best_move_index];
    entry.flag = 0; // exact score
    if (max_score <= alpha) {
        entry.flag = 2; // beta cutoff
    }
    else if (max_score >= beta) {
        entry.flag = 1; // alpha cutoff
    }

    // terminal node: checkmate or stalemate.
    if (legal_moves == 0) {
        // king is attacked: checkmate
        if (pseudo_to_legal(state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count, rook_attack_lookup_table, king_lookup_table, occupancy_bitboard)) {
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
    U64 b_rook = 9295429630892703744ULL;
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
    std::array<U64, 128> pawn_move_lookup_table;
    std::array<U64, 128> pawn_attack_lookup_table; 
    std::array<U64, 64> knight_lookup_table; 
    std::array<U64, 64> bishop_magics;
    std::array<U64, 64> bishop_mask_lookup_table; 
    std::array<U64, 64> bishop_mask_bit_count;
    std::array<U64, 262144> bishop_attack_lookup_table; //2MB, can be on the stack
    std::array<U64, 64> rook_magics; 
    std::array<U64, 64> rook_mask_lookup_table;
    std::array<U64, 64> rook_mask_bit_count;
    std::array<U64, 262144> rook_attack_lookup_table; //2MB, can be on the stack
    std::array<U64, 64> king_lookup_table;

    generate_lookup_tables( pawn_move_lookup_table, pawn_attack_lookup_table, 
        knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count,
        bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count,
        rook_attack_lookup_table, king_lookup_table);

    // create zobrist randoms
    zobrist_randoms zobrist;

    // create move object array
    //untill depth 256
    std::array<std::array<move, 256>, 256> moves_stack;

    // create move undo object array
    std::array<move_undo, 256> undo_stack;

    // create transposition table
    // get transposition table index like this: hash & (TT_SIZE - 1)
    // vector to put it on the heap, otherwize we get a segfault
    std::vector<transposition_table_entry> transposition_table;
    transposition_table.resize(TT_SIZE);

    // initialize
    game_state state = initial_game_state;
    U64 zobrist_hash = init_zobrist_hashing(state, zobrist, false);
    U64 occupancy_bitboard = get_occupancy(state.piece_bitboards);
    int best_score;
    int best_move_index = 0;
    bool color = false;
    int negamax_depth = 7;

    // visualize initial game state
    std::cout << "Initial game state:" << std::endl;
    visualize_game_state(state);

    //std::cout << "evaluation" << std::endl;
    //std::cout << evaluation(initial_game_state) << std::endl;

    //int score = negamax(initial_game_state, 2, -INF, INF, color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, occupancy_bitboard);
    //std::cout << score << std::endl;

    // game loop
    while (true) {
        
        int best_score = -INF;
        int best_move_index = 0;

        // generate pseudo-legal moves
        auto start = std::chrono::high_resolution_clock::now();
        std::array<move, 256> moves;
        int move_count = pseudo_legal_move_generator(
            moves, state, color, pawn_move_lookup_table, pawn_attack_lookup_table, 
            knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count,
            bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count,
            rook_attack_lookup_table, king_lookup_table, 
            occupancy_bitboard);

        // iterate over all pseudo-legal moves
        for (int i = 0; i < move_count; i++) {

            // negamax needs to start at depth 1 because depth 0 is here
            move_undo& undo = undo_stack[0];
            apply_move(state, moves[i], zobrist_hash, zobrist, undo);
            U64 new_occupancy = get_occupancy(state.piece_bitboards);

            // ensure move is legal (not putting king in check)
            if (pseudo_to_legal(state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count, rook_attack_lookup_table, king_lookup_table, new_occupancy)) {
                // apply negamax !!!from perspective of opponent, so score needs to be negated

                int score = -negamax(state, negamax_depth, -INF, INF, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count, rook_attack_lookup_table, king_lookup_table, new_occupancy, 1, zobrist, zobrist_hash, moves_stack, undo_stack, transposition_table);

                // check if it's best move
                if (score > best_score) {
                    best_score = score;
                    best_move_index = i;
                }
            }

            // Undo the move
            undo_move(state, moves[i], zobrist_hash, zobrist, undo);
        }
        std::cout << "Best score: " << best_score << std::endl;
        apply_move(state, moves[best_move_index], zobrist_hash, zobrist, undo_stack[0]);
        std::cout << "Best move: piece index: " << moves[best_move_index].piece_index << " from: " << moves[best_move_index].from_position << " to: " << moves[best_move_index].to_position << std::endl;
        visualize_game_state(state);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        std::cout << "Time taken: " << duration.count() << " ms" << std::endl;
        std::cout << "TT hits: " << tt_hits << std::endl;
        std::cout << "TT misses: " << tt_miss << std::endl;
        std::cout << "TT score: " << tt_score << std::endl;
        std::cout << "TT cutoff: " << tt_cutoff << std::endl;

        // update state
        color = !color;
        occupancy_bitboard = get_occupancy(state.piece_bitboards);

        // other player plays
        // generate pseudo-legal moves
        std::array<move, 256> moves2;
        int move_count2 = pseudo_legal_move_generator(
            moves2, state, color, pawn_move_lookup_table, pawn_attack_lookup_table, 
            knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count,
            bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count,
            rook_attack_lookup_table, king_lookup_table, 
            occupancy_bitboard);

        // iterate over all pseudo-legal moves
        for (int i = 0; i < move_count2; i++) {

            apply_move(state, moves2[i], zobrist_hash, zobrist, undo_stack[0]);
            U64 new_occupancy = get_occupancy(state.piece_bitboards);

            // ensure move is legal (not putting king in check)
            if (pseudo_to_legal(state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count, rook_attack_lookup_table, king_lookup_table, new_occupancy)) {
                std::cout << "from: " << index_to_chess(moves2[i].from_position) << " to: " << index_to_chess(moves2[i].to_position) << " move index: " << i << std::endl;
            }

            // Undo the move
            undo_move(state, moves2[i], zobrist_hash, zobrist, undo_stack[0]);
        }

        // ask for move
        int player_move;
        std::cout << "Move: ";
        std::cin >> player_move;

        apply_move(state, moves2[player_move], zobrist_hash, zobrist, undo_stack[0]);
        visualize_game_state(state);

        // update state
        color = !color;
        occupancy_bitboard = get_occupancy(state.piece_bitboards);

    }

    return 0;
}