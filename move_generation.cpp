#include "move_generation.h"

// usefull functions

U64 combine_white(const std::array<U64, 12>& piece_bitboards) {
    U64 combined = 0;
    for (int i = 0; i < 6; i++) {
        combined |= piece_bitboards[i];
    }
    return combined;
}

U64 combine_black(const std::array<U64, 12>& piece_bitboards) {
    U64 combined = 0;
    for (int i = 6; i < 12; i++) {
        combined |= piece_bitboards[i];
    }
    return combined;
}

U64 get_occupancy(const std::array<U64, 12>& piece_bitboards) {
    U64 combined = 0;
    for (int i = 0; i < 12; i++) {
        combined |= piece_bitboards[i];
    }
    return combined;
}

// this can return a vector, because it only runs on startup
std::vector<int> get_set_bit_positions(U64 bitboard) {
    // function to get the positions of set bits in a U64 bitboard

    std::vector<int> positions;
    while (bitboard) {

        // isolate the least significant set bit
        U64 lsb = bitboard & -bitboard;

        // get the position of the least significant set bit
        int position = __builtin_ctzll(bitboard);

        // add the position to the list
        positions.push_back(position);

        // remove the least significant set bit from the bitboard
        bitboard &= (bitboard - 1);
    }
    return positions;
}

// this can not return a vector, because it is called during the move generation
inline int pop_lsb(U64& bitboard) {
    // function to pop the least significant set bit in a U64 bitboard
    // the bitboard is passed by reference, so it gets modified

    int lsb_index = __builtin_ctzll(bitboard);

    // remove the least significant set bit from the bitboard
    bitboard &= (bitboard - 1);

    return lsb_index;
}

inline int count_set_bits(U64 bitboard) {
    // returns the number of set bits in the bitboard
    return __builtin_popcountll(bitboard);
}

inline bool is_bit_set(U64 bitboard, int position) {
    // returns true if the bit at the given position is set in the bitboard
    return bitboard & (1ULL << position);
}

// magic bitboards

// this can return a vector, because it only runs on startup
std::vector<U64> get_blocker_boards(int position, U64 mask_bitboard) {
    // generate all possible blocker boards for a given position and blocker board
    // usable for both rook and bishop

    // initialize
    std::vector<U64> blocker_bitboards;

    // get integer positions of set bits in the mask bitboard
    std::vector<int> mask_positions = get_set_bit_positions(mask_bitboard);

    // iterate over all possible subsets of mask positions
    for (int i = 0; i < (1 << mask_positions.size()); i++) {
        U64 blocker_bitboard = 0;
        for (int j = 0; j < mask_positions.size(); j++) {
            if (i & (1 << j)) {
                blocker_bitboard |= 1ULL << mask_positions[j];
            }
        }
        blocker_bitboards.push_back(blocker_bitboard);
    }
    return blocker_bitboards;
}

U64 generate_candidate_magic() {
    // generate a random 64-bit number with a low amount of set bits

    // generate a seed
    std::random_device rd;
    std::mt19937_64 rng(rd());

    // initialize the pseudo-random number generator
    static std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    // reduce the amount of set bits by combining random numbers
    return dist(rng) & dist(rng) & dist(rng);
}

void generate_magics(int position, U64 mask_bitboard, std::vector<U64> blocker_boards, std::vector<U64> attack_bitboards, U64& candidate_magic, std::array<U64, 262144>& lookup_table) {
    // generate candidate magic numbers and lookup tables for a given position

    // initialize
    int index_bits = count_set_bits(mask_bitboard);

    for (int i = 0; i < 100000000; i++) {
        // generate a candidate magic number
        candidate_magic = generate_candidate_magic();

        // initialize
        bool valid = true;

        // initialize the lookup table
        for (int j = 0; j < MAGIC_TABLE_SIZE; j++) {
            lookup_table[position*MAGIC_TABLE_SIZE + j] = 0;
        }

        // fill the lookup table
        for (int j = 0; j < blocker_boards.size(); j++) {
            U64 index = (blocker_boards[j] * candidate_magic) >> (64 - index_bits);
            if (lookup_table[position*MAGIC_TABLE_SIZE + index] == 0) {
                lookup_table[position*MAGIC_TABLE_SIZE + index] = attack_bitboards[j];
            }
            else if (lookup_table[position*MAGIC_TABLE_SIZE + index] != attack_bitboards[j]) {
                valid = false;
                break;
            }
        }

        // stop looking if a valid magic number is found
        if (valid) {
            break;
        }
    }
}

// pawn

U64 get_pawn_move(int position, bool color) {
    // returns a bitboard with one or two squares in front of the pawn set

    // initialize
    U64 pawn_move_bitboard = 0;
    int direction;

    if (color) {
        direction = -1;
    }
    else {
        direction = 1;
    }

    // check if pawn is on starting rank
    if (((8 + color*40) <= position) && (position < (16 + color*40))) {
        std::array<int, 2> moves = {8, 16};

        for (int move : moves) {
            if (position + move*direction >= 0 && position + move*direction < 64) {
                pawn_move_bitboard |= 1ULL << (position + move*direction);
            }
        }
    }
    else {
        std::array<int, 1> moves = {8};

        for (int move : moves) {
            if (position + move*direction >= 0 && position + move*direction < 64) {
                pawn_move_bitboard |= 1ULL << (position + move*direction);
            }
        }
    }

    return pawn_move_bitboard;
}

