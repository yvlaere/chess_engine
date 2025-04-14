import chess
import chess.pgn
import chess.engine

# Define the FEN for "Position 3"
fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"  # Replace if needed

# Initialize board
board = chess.Board(fen)
moves = list(board.legal_moves)

# divide
print("Move 1")
board.push(moves[47])
print(moves[47])

moves = list(board.legal_moves)
board.push(moves[34])
print("Move 2")
print(moves[34])

moves = list(board.legal_moves)
for move in moves:
    print(move)
board.push(moves[40])
print("Move 3")
print(moves[40])


# Dictionary to count captures per piece type
capture_counts = {
    "P": 0, "N": 0, "B": 0, "R": 0, "Q": 0, "K": 0,  # White pieces
    "p": 0, "n": 0, "b": 0, "r": 0, "q": 0, "k": 0,  # Black pieces
}

def perft(board, depth):
    if depth == 0:
        return 1

    nodes = 0
    for move in board.legal_moves:
        board.push(move)
        #print("move")
        #print(board)

        # Check if the move is a capture
        if (depth == 1):
            if board.is_capture(move):
                captured_piece = board.piece_at(move.to_square)
                if captured_piece:
                    capture_counts[captured_piece.symbol()] += 1
                    #print(captured_piece.symbol())
                    if captured_piece.symbol() == "p":
                        print('possible move')
                        print(move)

        nodes += perft(board, depth - 1)
        board.pop()

    return nodes

# Run Perft at Depth 4
total_nodes = perft(board, 1)

# Print Results
print("\nTotal Nodes Searched:", total_nodes)
print("Captures per Piece Type:")
for piece, count in capture_counts.items():
    print(f"{piece}: {count}")