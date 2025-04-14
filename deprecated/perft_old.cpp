#include "move_generation_2.h"
#include <iostream>
#include <array>
#include <chrono>

// debugging functions

void print_bitboard(const U64 bitboard) {
    // Loop over ranks from 8 (top) to 1 (bottom)
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            // Calculate the bit index. For a1 = bit0, we use:
            // square = rank * 8 + file
            int square = rank * 8 + file;
            // Check if the bit at 'square' is set.
            if (bitboard & (1ULL << square))
                std::cout << "X ";
            else
                std::cout << ". ";
        }
        std::cout << '\n';
    }
}

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

// perft

void perft(game_state& state, int depth, bool color, 
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
    const U64& occupancy_bitboard,
    std::vector<uint64_t>& depth_nodes, int current_depth, 
    std::vector<uint64_t>& captures, std::vector<uint64_t>& promotions, 
    std::vector<uint64_t>& castlings, std::array<int, 6>& piece_capture,
    zobrist_randoms& zobrist, U64& zobrist_hash, 
    std::array<std::array<move, 256>, 256>& moves_stack, 
    std::array<move_undo, 256>& undo_stack) {

    if (depth == 0) {
        // depth_nodes[current_depth]++;
        return;
    }

    // Generate pseudo-legal moves
    std::array<move, 256>& moves = moves_stack[current_depth];
    int move_count = pseudo_legal_move_generator(moves, 
        state, color, pawn_move_lookup_table, pawn_attack_lookup_table, 
        knight_lookup_table, bishop_magics, bishop_mask_lookup_table, 
        bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, 
        rook_attack_lookup_table, king_lookup_table, 
        occupancy_bitboard);

    //std::cout << "Current depth: " << current_depth << "Move count: " << move_count << std::endl;

    // iterate over all pseudo-legal moves
    for (int i = 0; i < move_count; i++) {
        if (moves[i].piece_index != -1) {
            move_undo& undo = undo_stack[current_depth];

            //if (current_depth == 0) {
                //std::cout << "before application" << std::endl;
                //visualize_game_state(state);
            //}

            apply_move(state, moves[i], zobrist_hash, zobrist, undo);

            //if (current_depth == 0) {
                //std::cout << "after application" << std::endl;
                //visualize_game_state(state);
            //}
            
            // Ensure move is legal (not putting king in check)
            if (pseudo_to_legal(state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, get_occupancy(state.piece_bitboards))) {
                
                // count captures
                U64 new_occupancy_bitboard = get_occupancy(state.piece_bitboards);
                std::vector<int> old_count = get_set_bit_positions(occupancy_bitboard);
                std::vector<int> new_count = get_set_bit_positions(new_occupancy_bitboard);
                
                if (old_count.size() != new_count.size()) {
                    captures[current_depth]++;
                    //if (current_depth == 0) {
                        //std::cout << "capture" << std::endl;
                        //visualize_game_state(state);
                    //}
                }
                if (current_depth == 0) {
                    piece_capture[moves[i].piece_index - 6*color]++;
                }
                if (moves[i].promotion_piece_index != moves[i].piece_index) {
                    promotions[current_depth]++;
                }
                if (moves[i].castling) {
                    castlings[current_depth]++;
                }
                
                perft(state, depth - 1, !color, pawn_move_lookup_table, 
                    pawn_attack_lookup_table, knight_lookup_table, bishop_magics, 
                    bishop_mask_lookup_table, bishop_attack_lookup_table, rook_magics, 
                    rook_mask_lookup_table, rook_attack_lookup_table, king_lookup_table, 
                    new_occupancy_bitboard, depth_nodes, current_depth + 1, captures, 
                    promotions, castlings, piece_capture, zobrist, zobrist_hash,
                    moves_stack, undo_stack);
                depth_nodes[current_depth]++;
            }

            // Undo the move
            undo_move(state, moves[i], zobrist_hash, zobrist, undo);
        }
    }
}