U64 get_pawn_attack(int position, bool color) {
    // returns a bitboard with the two squares diagonally in front of the pawn set

    // initialize
    U64 pawn_attack_bitboard = 0;
    int direction;
    std::array<int, 2> moves = {7, 9};

    if (color) {
        direction = -1;
    }
    else {
        direction = 1;
    }

    for (int move : moves) {
        if ((position + move*direction >= 0) && (position + move*direction < 64)) {
            if ((!color) && ((position % 8 == 0 && move == 7) || (position % 8 == 7 && move == 9))) {
                
                // the move is not allowed to happen
                continue;
            }
            else if ((color) && ((position % 8 == 0 && move == 9) || (position % 8 == 7 && move == 7))) {
                
                // the move is not allowed to happen
                continue;
            }
            else {
                // the move happens
                pawn_attack_bitboard |= 1ULL << (position + move*direction);
            }
        }
    }

    return pawn_attack_bitboard;
}

// knight

U64 get_knight_attack(int position) {
    // returns a bitboard with all the possible knight moves starting from the given position

    // initialize
    U64 knight_attack_bitboard = 0;

    // positions for knight moves
    // these indicate what the possible positions of a knight are relative to a given position (input)
    int knight_moves[8] = {-17, -15, -10, -6, 6, 10, 15, 17};

    for (int move : knight_moves) {

        // check if the move is valid rankwise
        if (position + move >= 0 && position + move < 64) {

            // check if the move is valid filewise
            if ((position % 8 == 0 && move == -17) || 
                (position % 8 == 0 && move == -10) || 
                (position % 8 == 0 && move == 6) || 
                (position % 8 == 0 && move == 15) ||
                (position % 8 == 1 && move == -10) || 
                (position % 8 == 1 && move == 6) || 
                (position % 8 == 7 && move == 17) || 
                (position % 8 == 7 && move == 10) || 
                (position % 8 == 7 && move == -6) || 
                (position % 8 == 7 && move == -15) || 
                (position % 8 == 6 && move == 10) || 
                (position % 8 == 6 && move == -6)) {
                
                // the move is not allowed to happen
                continue;
            }
            else
            {
                // the move happens
                knight_attack_bitboard |= 1ULL << (position + move);
            }
        }
    }
    return knight_attack_bitboard;
}

// bishop

U64 get_bishop_mask(int position) {
    // returns a bitboard with all the positions that could block a bishop given the position of the bishop

    // initialize
    U64 bishop_mask_bitboard = 0;

    // top left
    // increase values by 7, untill A or H file is reached
    int top_left = position + 7;
    while (top_left % 8 != 0 && top_left % 8 != 7 && top_left < 56) {
        bishop_mask_bitboard |= 1ULL << (top_left);
        top_left += 7;
    }

    // top right
    // increase values by 9, untill A or H file is reached
    int top_right = position + 9;
    while (top_right % 8 != 0 && top_right % 8 != 7 && top_right < 56) {
        bishop_mask_bitboard |= 1ULL << (top_right);
        top_right += 9;
    }

    // bottom left
    // decrease values by 9, untill A or H file is reached
    int bottom_left = position - 9;
    while (bottom_left % 8 != 0 && bottom_left % 8 != 7 && bottom_left >= 8) {
        bishop_mask_bitboard |= 1ULL << (bottom_left);
        bottom_left -= 9;
    }

    // bottom right
    // decrease values by 7, untill A or H file is reached
    int bottom_right = position - 7;
    while (bottom_right % 8 != 0 && bottom_right % 8 != 7 && bottom_right >= 8) {
        bishop_mask_bitboard |= 1ULL << (bottom_right);
        bottom_right -= 7;
    }

    return bishop_mask_bitboard;
}

U64 get_bishop_attack(int position, U64 bishop_blocker_bitboard) {
    // calculates the attack bitboard for a given blocker bitboard for the bishop

    // initialize
    U64 bishop_attack_bitboard = 0;

    // top left
    int top_left = position + 7;
    while (top_left % 8 != 0 && top_left % 8 != 7 && top_left < 64) {
        if (is_bit_set(bishop_blocker_bitboard, top_left)) {
            bishop_attack_bitboard |= 1ULL << top_left;
            break;
        }
        else {
            bishop_attack_bitboard |= 1ULL << top_left;
        }
        top_left += 7;
    }

    if (top_left < 64 && position % 8 != 0) {
        bishop_attack_bitboard |= 1ULL << top_left;
    }

    // top right
    int top_right = position + 9;
    while (top_right % 8 != 0 && top_right % 8 != 7 && top_right < 64) {
        if (is_bit_set(bishop_blocker_bitboard, top_right)) {
            bishop_attack_bitboard |= 1ULL << top_right;
            break;
        }
        else {
            bishop_attack_bitboard |= 1ULL << top_right;
        }
        top_right += 9;
    }

    if (top_right < 64 && position % 8 != 7) {
        bishop_attack_bitboard |= 1ULL << top_right;
    }

    // bottom left
    int bottom_left = position - 9;
    while (bottom_left % 8 != 0 && bottom_left % 8 != 7 && bottom_left >= 0) {
        if (is_bit_set(bishop_blocker_bitboard, bottom_left)) {
            bishop_attack_bitboard |= 1ULL << bottom_left;
            break;
        }
        else {
            bishop_attack_bitboard |= 1ULL << bottom_left;
        }
        bottom_left -= 9;
    }

    if (bottom_left >= 0 && position % 8 != 0) {
        bishop_attack_bitboard |= 1ULL << bottom_left;
    }

    // bottom right
    int bottom_right = position - 7;
    while (bottom_right % 8 != 0 && bottom_right % 8 != 7 && bottom_right >= 0) {
        if (is_bit_set(bishop_blocker_bitboard, bottom_right)) {
            bishop_attack_bitboard |= 1ULL << bottom_right;
            break;
        }
        else {
            bishop_attack_bitboard |= 1ULL << bottom_right;
        }
        bottom_right -= 7;
    }

    if (bottom_right >= 0 && position % 8 != 7) {
        bishop_attack_bitboard |= 1ULL << bottom_right;
    }

    return bishop_attack_bitboard;
}

