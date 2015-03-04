
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <stack>
#include <bitset>
#include <tuple>

using namespace std;

namespace Sudoku {
  // A point in a sudoku grid
  struct Point {
    Point() : row(0), col(0) {}
    Point(uint8_t _row, uint8_t _col) : row(_row), col(_col) {}
    uint8_t row;
    uint8_t col;
    bool operator<(const Point &rhs) const { return make_pair(row, col) < make_pair(rhs.row, rhs.col); }
    bool operator==(const Point &rhs) const { return make_pair(row, col) == make_pair(rhs.row, rhs.col); }
  };
}

namespace std {
  template <> struct hash<Sudoku::Point> {
    size_t operator()(const Sudoku::Point &p) const {
      return 9*p.row + p.col;
    }
  };
}


namespace Sudoku {

  // State tracking which and how many values that may be assigned to a sudoku cell
  struct CellState {
  private:
    bitset<9> option_state;
    uint8_t option_count;
    uint8_t value;

  public:
    CellState() : option_state(0xFFFF), option_count(9), value(0) { }

    bool remove_option( uint8_t value ) {
      uint8_t ix = value-1;
      if ( option_state[value-1] ) option_count--;
      option_state[value-1] = false;
      return option_state.none();
    }

    void add_option( uint8_t value ) {
      uint8_t ix = value-1;
      if ( !option_state[value-1] ) option_count++;
      option_state[value-1] = true;
    }

    bool has_option( uint8_t value ) const {
      return option_state[value-1];
    }

    uint8_t get_value() const { return value; }
    void set_value(uint8_t _value) { value = _value; }
    uint8_t get_option_count() const { return option_count; }
  };

  struct SudokuState {
    
    // Total number of unoccupied cells
    uint8_t free_cells;

    // Total number of guesses made
    uint32_t iterations;

    // Maintain a stack of "assign value" operations and their effects for backtracking search
    struct SetOperation {
      SetOperation(const Point &p, uint8_t val) : set(p), value(val) {}
      vector<Point> forbids;
      Point set;
      uint8_t value;
      void push(const Point &p) {
        forbids.push_back(p);
      }
      void push(const uint8_t row, const uint8_t col) {
        forbids.emplace_back(row, col);
      }
    };
    stack<SetOperation> undo_stack;

    // For each point, track sudoku cell states ...
    unordered_map<Point,CellState> cell_states;
    
    // ... and the other the 20 other points with which it shares constraints
    unordered_map<Point,vector<Point>> constraint_map;

    // Generate the constraint mapping for a single point
    void generate_constraint_map( const Point &p ) {
      // Don't generate any constraints if already generated.
      if ( constraint_map.count(p) > 0 ) return;

      // Else, generate constraints for this point
      vector<Point> ps;
      ps.resize(20);
      int ix = 0;

      const uint8_t row = p.row;
      const uint8_t col = p.col;
      for ( uint8_t i = 0 ; i < 9 ; ++i ) {
        if ( col != i ) {
          ps[ix++] = Point(row, i);
        }
        if ( row != i ) {
          ps[ix++] = Point(i, col);
        }
      }

      // Block
      uint8_t startRow = row - row%3;
      uint8_t startCol = col - col%3;
      for ( int i = startRow ; i < startRow+3 ; ++i ) {
        for ( int j = startCol ; j < startCol+3 ; ++j ) {
          if ( i != row && j != col ) {
            ps[ix++] = Point(i, j);
          }
        }
      }
      constraint_map.emplace(p, ps);
    }

    SudokuState() : free_cells(9*9), iterations(0) {
      for ( int i = 0 ; i < 9 ; ++i ) {
        for ( int j = 0 ; j < 9 ; ++j ) {
          Point p(i,j);
          cell_states.emplace( p, CellState() );
          generate_constraint_map(p);
        }
      }
    }
    
    bool is_solved() const {
      return free_cells == 0;
    }

    // Force a cell value with no option to undo
    bool force( Point p, uint8_t value ) { return set( p, value, true ); }