//rename to something else for inclusion in other scripts
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

    generate_lookup_tables(pawn_move_lookup_table, pawn_attack_lookup_table, 
        knight_lookup_table, bishop_magics, bishop_mask_lookup_table, 
        bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table,
        rook_attack_lookup_table, king_lookup_table);

    // create zobrist randoms
    zobrist_randoms zobrist;

    // create move object array
    //untill depth 256
    std::array<std::array<move, 256>, 256> moves_stack;

    // create move undo object array
    std::array<move_undo, 256> undo_stack;
    
    // initialize game state
    std::cout << "Position 1" << std::endl;
    visualize_game_state(initial_game_state);

    // alternative game states
    std::array<U64, 12> piece_bitboards2 = {34628232960, 68719738880, 6144, 129, 2097152, 16, 12754334924144640, 37383395344384, 18015498021109760, 9295429630892703744, 4503599627370496, 1152921504606846976};
    std::array<U64, 2> en_passant_bitboards2 = {w_en_passant, b_en_passant};
    game_state game_state2(piece_bitboards2, en_passant_bitboards2, w_long_castle, w_short_castle, b_long_castle, b_short_castle);

    std::cout << "Position 2" << std::endl;
    visualize_game_state(game_state2);

    // alternative game states
    std::array<U64, 12> piece_bitboards3 = {8589955072, 0, 0, 33554432, 0, 4294967296, 1134696536735744, 0, 0, 549755813888, 0, 2147483648};
    std::array<U64, 2> en_passant_bitboards3 = {w_en_passant, b_en_passant};
    game_state game_state3(piece_bitboards3, en_passant_bitboards3, false, false, false, false);

    std::cout << "Position 3" << std::endl;
    visualize_game_state(game_state3);

    // alternative game states
    std::array<U64, 12> piece_bitboards4 = {281483902241024, 140737490452480, 50331648, 33, 8, 64, 66991044457136640, 35188667056128, 72567767433216, 9295429630892703744, 65536, 1152921504606846976};
    std::array<U64, 2> en_passant_bitboards4 = {w_en_passant, b_en_passant};
    game_state game_state4(piece_bitboards4, en_passant_bitboards4, false, false, true, true);

    std::cout << "Position 4" << std::endl;
    visualize_game_state(game_state4);

    // alternative game states
    std::array<U64, 12> piece_bitboards5 = {2251799813736192, 4098, 67108868, 129, 8, 16, 63899217759830016, 144115188075864064, 292733975779082240, 9295429630892703744, 576460752303423488, 2305843009213693952};
    std::array<U64, 2> en_passant_bitboards5 = {w_en_passant, b_en_passant};
    game_state game_state5(piece_bitboards5, en_passant_bitboards5, true, true, false, false);

    std::cout << "Position 5" << std::endl;
    visualize_game_state(game_state5);

    // alternative game states
    std::array<U64, 12> piece_bitboards6 = {269084160, 2359296, 274945015808, 33, 4096, 64, 64749208967577600, 39582418599936, 18253611008, 2377900603251621888, 4503599627370496, 4611686018427387904};
    std::array<U64, 2> en_passant_bitboards6 = {w_en_passant, b_en_passant};
    game_state game_state6(piece_bitboards6, en_passant_bitboards6, false, false, false, false);

    std::cout << "Position 6" << std::endl;
    visualize_game_state(game_state6);

    // perft
    std::vector<uint64_t> depth_nodes(6, 0);
    std::vector<uint64_t> captures(6, 0);
    std::vector<uint64_t> promotions(6, 0);
    std::vector<uint64_t> castlings(6, 0);
    std::array<int, 6> piece_capture = {0, 0, 0, 0, 0, 0};

    game_state perft_state = initial_game_state;
    U64 zobrist_hash = init_zobrist_hashing(perft_state, zobrist, false);

    auto start = std::chrono::high_resolution_clock::now();

    perft(perft_state, 5, false, pawn_move_lookup_table, pawn_attack_lookup_table, 
          knight_lookup_table, bishop_magics, bishop_mask_lookup_table, 
          bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, 
          rook_attack_lookup_table, king_lookup_table, 
          get_occupancy(perft_state.piece_bitboards), depth_nodes, 0, 
          captures, promotions, castlings, piece_capture, zobrist, zobrist_hash, 
          moves_stack, undo_stack);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "Time taken: " << duration.count() << " ms" << std::endl;

    // Print the perft results per depth
    for (int d = 0; d <= 5; d++) {
        std::cout << "Nodes at depth " << d << ": " << depth_nodes[d] << " Captures: " << captures[d] << " Promotions: " << promotions[d] << " Castlings: " << castlings[d] << std::endl;
    }

    for (int i = 0; i < 6; i++) {
        std::cout << "Piece: " << i << " Captures: " << piece_capture[i] << std::endl;
    }

    return 0;
}