// rook

U64 get_rook_mask(int position) {
    // returns a bitboard with all the positions that could block a rook given the position of the rook
    
    // initialize
    U64 rook_mask_bitboard = 0;

    // above
    int above = position + 8;
    while (above < 56) {
        rook_mask_bitboard |= 1ULL << above;
        above += 8;
    }

    // below
    int below = position - 8;
    while (below >= 8) {
        rook_mask_bitboard |= 1ULL << below;
        below -= 8;
    }

    // left
    int left = position % 8;
    for (int i = 1; i < left; i++) {
        rook_mask_bitboard |= 1ULL << (position - i);
    }

    // right
    int right = 7 - (position % 8);
    for (int i = 1; i < right; i++) {
        rook_mask_bitboard |= 1ULL << (position + i);
    }

    return rook_mask_bitboard;
}

U64 get_rook_attack(int position, U64 rook_blocker_bitboard) {
    // calculates the attack bitboard for a given blocker bitboard for the rook

    // initialize
    U64 rook_attack_bitboard = 0;

    //above
    int above = position + 8;
    while (above < 64) {
        if (is_bit_set(rook_blocker_bitboard, above)) {
            rook_attack_bitboard |= 1ULL << above;
            break;
        }
        else {
            rook_attack_bitboard |= 1ULL << above;
        }
        above += 8;
    }

    // below
    int below = position - 8;
    while (below >= 0) {
        if (is_bit_set(rook_blocker_bitboard, below)) {
            rook_attack_bitboard |= 1ULL << below;
            break;
        }
        else {
            rook_attack_bitboard |= 1ULL << below;
        }
        below -= 8;
    }

    // left
    int left = position % 8;
    for (int i = 1; i <= left; i++) {
        int left_pos = position - i;
        if (is_bit_set(rook_blocker_bitboard, left_pos)) {
            rook_attack_bitboard |= 1ULL << left_pos;
            break;
        }
        else {
            rook_attack_bitboard |= 1ULL << left_pos;
        }
    }

    // right
    int right = 7 - (position % 8);
    for (int i = 1; i <= right; i++) {
        int right_pos = position + i;
        if (is_bit_set(rook_blocker_bitboard, right_pos)) {
            rook_attack_bitboard |= 1ULL << right_pos;
            break;
        }
        else {
            rook_attack_bitboard |= 1ULL << right_pos;
        }
    }

    return rook_attack_bitboard;
}

// king

