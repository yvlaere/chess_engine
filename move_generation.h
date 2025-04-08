#include <iostream>
#include <array>
#include <vector>
#include <random>
#include <bitset>

// aliases and initialization
using U64 = unsigned long long;

struct game_state {
    std::array<U64, 12> piece_bitboards;
    std::array<U64, 2> en_passant_bitboards;
    bool w_long_castle;
    bool w_short_castle;
    bool b_long_castle;
    bool b_short_castle;

    // constructor
    game_state(const std::array<U64, 12>& piece_bb, 
        const std::array<U64, 2>& en_passant_bb,
        bool wlc, bool wsc, bool blc, bool bsc) 
        : piece_bitboards(piece_bb),
        en_passant_bitboards(en_passant_bb),
        w_long_castle(wlc),
        w_short_castle(wsc),
        b_long_castle(blc),
        b_short_castle(bsc) {}
    };

struct move {
    int piece_index;
    int from_position;
    int to_position;
    int promotion_piece_index;
    bool en_passantable;
    bool castling;

    // default constructor
    move() : piece_index(-1), from_position(-1), to_position(-1), promotion_piece_index(-1), en_passantable(false), castling(false) {}

    // constructor
    move(int piece_index, int from_position, int to_position, 
        int promotion_piece_index, bool en_passantable, bool castling) 
       : piece_index(piece_index), from_position(from_position), to_position(to_position), 
         promotion_piece_index(promotion_piece_index), en_passantable(en_passantable), castling(castling) {}
};

// usefull functions

U64 combine_white(const std::array<U64, 12>& piece_bitboards);
U64 combine_black(const std::array<U64, 12>& piece_bitboards);
U64 get_occupancy(const std::array<U64, 12>& piece_bitboards);
std::vector<int> get_set_bit_positions(U64 bitboard);
int pop_lsb(U64& bitboard);
int count_set_bits(U64 bitboard);
bool is_bit_set(U64 bitboard, int position);

// magic bitboards

std::vector<U64> get_blocker_boards(int position, U64 mask_bitboard);
U64 generate_candidate_magic();
void generate_magics(int position, U64 mask_bitboard, std::vector<U64> blocker_boards, 
    std::vector<U64> attack_bitboards, U64& candidate_magic, std::array<U64, 4096>& lookup_table);

// pawn

U64 get_pawn_move(int position, bool color);
U64 get_pawn_attack(int position, bool color);

// knight

U64 get_knight_attack(int position);

// bishop

U64 get_bishop_mask(int position);
U64 get_bishop_attack(int position, U64 bishop_blocker_bitboard);

// rook

U64 get_rook_mask(int position);
U64 get_rook_attack(int position, U64 rook_blocker_bitboard);

// king

U64 get_king_attack(int position);

// move generation

U64 attacked(game_state state, bool color, 
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
    const U64& occupancy_bitboard);
int pseudo_legal_move_generator(std::array<move, 256>& moves,
    game_state& state, bool color, 
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
    const U64& occupancy_bitboard);
game_state apply_move(game_state& state, move& move_to_apply);
bool pseudo_to_legal(game_state& state, bool color, 
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
    const U64& occupancy_bitboard);
void generate_lookup_tables( 
    std::array<std::array<U64, 64>, 2>& pawn_move_lookup_table, 
    std::array<std::array<U64, 64>, 2>& pawn_attack_lookup_table, 
    std::array<U64, 64>& knight_lookup_table, 
    std::array<U64, 64>& bishop_magics, 
    std::array<U64, 64>& bishop_mask_lookup_table, 
    std::array<std::array<U64, 4096>, 64>& bishop_attack_lookup_table, 
    std::array<U64, 64>& rook_magics, 
    std::array<U64, 64>& rook_mask_lookup_table,
    std::array<std::array<U64, 4096>, 64>& rook_attack_lookup_table, 
    std::array<U64, 64>& king_lookup_table);