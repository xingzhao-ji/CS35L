import { useState } from 'react'; 
// useState is a React Hook that lets you add React state to function components

// adjacency for a 3×3 grid, including diagonals
const ADJ = {
  0: [1, 3, 4],     // square 0 touches squares 1, 3, and diagonal 4
  1: [0, 2, 3, 4, 5],
  2: [1, 4, 5],
  3: [0, 1, 4, 6, 7],
  4: [0, 1, 2, 3, 5, 6, 7, 8],
  5: [1, 2, 4, 7, 8],
  6: [3, 4, 7],
  7: [3, 4, 5, 6, 8],
  8: [4, 5, 7],
};

function Square({ value, onSquareClick }) {
  return (
    <button
      className="square"
      onClick={onSquareClick}
    >
      {value}
    </button>
  );
  // renders one cell of the board;
  // value is 'X', 'O', or null;
  // onSquareClick handles a click on this button
}

export default function Board() {
  // Board component manages game state and UI

  const [xIsNext, setXIsNext] = useState(true);
  // boolean: true when it's X’s turn, false for O

  const [squares, setSquares] = useState(Array(9).fill(null));
  // 9-element array holding 'X', 'O', or null for each cell

  const [xQueue, setXQ] = useState([]);
  // array of indices where X has placed its pieces (max 3)

  const [oQueue, setOQ] = useState([]);
  // array of indices where O has placed its pieces (max 3)

  const [phase2, setPhase2] = useState(false);
  // flag: false during initial placement phase, true during movement phase

  const [selectedPiece, setSelected] = useState(null);
  // holds the index of the piece the current player has clicked to move

  const [moveQueue, setMoveQueue] = useState([]);
  // generic FIFO queue of moves (not used by default UI)

  function enqueue(move) {
    setMoveQueue(prev => [...prev, move]);
  }
  // adds a move to the end of moveQueue

  function dequeue() {
    setMoveQueue(prev => prev.slice(1));
  }
  // removes the first move from moveQueue

  function handleClick(i) {
    if (calculateWinner(squares)) {
      return;
    }
    // if there is already a winner, ignore further clicks

    if (!phase2) { // PHASE 1: piece placement
      if (squares[i]) return;
      // cannot place on an occupied square

      const nextSquares = squares.slice();
      // copy array for immutability

      if (xIsNext) {
        if (xQueue.length >= 3) return;
        // X can place at most 3 pieces
        nextSquares[i] = 'X';
        setXQ(prev => [...prev, i]);
        // record X’s placement index
      } else {
        if (oQueue.length >= 3) return;
        nextSquares[i] = 'O';
        setOQ(prev => [...prev, i]);
      }

      setSquares(nextSquares);
      // update board
      setXIsNext(!xIsNext);
      // switch turn

      if (
        (xQueue.length + 1 === 3 && oQueue.length === 3) ||
        (oQueue.length + 1 === 3 && xQueue.length === 3)
      ) {
        setPhase2(true);
      }
      // once both have placed 3 pieces, move to phase 2
      return;
    }

    // PHASE 2: piece movement
    if (phase2) {
      if (selectedPiece === null) {
        // first click: select one of your pieces
        if (
          (xIsNext && squares[i] === 'X') ||
          (!xIsNext && squares[i] === 'O')
        ) {
          setSelected(i);
        }
        return;
      }

      // CENTER rule enforcement:
      const CENTER = 4;
      const centerOwner = squares[CENTER];
      let enforceCenterRule = false;
      if (centerOwner === 'X' && xIsNext) enforceCenterRule = true;
      if (centerOwner === 'O' && !xIsNext) enforceCenterRule = true;

      if (centerOwner && enforceCenterRule && selectedPiece !== CENTER) {
        // if center occupied by current player, they must move out of center
        if (
          ADJ[selectedPiece].indexOf(i) === -1 ||
          squares[i] !== null
        ) {
          setSelected(null);
          return;
        }
        const temp = squares.slice();
        temp[selectedPiece] = null;
        temp[i] = xIsNext ? 'X' : 'O';
        if (!calculateWinner(temp)) {
          // if move doesn’t create a win, cancel selection
          setSelected(null);
          return;
        }
      }

      if (!ADJ[selectedPiece].includes(i) || squares[i] !== null) {
        // move is legal only to an adjacent empty square
        setSelected(null);
        return;
      }

      // perform the move
      const next = squares.slice();
      next[selectedPiece] = null;
      next[i] = xIsNext ? 'X' : 'O';
      setSquares(next);

      // update the appropriate queue to reflect new position
      if (xIsNext) {
        setXQ(queue =>
          queue.map(p => (p === selectedPiece ? i : p))
        );
      } else {
        setOQ(queue =>
          queue.map(p => (p === selectedPiece ? i : p))
        );
      }

      setSelected(null);
      setXIsNext(!xIsNext);
    }
  }

  const winner = calculateWinner(squares);
  let status;
  if (winner) {
    status = 'Winner: ' + winner;
  } else if (phase2 && selectedPiece !== null) {
    status = `Move ${xIsNext ? 'X' : 'O'} from ${selectedPiece}.`;
  } else {
    status = 'Next player: ' + (xIsNext ? 'X' : 'O');
  }
  // compute status message based on game state

  return (
    <>
      <div className="status">{status}</div>
      <div className="board-row">
        {[0, 1, 2].map(i => (
          <Square
            key={i}
            value={squares[i]}
            isSelected={i === selectedPiece}
            onSquareClick={() => handleClick(i)}
          />
        ))}
      </div>
      <div className="board-row">
        {[3, 4, 5].map(i => (
          <Square
            key={i}
            value={squares[i]}
            isSelected={i === selectedPiece}
            onSquareClick={() => handleClick(i)}
          />
        ))}
      </div>
      <div className="board-row">
        {[6, 7, 8].map(i => (
          <Square
            key={i}
            value={squares[i]}
            isSelected={i === selectedPiece}
            onSquareClick={() => handleClick(i)}
          />
        ))}
      </div>
    </>
  );
}

function calculateWinner(squares) {
  const lines = [
    [0, 1, 2], // rows
    [3, 4, 5],
    [6, 7, 8],
    [0, 3, 6], // columns
    [1, 4, 7],
    [2, 5, 8],
    [0, 4, 8], // diagonals
    [2, 4, 6],
  ];
  for (let i = 0; i < lines.length; i++) {
    const [a, b, c] = lines[i];
    if (
      squares[a] &&
      squares[a] === squares[b] &&
      squares[a] === squares[c]
    ) {
      return squares[a];
    }
  }
  return null;
  // returns 'X' or 'O' if three in a row, otherwise null
}

#!/bin/bash
tr -cs "\-A-Za-z0-9'./&" $'\n' |  # break input into words
sort -u | \                         # sort uniquely
comm -23 - sorted.words            # remove common words with sorted.words file