U64 get_king_attack(int position) {
    // returns a bitboard with all the possible king moves starting from the given position

    // initialize
    U64 king_attack_bitboard = 0;
    std::array<int, 8> moves = {-9, -8, -7, -1, 1, 7, 8, 9};

    for (int move : moves) {
        if ((position + move >= 0) && (position + move < 64)) {
            if ((position % 8 == 0 && move == 7) || 
                (position % 8 == 0 && move == -1) ||
                (position % 8 == 0 && move == -9) ||
                (position % 8 == 7 && move == 9) ||
                (position % 8 == 7 && move == 1) ||
                (position % 8 == 7 && move == -7)) {
                
                // the move is not allowed to happen
                continue;
            }
            else
            {
                // the move happens
                king_attack_bitboard |= 1ULL << (position + move);
            }
        }
    }

    return king_attack_bitboard;
}

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
    const U64& occupancy_bitboard) {
    // return a bitboard of all attacked positions by the given color
    
    // initialize
    U64 attacked_bitboard = 0;

    // iterate over all pieces
    for (int i = 0; i < 6; i++) {

        // get the correct piece bitboard
        U64 piece_bitboard = state.piece_bitboards[i + 6*color];
        while (piece_bitboard) {

            // get the position of the least significant set bit and remove it from the bitboard
            int position = pop_lsb(piece_bitboard);
        
            // get the possible moves for the piece
            U64 possible_moves = 0;
            switch (i) {
                // pawn
                case 0: {
                    possible_moves = pawn_attack_lookup_table[position + NUM_SQUARES*color];
                    break;
                }

                // knight
                case 1: {
                    possible_moves = knight_lookup_table[position];
                    break;
                }

                // bishop
                case 2: {
                    U64 bishop_blocker_bitboard = bishop_mask_lookup_table[position] & occupancy_bitboard;
                    int index = (bishop_blocker_bitboard * bishop_magics[position]) >> (64 - bishop_mask_bit_count[position]);
                    possible_moves = bishop_attack_lookup_table[ position*MAGIC_TABLE_SIZE + index];
                    break;
                }

                // rook
                case 3: {
                    U64 rook_blocker_bitboard = rook_mask_lookup_table[position] & occupancy_bitboard;
                    int index = (rook_blocker_bitboard * rook_magics[position]) >> (64 - rook_mask_bit_count[position]);
                    possible_moves = rook_attack_lookup_table[ position*MAGIC_TABLE_SIZE + index];
                    break;
                }
                
                // queen
                case 4: {
                    // bishop aspect
                    U64 bishop_blocker_bitboard = bishop_mask_lookup_table[position] & occupancy_bitboard;
                    int index = (bishop_blocker_bitboard * bishop_magics[position]) >> (64 - bishop_mask_bit_count[position]);
                    U64 possible_bishop_moves = bishop_attack_lookup_table[ position*MAGIC_TABLE_SIZE + index];

                    // rook aspect
                    U64 rook_blocker_bitboard = rook_mask_lookup_table[position] & occupancy_bitboard;
                    index = (rook_blocker_bitboard * rook_magics[position]) >> (64 - rook_mask_bit_count[position]);
                    U64 possible_rook_moves = rook_attack_lookup_table[ position*MAGIC_TABLE_SIZE + index];
                    
                    //combine
                    possible_moves = possible_bishop_moves | possible_rook_moves;
                    break;
                }

                // king
                case 5: {
                    possible_moves = king_lookup_table[position];
                    break;
                }
            }

            // add possible_moves to the attacked_bitboard
            attacked_bitboard |= possible_moves;
        }
    }

    return attacked_bitboard;
}

