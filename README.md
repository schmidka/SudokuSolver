This is a relatively silly Sudoku solver, based on a simple backtracking 
recursive search.

It reads sudokus from standard in. To solve, one would generally run
SudokuSolver.exe < sudoku_file.txt

Input sudokus should be on a format similar to

+---+---+---+
|.61|..7|..3|
|.92|..3|...|
|...|...|...|
+---+---+---+
|..8|53.|...|
|...|...|5.4|
|5..|..8|...|
+---+---+---+
|.4.|...|..1|
|...|16.|8..|
|6..|...|...|
+---+---+---+

or 

200080300
060070084
030500209
000105408
000000000
402706000
301007040
720040060
004010003

The parser is quite simple (read: stupid) and has the following requirements:
- Any whitespace character as well as +, /, \, - and | is ignored.
- Empty lines, or lines consisting only of the above, are ignored.
- Any non-empty line is seen as defining a single row of the sudoku.
- Characters not in the set [1..9] are treated as empty cells.

The solution will be printed to cout, if one exists, along with some
superfluous information.
