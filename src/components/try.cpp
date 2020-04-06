/* Klotski solver. */
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <vector>

using std::array;
using std::cout;
using std::endl;
using std::map;
using std::ostream;
using std::queue;
using std::set;
using std::vector;

// Set of block labels.  Every block is the set of cells with the same label.
using Labels = set<char>;

// State of the board, that is, position of blocks, encoded as the label
// corresponding to each cell, with ' ' denoting empty cell.
using State = array<char,20>;

// Undirected graph with states as vertices and valid moves as edges.
// Every vertex is mapped to its set of neighbors.
using Graph = map<State, set<State>>;

// Solution for any reachable state: each non-solved state is mapped to one of
// its neighbors that is one of the optimal moves (optimal as in least number of
// steps).  Solves states map to themselves.  All steps of an optimal solution
// starting from any reachable state can be found by iterated application of the
// map.
using Solution = map<State, State>;

enum Direction {
  LEFT,
  RIGHT,
  TOP,
  BOTTOM
};

// Relabel blocks in |state| such that they are encountered in increasing order.
// This is permissible because congruent blocks in the same orientation are
// indistinguishable.  This normalization significantly cuts down on
// computational cost and memory usage.
void Normalize(State* state, const Labels& labels) {
  set<char>::const_iterator l = labels.begin();
  for (size_t i = 0; i < state->size(); ++i) {
    // At this point, the first i-1 cells of |state| have labels that appear in
    // increasing order.  |*l| is the smallest label that has not been seen yet.
    char c = (*state)[i];
    if (c == ' ') {
      continue;
    }
    if (c < *l) {
      continue;
    }
    if (c == *l) {
      ++l;
      if (l == labels.end()) {
        break;
      }
      continue;
    }
    // The label |c| is encountered, but it is larger than |*l|.  Swap |c| and
    // |*l| labels.  None of them can appear among the first |i-1| cells.
    for (size_t j = i; j < state->size(); ++j) {
      if ((*state)[j] == c) {
        (*state)[j] = *l;
        continue;
      }
      if ((*state)[j] == *l) {
        (*state)[j] = c;
      }
    }
    ++l;
    if (l == labels.end()) {
      break;
    }
  }
}

Labels GatherLabels(State initial) {
  Labels labels;
  for (char c : initial) {
    // ' ' is a special label denoting empty space.
    if (c != ' ') {
      labels.insert(c);
    }
  }
  return labels;
}

// Is the cell corresponding to |index| on the |direction| edge of the board,
// that is, one of the |direction|-most cells.
bool IsEdge(Direction direction, size_t index) {
  switch (direction) {
    case Direction::LEFT:
      return index%4 == 0;
    case Direction::RIGHT:
      return index%4 == 3;
    case Direction::TOP:
      return index < 4;
    case Direction::BOTTOM:
      return index > 15;
  }
  return true;
}

// Return the index corresponding to the cell that is adjacent to |index| and is
// to |direction| from it.
size_t Move(Direction direction, size_t index) {
  switch (direction) {
    case Direction::LEFT:
      return index - 1;
    case Direction::RIGHT:
      return index + 1;
    case Direction::TOP:
      return index - 4;
    case Direction::BOTTOM:
      return index + 4;
  }
  return 0;
}