int pseudo_legal_move_generator(std::array<move, 256>& moves, game_state& state, bool color, 
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
    const U64& occupancy_bitboard) {
    // generate all pseudo legal moves for the given color in the given game state
    // color is the color of the attacker

    // initialize
    int move_index = 0;

    // iterate over all pieces
    for (int i = 0; i < 6; i++) {

        // get the correct piece bitboard
        U64 piece_bitboard = state.piece_bitboards[i + 6*color];
        while (piece_bitboard) {

            // get the position of the least significant set bit and remove it from the bitboard
            int position = pop_lsb(piece_bitboard);
        
            // get the possible moves for the piece
            U64 possible_moves = 0;
            bool promotion = false;
            bool en_passantable = false;

            switch (i) {
                // pawn
                case 0: {
                    
                    U64 pawn_move_bitboard = 0;

                    // check if pawn is not blocked
                    if (!(occupancy_bitboard & (1ULL << position + 8 - 16*color))) {
                        // get normal pawn moves
                        pawn_move_bitboard = pawn_move_lookup_table[position + NUM_SQUARES*color];
                        // only keep moves that are not blocked
                        pawn_move_bitboard = pawn_move_bitboard & ~occupancy_bitboard;

                        // set en passant bool
                        // this operation checks if the pawn move bitboard has more than one set bit
                        en_passantable = pawn_move_bitboard & (pawn_move_bitboard - 1);
                    }

                    // get pawn attack moves
                    U64 pawn_attack_bitboard = pawn_attack_lookup_table[position + NUM_SQUARES*color];
                    // only keep moves that capture something
                    pawn_attack_bitboard = pawn_attack_bitboard & (occupancy_bitboard | state.en_passant_bitboards[!color]);
                    
                    // set promotion bool
                    if (((48 - color*40) <= position) and (position < (56 - color*40))) {
                        promotion = true;
                    }

                    // combine the move and attack bitboards
                    possible_moves = pawn_move_bitboard | pawn_attack_bitboard;
                    break;
                }

                // knight
                case 1: {
                    possible_moves = knight_lookup_table[position];
                    break;
                }

                // bishop
                case 2: {
                    
                    U64 bishop_blocker_bitboard = bishop_mask_lookup_table[position] & occupancy_bitboard;
                    int index = (bishop_blocker_bitboard * bishop_magics[position]) >> (64 - bishop_mask_bit_count[position]);
                    possible_moves = bishop_attack_lookup_table[position*MAGIC_TABLE_SIZE + index];
                    break;
                }

                // rook
                case 3: {
                    U64 rook_blocker_bitboard = rook_mask_lookup_table[position] & occupancy_bitboard;
                    int index = (rook_blocker_bitboard * rook_magics[position]) >> (64 - rook_mask_bit_count[position]);
                    possible_moves = rook_attack_lookup_table[position*MAGIC_TABLE_SIZE + index];
                    break;
                }
                
                // queen
                case 4: {
                    // bishop aspect
                    U64 bishop_blocker_bitboard = bishop_mask_lookup_table[position] & occupancy_bitboard;
                    int index = (bishop_blocker_bitboard * bishop_magics[position]) >> (64 - bishop_mask_bit_count[position]);
                    U64 possible_bishop_moves = bishop_attack_lookup_table[position*MAGIC_TABLE_SIZE + index];

                    // rook aspect
                    U64 rook_blocker_bitboard = rook_mask_lookup_table[position] & occupancy_bitboard;
                    index = (rook_blocker_bitboard * rook_magics[position]) >> (64 - rook_mask_bit_count[position]);
                    U64 possible_rook_moves = rook_attack_lookup_table[position*MAGIC_TABLE_SIZE + index];
                    
                    //combine
                    possible_moves = possible_bishop_moves | possible_rook_moves;
                    break;
                }

                // king
                case 5: {
                    possible_moves = king_lookup_table[position];
                    break;
                }
            }

            // filter out capturing of own pieces
            if (color) {
                possible_moves = possible_moves & ~combine_black(state.piece_bitboards);
            }
            else {
                possible_moves = possible_moves & ~combine_white(state.piece_bitboards);
            }

            // turn the possible_moves bitboard into an array of moves
            while (possible_moves) {

                // get the position of the least significant set bit
                int move_position = pop_lsb(possible_moves);

                // check if the move creates an en passantable pawn
                if (en_passantable && (std::abs(position - move_position) > 9)) {
                    // validly en passantable
                    moves[move_index] = move(i + 6*color, position, move_position, i + 6*color, true, false);
                    move_index++;
                }

                // check if the move is a promotion
                else if (promotion) {
                    for (int j = 1; j < 5; j++) {
                        moves[move_index] = move(i + 6*color, position, move_position, j + 6*color, false, false);
                        move_index++;
                    }
                }

                // normal moves
                else {
                    moves[move_index] = move(i + 6*color, position, move_position, i + 6*color, false, false);
                    move_index++;
                }
            }
        }
    }

    // castling

    bool long_castle;
    bool short_castle;
    U64 long_castle_occupation_mask = 0;
    U64 short_castle_occupation_mask = 0;
    U64 long_castle_check_mask = 0;
    U64 short_castle_check_mask = 0;
    
    if (color) {
        long_castle = state.b_long_castle;
        short_castle = state.b_short_castle;
        long_castle_occupation_mask = 1008806316530991104;
        short_castle_occupation_mask = 6917529027641081856;
        long_castle_check_mask = 2017612633061982208;
        short_castle_check_mask = 8070450532247928832;
    }
    else {
        long_castle = state.w_long_castle;
        short_castle = state.w_short_castle;
        long_castle_occupation_mask = 14;
        short_castle_occupation_mask = 96;
        long_castle_check_mask = 28;
        short_castle_check_mask = 112;
    }

    // long castle
    if (long_castle) {
        if (!(occupancy_bitboard & long_castle_occupation_mask)) {
            U64 attacked_bitboard = attacked(state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count, bishop_attack_lookup_table , rook_magics, rook_mask_lookup_table, rook_mask_bit_count, rook_attack_lookup_table, king_lookup_table, occupancy_bitboard);
            if (!(attacked_bitboard & long_castle_check_mask)) {
                moves[move_index] = move(5 + 6*color, 4 + 56*color, 2 + 56*color, 5 + 6*color, false, true);
                move_index++;
            }
        }
    }

    // short castle
    if (short_castle) {
        if (!(occupancy_bitboard & short_castle_occupation_mask)) {
            U64 attacked_bitboard = attacked(state, !color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count, rook_attack_lookup_table, king_lookup_table, occupancy_bitboard);
            if (!(attacked_bitboard & short_castle_check_mask)) {
                moves[move_index] = move(5 + 6*color, 4 + 56*color, 6 + 56*color, 5 + 6*color, false, true);
                move_index++;
            }
        }
    }

    return move_index;
}

U64 init_zobrist_hashing_mailbox(game_state &state, zobrist_randoms &zobrist, bool color, std::array<int, 64>& piece_on_square) {
    // create random bitstrings for each game element and hash the first position
    // populate the mailbox representation with the piece index

    // generate a seed
    std::random_device rd;
    std::mt19937_64 rng(rd());

    // initialize the pseudo-random number generator
    static std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    // fill zobrist randoms piece table
    // position*NUM_PIECES + piece_index
    for (int i = 0; i < NUM_SQUARES; ++i) {
        for (int j = 0; j < NUM_PIECES; ++j) {
            zobrist.zobrist_piece_table[i*NUM_PIECES + j] = dist(rng);
        }
    }

    zobrist.zobrist_black_to_move = dist(rng);
    zobrist.zobrist_w_long_castle = dist(rng);
    zobrist.zobrist_w_short_castle = dist(rng);
    zobrist.zobrist_b_long_castle = dist(rng);
    zobrist.zobrist_b_short_castle = dist(rng);

    for (int i = 0; i < 8; ++i) {
        zobrist.zobrist_en_passant[i] = dist(rng);
    }

    // initialize the hash
    U64 hash = 0;
    // hash the pieces
    for (int i = 0; i < NUM_PIECES; ++i) {
        U64 bb = state.piece_bitboards[i];
        while (bb) {
            int position = __builtin_ctzll(bb);
            hash ^= zobrist.zobrist_piece_table[position*NUM_PIECES + i];
            bb &= bb - 1;
        }
    }
    // hash the en passant squares
    for (int i = 0; i < 2; ++i) {
        U64 bb = state.en_passant_bitboards[i];
        while (bb) {
            int file = __builtin_ctzll(bb) % 8;
            hash ^= zobrist.zobrist_en_passant[file];
            bb &= bb - 1;
        }
    }
    // hash the castling rights
    if (state.w_long_castle) {
        hash ^= zobrist.zobrist_w_long_castle;
    }
    if (state.w_short_castle) {
        hash ^= zobrist.zobrist_w_short_castle;
    }
    if (state.b_long_castle) {
        hash ^= zobrist.zobrist_b_long_castle;
    }
    if (state.b_short_castle) {
        hash ^= zobrist.zobrist_b_short_castle;
    }
    // hash the black to move
    if (color == 1) {
        hash ^= zobrist.zobrist_black_to_move;
    }

    // populate the mailbox representation
    for (int i = 0; i < 6; i++) {

        // get the correct piece bitboard
        U64 piece_bitboard = state.piece_bitboards[i + 6*color];
        while (piece_bitboard) {

            // get the position of the least significant set bit and remove it from the bitboard
            int position = pop_lsb(piece_bitboard);
            piece_on_square[position] = i + 6*color;
        }
    }
        

    return hash;
}