    // Sets a value. If force is set, does not allow undo.
    bool set( Point p, uint8_t value, bool force = false ) {
      CellState &state = cell_states[p];

      // Cannot set if already set
      if ( state.get_value() > 0 ) return false;

      // Cannot set if value is not a valid option
      if ( !state.has_option(value) ) return false;
      
      state.set_value(value);
      const vector<Point> &cs = constraint_map[p];

      if ( force ) {
        // If forcing, just remove the option from constrained points
        for_each( begin(cs), end(cs), [&](Point other) {
          cell_states[other].remove_option(value);
        });
      }
      else {
        // Else remove option and add changes to undo stack
        undo_stack.emplace(p, value);
        SetOperation &undo = undo_stack.top();
        for_each( begin(cs), end(cs), [&](Point other) {
          CellState &other_state = cell_states[other];
          if ( other_state.has_option(value) ) {
            undo.push(other);
            other_state.remove_option(value);
          }
        });
      }
      free_cells--;
      return true;
    }

    // Read the last set operation from the undo stack and undo all its changes to the state
    void unset() {
      const SetOperation &op = undo_stack.top();
      const uint8_t value = op.value;
      for ( auto it = begin(op.forbids) ; it != end(op.forbids) ; ++it ) {
        cell_states[*it].add_option(value);
      }
      cell_states[op.set].set_value(0);
      free_cells++;
      undo_stack.pop();
    }

    void print(ostream &out) const {
      // for each row
      for ( int i = 0 ; i < 9 ; ++i ) {
        // print divider every 3:d row
        if ( i%3 == 0 ) out << "+---+---+---+" << endl;

        // print row with divider every 3 columns.
        for ( int j = 0 ; j < 9 ; ++j ) {
          if ( j%3 == 0 ) out << "|";

          // This is a bit terrible. Const correctness! :argh:
          CellState &state = const_cast<unordered_map<Point,CellState>&>(cell_states)[Point(i,j)];
          uint8_t value = state.get_value();
          if ( value > 0 ) out << int(value);
          else out << ".";
        }
        out << "|" << endl;

      }
      cout << "+---+---+---+" << endl << endl;
    }

    bool solve() {
      // If the sudoku is solved then we're good
      if ( is_solved() ) return true;
      iterations++;

      // Not solved, find the most constrained valid cell
      pair<Point,CellState> most_constrained_cell;
      uint8_t most_constraints = 10;
      for ( auto it = begin(cell_states) ; it != end(cell_states) ; ++it ) {
        const Point     &p     = it->first;
        const CellState &state = it->second;

        // For any empty point
        if ( !state.get_value() ) {
          // If it is overconstrained, this solution is invalid
          uint8_t option_count = state.get_option_count();
          if ( !option_count ) return false;

          // Else, see if this is a better option than our other best
          if ( option_count > 0 && option_count < most_constraints ) {
            most_constraints = option_count;
            most_constrained_cell = *it;
            if ( most_constraints == 1 ) break;
          }
        }
      }

      // If all cells are fully constrained then this solution is not valid
      if ( most_constraints > 9 ) return false;

      // Try all valid values for the most constrained cell
      for ( uint8_t value = 1 ; value < 10 ; ++value ) {
        // If it is allowed then try solving from that guess
        if ( most_constrained_cell.second.has_option(value) ) {
          set(most_constrained_cell.first, value);
          if ( solve() ) return true;
          unset();
        }
      }
      return false;
    }
  };

  SudokuState read( istream &in ) {
    SudokuState state;

    // This reader is quite permissive as to formatting. Anything not in skip is treated as a
    // cell data point, anything not a digit is interpreted as an empty cell.
    string skip(" -|+/\\\n");
    
    // Keep consuming input until eof
    for ( int row = 0; !in.eof() ; ) {
      bool hasData = false;

      string line;
      getline(in, line);

      // Check each character of the line we read
      for ( const char *str = line.c_str(), int col = 0 ; *str ; ) {
        char c = *(str++);

        // If it's in the skiplist, ignore it
        if ( skip.find_first_of(c) != string::npos ) continue;

        // Else, if it's a digit update the sudoku state
        if ( '1' <= c && c <= '9' ) state.force( Point(row,col), c-'0');
        
        hasData = true;
        ++col;
        if ( col > 9 ) break;
      }

      // A line in the input is considered specifying a full row in the sudoku if it has at least
      // one point of data.
      if ( hasData ) ++row;
      if ( row > 9 ) break;
    }
    return state;
  }

}


int main(int argc, char *argv[]) 
{
  Sudoku::SudokuState state = Sudoku::read(cin);
  cout << "Read sudoku " << endl;
  state.print(cout);

  if ( state.solve() ) {
    cout << "Sudoku solved in " << state.iterations << " iterations" << endl << endl;
    state.print(cout);
  }
  else cout << "Sudoku has no valid solution" << endl;

  system("PAUSE");
}
