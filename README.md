This is a relatively silly Sudoku solver, based on a simple backtracking 
recursive search.

It reads sudokus from cin, where sudokus are on the format

..53.....
8......2.
.7..1.5..
4....53..
.1..7...6
..32...8.
.6.5....9
..4....3.
.....97..

The specific character used for empty doesn't matter, the others do. No 
whitespace or dividing lines are allowed. A few valid sample sudokus are 
included in the samples/ folder.

To solve, one would generally run SudokuSolver.exe < sudoku_file.txt

The solution will be printed to cout, along with some superfluous information.