void apply_move(game_state& state, move& move_to_apply, U64& zobrist_hash, zobrist_randoms &zobrist, move_undo& undo, std::array<int, 64>& piece_on_square) {
    // apply a move object to a gamestate bitboard

    // save undo information
    undo.zobrist_hash = zobrist_hash;
    undo.w_long_castle = state.w_long_castle;
    undo.w_short_castle = state.w_short_castle;
    undo.b_long_castle = state.b_long_castle;
    undo.b_short_castle = state.b_short_castle;
    undo.en_passant = false;
    undo.en_passant_bitboards[0] = state.en_passant_bitboards[0];
    undo.en_passant_bitboards[1] = state.en_passant_bitboards[1];
    undo.captured_piece_index = -1;

    // remove the piece from the from position
    state.piece_bitboards[move_to_apply.piece_index] &= ~(1ULL << move_to_apply.from_position);
    zobrist_hash ^= zobrist.zobrist_piece_table[move_to_apply.from_position*NUM_PIECES + move_to_apply.piece_index];
    piece_on_square[move_to_apply.from_position] = 0;

    // add the piece to the to position
    state.piece_bitboards[move_to_apply.promotion_piece_index] |= 1ULL << move_to_apply.to_position;
    zobrist_hash ^= zobrist.zobrist_piece_table[move_to_apply.to_position*NUM_PIECES + move_to_apply.piece_index];
    piece_on_square[move_to_apply.to_position] = move_to_apply.promotion_piece_index;

    // remove potential captured piece
    // get opponent color
    bool opponent_color = move_to_apply.piece_index < 6;
    zobrist_hash ^= zobrist.zobrist_black_to_move;

    // remove captured opponent piece
    for (int i = 0; i < 6; i++) {
        // check if a capture happens
        if (state.piece_bitboards[i + 6*opponent_color] & (1ULL << move_to_apply.to_position)) {
            // remove captured piece
            state.piece_bitboards[i + 6*opponent_color] &= ~(1ULL << move_to_apply.to_position);
            zobrist_hash ^= zobrist.zobrist_piece_table[move_to_apply.to_position*NUM_PIECES + (i + 6*opponent_color)];
            undo.captured_piece_index = i + 6*opponent_color;
        }
    }

    // remove captured en passant piece
    if (move_to_apply.piece_index == 0) {
        if (state.en_passant_bitboards[1] & (1ULL << move_to_apply.to_position)) {
            state.piece_bitboards[6] &= ~(1ULL << (move_to_apply.to_position - 8));
            zobrist_hash ^= zobrist.zobrist_piece_table[(move_to_apply.to_position - 8)*NUM_PIECES + 6];
            undo.captured_piece_index = 6;
            undo.en_passant = true;
            piece_on_square[move_to_apply.to_position - 8] = 0;
        }
    }
    else if (move_to_apply.piece_index == 6) {
        if (state.en_passant_bitboards[0] & (1ULL << move_to_apply.to_position)) {
            state.piece_bitboards[0] &= ~(1ULL << (move_to_apply.to_position + 8));
            zobrist_hash ^= zobrist.zobrist_piece_table[(move_to_apply.to_position + 8)*NUM_PIECES + 0];
            undo.captured_piece_index = 0;
            undo.en_passant = true;
            piece_on_square[move_to_apply.to_position + 8] = 0;
        }
    }

    // clear en passant bitboards
    state.en_passant_bitboards[0] = 0;
    state.en_passant_bitboards[1] = 0;

    // update en passant bitboards
    if (move_to_apply.en_passantable) {
        if (move_to_apply.piece_index == 0) {
            state.en_passant_bitboards[0] = 1ULL << (move_to_apply.to_position - 8);
        }
        else if (move_to_apply.piece_index == 6) {
            state.en_passant_bitboards[1] = 1ULL << (move_to_apply.to_position + 8);
        }
    }

    // castling rights
    if ((state.w_long_castle) && (move_to_apply.to_position == 0)) {
        zobrist_hash ^= zobrist.zobrist_w_long_castle;
        state.w_long_castle = false;
    }
    else if ((state.w_short_castle) && (move_to_apply.to_position == 7)) {
        zobrist_hash ^= zobrist.zobrist_w_short_castle;
        state.w_short_castle = false;
    }
    else if ((state.b_long_castle) && (move_to_apply.to_position == 56)) {
        zobrist_hash ^= zobrist.zobrist_b_long_castle;
        state.b_long_castle = false;
    }
    else if ((state.b_short_castle) && (move_to_apply.to_position == 63)) {
        zobrist_hash ^= zobrist.zobrist_b_short_castle;
        state.b_short_castle = false;
    }
    // white king
    if (move_to_apply.piece_index == 5) {
        if (state.w_long_castle) {
            zobrist_hash ^= zobrist.zobrist_w_long_castle;
            state.w_long_castle = false;
        }
        if (state.w_short_castle) {
            zobrist_hash ^= zobrist.zobrist_w_short_castle;
            state.w_short_castle = false;
        }        
    }
    // black king
    else if (move_to_apply.piece_index == 11) {
        if (state.b_long_castle) {
            zobrist_hash ^= zobrist.zobrist_b_long_castle;
            state.b_long_castle = false;
        }
        if (state.b_short_castle) {
            zobrist_hash ^= zobrist.zobrist_b_short_castle;
            state.b_short_castle = false;
        }
    }
    // white rook
    else if ((state.w_long_castle) && (move_to_apply.piece_index == 3) && (move_to_apply.from_position == 0)) {
        zobrist_hash ^= zobrist.zobrist_w_long_castle;
        state.w_long_castle = false;
    }
    else if ((state.w_short_castle) && (move_to_apply.piece_index == 3) && (move_to_apply.from_position == 7)) {
        zobrist_hash ^= zobrist.zobrist_w_short_castle;
        state.w_short_castle = false;
    }
    // black rook
    else if ((state.b_long_castle) && (move_to_apply.piece_index == 9) && (move_to_apply.from_position == 56)) {
        zobrist_hash ^= zobrist.zobrist_b_long_castle;
        state.b_long_castle = false;
    }
    else if ((state.b_short_castle) && (move_to_apply.piece_index == 9) && (move_to_apply.from_position == 63)) {
        zobrist_hash ^= zobrist.zobrist_b_short_castle;
        state.b_short_castle = false;
    }

    // castling
    if (move_to_apply.castling) {
        // white long castling
        if (move_to_apply.to_position == 2) {
            state.piece_bitboards[3] &= ~(1ULL << 0);
            state.piece_bitboards[3] |= 1ULL << 3;
            // update rook part of the hash
            zobrist_hash ^= zobrist.zobrist_piece_table[0*NUM_PIECES + 3];
            zobrist_hash ^= zobrist.zobrist_piece_table[3*NUM_PIECES + 3];
            piece_on_square[3] = 3;
            piece_on_square[0] = 0;
        }
        // black long castling
        else if (move_to_apply.to_position == 58) {
            state.piece_bitboards[9] &= ~(1ULL << 56);
            state.piece_bitboards[9] |= 1ULL << 59;
            // update rook part of the hash
            zobrist_hash ^= zobrist.zobrist_piece_table[56*NUM_PIECES + 9];
            zobrist_hash ^= zobrist.zobrist_piece_table[59*NUM_PIECES + 9];
            piece_on_square[59] = 9;
            piece_on_square[56] = 0;
        }
        // white short castling
        else if (move_to_apply.to_position == 6) {
            state.piece_bitboards[3] &= ~(1ULL << 7);
            state.piece_bitboards[3] |= 1ULL << 5;
            // update rook part of the hash
            zobrist_hash ^= zobrist.zobrist_piece_table[7*NUM_PIECES + 3];
            zobrist_hash ^= zobrist.zobrist_piece_table[5*NUM_PIECES + 3];
            piece_on_square[5] = 3;
            piece_on_square[7] = 0;
        }
        // black short castling
        else if (move_to_apply.to_position == 62) {
            state.piece_bitboards[9] &= ~(1ULL << 63);
            state.piece_bitboards[9] |= 1ULL << 61;
            // update rook part of the hash
            zobrist_hash ^= zobrist.zobrist_piece_table[63*NUM_PIECES + 9];
            zobrist_hash ^= zobrist.zobrist_piece_table[61*NUM_PIECES + 9];
            piece_on_square[61] = 9;
            piece_on_square[63] = 0;
        }
    }
}

