#include <iostream>
#include <array>
#include <vector>
#include <random>
#include <bitset>

// aliases and initialization
using U64 = unsigned long long;
constexpr int MAGIC_TABLE_SIZE = 4096;
constexpr int NUM_SQUARES = 64;
constexpr int NUM_PIECES = 12;

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

struct zobrist_randoms {
    std::array<U64, 768> zobrist_piece_table{};
    U64 zobrist_black_to_move = 0;
    U64 zobrist_w_long_castle = 0;
    U64 zobrist_w_short_castle = 0;
    U64 zobrist_b_long_castle = 0;
    U64 zobrist_b_short_castle = 0;
    std::array<U64, 8> zobrist_en_passant{};

    // default constructor
    zobrist_randoms() = default;
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

struct move_undo {
    U64 zobrist_hash;
    bool w_long_castle;
    bool w_short_castle;
    bool b_long_castle;
    bool b_short_castle;
    bool en_passant;
    U64 en_passant_bitboards[2];
    int captured_piece_index; // -1 if no piece was captured
};

void visualize_game_state_2(const game_state& state);

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
    const U64& occupancy_bitboard);
int pseudo_legal_move_generator(std::array<move, 256>& moves,
    game_state& state, bool color, 
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
    const U64& occupancy_bitboard);
U64 init_zobrist_hashing_mailbox(game_state &state, zobrist_randoms &zobrist, bool color, std::array<int, 64>& piece_on_square);
void apply_move(game_state& state, move& move_to_apply, U64& zobrist_hash, zobrist_randoms &zobrist, move_undo& undo, std::array<int, 64>& piece_on_square);
void undo_move(game_state& state, move& move_to_undo, U64& zobrist_hash, zobrist_randoms &zobrist, move_undo& undo, std::array<int, 64>& piece_on_square);
bool pseudo_to_legal(game_state& state, bool color, 
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
    const U64& occupancy_bitboard);
void generate_lookup_tables( 
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
    std::array<U64, 64>& king_lookup_table);