// Pop an element of |states_to_explore| and add it together with its edges
// to |graph|, and add all its not yet explored neighbors to
// |states_to_explore|.
void AddNeighbors(Graph* graph, set<State>* states_to_explore, const Labels& labels) {
  set<State>::iterator explore_iterator = states_to_explore->begin();
  State current = *explore_iterator;
  states_to_explore->erase(explore_iterator);
  map<State, set<State>>::iterator graph_iterator;
  bool success;
  std::tie(graph_iterator, success) =
      graph->insert(make_pair(current, set<State>({})));
  assert(success);
  // Cycle through blocks.
  for (char label : labels) {
    vector<size_t> indices;
    for (size_t i = 0; i < current.size(); ++i) {
      if (current[i] == label)
        indices.push_back(i);
    }
    // Cycle through possible directions.
    for (Direction direction : {LEFT, RIGHT, TOP, BOTTOM}) {
      bool canmove = true;
      for (size_t i : indices) {
        if (IsEdge(direction, i)) {
          canmove = false;
          break;
        }
        char neighbor = current[Move(direction, i)];
        if (neighbor != ' ' && neighbor != label) {
          canmove = false;
          break;
        }
      }
      if (!canmove)
        continue;
      State newstate(current);
      for (size_t i : indices)
        newstate[i] = ' ';
      for (size_t i : indices)
        newstate[Move(direction, i)] = label;
      Normalize(&newstate, labels);
      graph_iterator->second.insert(newstate);
      if (graph->find(newstate) != graph->end())
        continue;
      states_to_explore->insert(newstate);
    }
  }
}

Graph GenerateGraph(State initial) {
  cout << "Generating graph..." << endl;
  Labels labels = GatherLabels(initial);
  Normalize(&initial, labels);
  Graph graph;
  set<State> states_to_explore{initial};
  while (!states_to_explore.empty()) {
    AddNeighbors(&graph, &states_to_explore, labels);
  }
  cout << graph.size() << " vertices found." << endl;
  return graph;
}

bool IsSolved(const State& state) {
  return state[13] == state[14] && state[14] == state[17] &&
      state[17] == state[18] && state[18] != ' ';
}

Solution Solve(const Graph& graph) {
  cout << "Finding solutions..." << endl;
  // States are queued in non-decreasing order of shortest distance from a
  // solved state.
  queue<State> states_to_explore;
  Solution solution;
  for (const auto& vertex : graph) {
    if (!IsSolved(vertex.first)) {
      continue;
    }
    auto result = solution.insert(make_pair(vertex.first, vertex.first));
    assert(result.second);
    states_to_explore.push(vertex.first);
  }
  while (!states_to_explore.empty()) {
    const State& state = states_to_explore.front();
    Graph::const_iterator it = graph.find(state);
    for (const State& neighbor : it->second) {
      auto result = solution.insert(make_pair(neighbor, state));
      if (!result.second) {
        // |neighbor| already existed in |solution|,
        // insertion was a no-op.
        continue;
      }
      states_to_explore.push(neighbor);
    }
    states_to_explore.pop();
  }
  assert(solution.size() == graph.size());
  cout << "Done." << endl;
  return solution;
}

ostream& operator<<(ostream& stream, const State& state) {
  for (size_t i = 0; i < 20; i += 4) {
    cout << state[i] << state[i+1] << state[i+2] << state[i+3] << endl;
  }
  cout << endl;
  return stream;
}

void Print(const State& initial, const Solution& solution) {
  Solution::const_iterator it = solution.find(initial);
  cout << it->first;
  while (it->first != it->second) {
    cout << it->second;
    it = solution.find(it->second);
  }
}

int main() {
  /*
  // Set 1 Level 15
  State initial({
      '0', '1', '1', '2',
      '0', '1', '1', '3',
      '4', '5', '6', '7',
      '4', '8', '6', '7',
      ' ', ' ', '9', '9'});
  */
  /*
  // Set 1 Level 18
  State initial({
      '1', '2', '2', '3',
      '1', '2', '2', '4',
      '5', '6', '7', '8',
      '5', '6', '7', '9',
      ' ', 'a', 'a', ' '});
  */
  // Set 1 Level 19
  State initial({
      '1', '2', '2', '3',
      '1', '2', '2', '3',
      '4', '5', '6', '7',
      '8', '9', '9', 'a',
      '8', ' ', ' ', 'a'});
  Graph graph = GenerateGraph(initial);
  Solution solution = Solve(graph);
  Print(initial, solution);
  return 0;
}