void undo_move(game_state& state, move& move_to_undo, U64& zobrist_hash, zobrist_randoms &zobrist, move_undo& undo, std::array<int, 64>& piece_on_square) {
    // undo a move object to a gamestate bitboard

    zobrist_hash = undo.zobrist_hash;
    state.w_long_castle = undo.w_long_castle;
    state.w_short_castle = undo.w_short_castle;
    state.b_long_castle = undo.b_long_castle;
    state.b_short_castle = undo.b_short_castle;
    state.en_passant_bitboards[0] = undo.en_passant_bitboards[0];
    state.en_passant_bitboards[1] = undo.en_passant_bitboards[1];

    // remove the piece from the to position
    state.piece_bitboards[move_to_undo.promotion_piece_index] &= ~(1ULL << move_to_undo.to_position);
    piece_on_square[move_to_undo.to_position] = 0;

    // add the piece to the from position
    state.piece_bitboards[move_to_undo.piece_index] |= 1ULL << move_to_undo.from_position;
    piece_on_square[move_to_undo.from_position] = move_to_undo.piece_index;

    // captured pieces
    if (undo.captured_piece_index != -1) {
        // check for en passant
        if (undo.en_passant) {
            if (undo.captured_piece_index == 0) {
                state.piece_bitboards[undo.captured_piece_index] |= 1ULL << (move_to_undo.to_position + 8);
                piece_on_square[move_to_undo.to_position + 8] = undo.captured_piece_index;
            }
            else if (undo.captured_piece_index == 6) {
                state.piece_bitboards[undo.captured_piece_index] |= 1ULL << (move_to_undo.to_position - 8);
                piece_on_square[move_to_undo.to_position - 8] = undo.captured_piece_index;
            }
        }
        else {
            state.piece_bitboards[undo.captured_piece_index] |= 1ULL << move_to_undo.to_position;
            piece_on_square[move_to_undo.to_position] = undo.captured_piece_index;
        }
    }

    // undo rook moves in castling
    if (move_to_undo.castling) {
        // white long castling
        if (move_to_undo.to_position == 2) {
            state.piece_bitboards[3] &= ~(1ULL << 3);
            state.piece_bitboards[3] |= 1ULL << 0;
            piece_on_square[0] = 3;
            piece_on_square[3] = 0;
        }
        // black long castling
        else if (move_to_undo.to_position == 58) {
            state.piece_bitboards[9] &= ~(1ULL << 59);
            state.piece_bitboards[9] |= 1ULL << 56;
            piece_on_square[56] = 9;
            piece_on_square[59] = 0;
        }
        // white short castling
        else if (move_to_undo.to_position == 6) {
            state.piece_bitboards[3] &= ~(1ULL << 5);
            state.piece_bitboards[3] |= 1ULL << 7;
            piece_on_square[7] = 3;
            piece_on_square[5] = 0;
        }
        // black short castling
        else if (move_to_undo.to_position == 62) {
            state.piece_bitboards[9] &= ~(1ULL << 61);
            state.piece_bitboards[9] |= 1ULL << 63;
            piece_on_square[63] = 9;
            piece_on_square[61] = 0;
        }
    }
}

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
    const U64& occupancy_bitboard) {
    // check if a given game state is legal for the given color

    // get attacked bitboard
    U64 attacked_bitboard = attacked(state, color, pawn_move_lookup_table, pawn_attack_lookup_table, knight_lookup_table, bishop_magics, bishop_mask_lookup_table, bishop_mask_bit_count, bishop_attack_lookup_table, rook_magics, rook_mask_lookup_table, rook_mask_bit_count, rook_attack_lookup_table, king_lookup_table, get_occupancy(state.piece_bitboards));

    // get the king position
    int king_position;
    U64 king_bitboard = state.piece_bitboards[11 - 6*color];

    // check if the king is attacked
    return !(attacked_bitboard & king_bitboard);
}

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
    std::array<U64, 64>& king_lookup_table) {

    // create pawn move bitboards lookup table
    // position + NUM_SQUARES*color as index
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 2; j++) {
            pawn_move_lookup_table[i + 64*j] = get_pawn_move(i, j);
        }
    }

    // create pawn attack bitboards lookup table
    // position + NUM_SQUARES*color as index
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 2; j++) {
            pawn_attack_lookup_table[i + 64*j] = get_pawn_attack(i, j);
        }
    }

    // create knight attack bitboards lookup table
    for (int i = 0; i < 64; i++) {
        knight_lookup_table[i] = get_knight_attack(i);
    }

    // create bishop attack bitboards lookup tables
    // generate magics and lookup tables for each position
    // position*MAGIC_TABLE_SIZE + index as index
    for (int i = 0; i < 64; i++) {
        U64 mask_bitboard = get_bishop_mask(i);
        bishop_mask_lookup_table[i] = mask_bitboard;
        bishop_mask_bit_count[i] = count_set_bits(mask_bitboard);
        std::vector<U64> blocker_boards = get_blocker_boards(i, mask_bitboard);
        std::vector<U64> attack_bitboards;
        for (U64 blocker_board : blocker_boards) {
            attack_bitboards.push_back(get_bishop_attack(i, blocker_board));
        }
        generate_magics(i, mask_bitboard, blocker_boards, attack_bitboards, bishop_magics[i], bishop_attack_lookup_table);
    }

    // create rook attack bitboards lookup tables
    // generate magics and lookup tables for each position
    // position*MAGIC_TABLE_SIZE + index as index
    for (int i = 0; i < 64; i++) {
        U64 mask_bitboard = get_rook_mask(i);
        rook_mask_lookup_table[i] = mask_bitboard;
        rook_mask_bit_count[i] = count_set_bits(mask_bitboard);
        std::vector<U64> blocker_boards = get_blocker_boards(i, mask_bitboard);
        std::vector<U64> attack_bitboards;
        for (U64 blocker_board : blocker_boards) {
            attack_bitboards.push_back(get_rook_attack(i, blocker_board));
        }
        generate_magics(i, mask_bitboard, blocker_boards, attack_bitboards, rook_magics[i], rook_attack_lookup_table);
    }

    // create king attack bitboards lookup table
    for (int i = 0; i < 64; i++) {
        king_lookup_table[i] = get_king_attack(i);
    }
}