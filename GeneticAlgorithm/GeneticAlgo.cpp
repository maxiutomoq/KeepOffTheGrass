#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <random>
#include <ctime>
#include <numeric>
#include <cassert>
#include <cmath>

const double EPS = 1e-6;
const int INF = 1000000000;
static std::mt19937 rng(time(nullptr));

int randRange(int minVal, int maxVal) {
	if (maxVal < minVal) {
		throw std::invalid_argument("maxVal must be greater than or equal to minVal");
	}

	std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(rng);	
}

static constexpr int ME = 1;
static constexpr int OPP = 0;
static constexpr int NONE = -1;

struct Tile {
	int x, y;                   // tile coordinates
	int scrap_amount;           // 0 if grass
	int owner;                  // 1 = ME, 0 = OPP, -1 = NONE
	int myUnits;                // number of my robots
	int oppUnits;               // number of opp robots
	bool recycler;              // true if there's a recycler
	bool can_build;             // true if a recycler can be built
	bool can_spawn;             // true if units can be spawned
	bool in_range_of_recycler;  // will be recycled at the end of turn
};

bool operator==(const Tile &a, const Tile &b) {
	return a.x == b.x &&
		   a.y == b.y &&
		   a.scrap_amount == b.scrap_amount &&
		   a.owner == b.owner &&
		   a.myUnits == b.myUnits &&
		   a.oppUnits == b.oppUnits &&
		   a.recycler == b.recycler &&
		   a.can_build == b.can_build &&
		   a.can_spawn == b.can_spawn &&
		   a.in_range_of_recycler == b.in_range_of_recycler;
}

std::ostream& operator<<(std::ostream& os, const Tile &t) {
	os << "(" << t.x << "," << t.y << ")";
	return os;
}

// Movement delta
const int dx[] = {1, 0, -1, 0};
const int dy[] = {0, 1, 0, -1};

bool checkAdjacent(int x1, int y1, int x2, int y2) {
	return abs(x1 - x2) + abs(y1 - y2) == 1;
}

bool checkInBounds(int x, int y, int width, int height) {
	return (0 <= x && x < width && 0 <= y && y < height);
}

enum class ActionType { BUILD, SPAWN, MOVE };

struct Action {
	ActionType type;

	// For BUILD and SPAWN
	int x, y;

	// For SPWAN and MOVE
	int amount;

	// For MOVE
	int fromX, fromY;
	int toX, toY;
};

// ------------------------------------------------------
// GameState Class
// ------------------------------------------------------
class GameState {
public:
	GameState() {}
	GameState(const GameState &other) = default;
	GameState(int _myMatter, int _oppMatter, int _width, int _height, std::vector<Tile> _tiles)
		: myMatter(_myMatter), oppMatter(_oppMatter), width(_width), height(_height), tiles(_tiles) {}
	
	inline int tileIndex(int x, int y) const {
		return y * width + x;
	}

	void applyAction(const Action &action, bool isMyAction) {
		int &currentMatter = (isMyAction ? myMatter : oppMatter);

		switch (action.type) {
			case ActionType::BUILD: {
				if (currentMatter < 10) return;
				int idx = tileIndex(action.x, action.y);
				Tile &tile = tiles[idx];

				if (!tile.recycler && tile.scrap_amount > 1 &&
					tile.can_build && tile.owner == (isMyAction ? ME : OPP)) {
					tile.recycler = true;
					currentMatter -= 10;
				}
				
				break;
			}
			case ActionType::SPAWN: {
				if (currentMatter < 10) return;
				int idx = tileIndex(action.x, action.y);
				Tile &tile = tiles[idx];

				if (tile.can_spawn && tile.scrap_amount > 0 &&
					tile.owner == (isMyAction ? ME : OPP)) {
					if (isMyAction) {
						tile.myUnits += action.amount;
					}
					else {
						tile.oppUnits += action.amount;
					}
					currentMatter -= 10;
				}

				break;
			}
			case ActionType::MOVE: {
				int fromIdx = tileIndex(action.fromX, action.fromY);
				int toIdx = tileIndex(action.toX, action.toY);
				Tile &fromTile = tiles[fromIdx];
				Tile &toTile = tiles[toIdx];

				if (fromTile.owner != (isMyAction ? ME : OPP)) return;
				if (isMyAction && fromTile.myUnits < action.amount) return;
				if (!isMyAction && fromTile.oppUnits < action.amount) return;
				if (toTile.scrap_amount == 0 || toTile.recycler) return;
				if (!checkAdjacent(action.fromX, action.fromY, action.toX, action.toY)) return;

				if (isMyAction) {
					fromTile.myUnits -= action.amount;
					toTile.myUnits += action.amount;
				}
				else {
					fromTile.oppUnits -= action.amount;
					toTile.oppUnits += action.amount;
				}

				break;
			}
		}
	}

	void applyMultipleActions(const std::vector<Action> &actions, bool isMyAction, bool buildPhase) {
		for (const auto &act : actions) {
			if (buildPhase && act.type == ActionType::BUILD) {
				applyAction(act, isMyAction);
			}
			else if (!buildPhase && (act.type == ActionType::SPAWN || act.type == ActionType::MOVE)) {
				applyAction(act, isMyAction);
			}
		}
	}

	void resolveGameState() {
		// Resolve clashing units
		for (auto &tile : tiles) {
			if (tile.myUnits > 0 && tile.oppUnits > 0) {
				int clashedUnits = std::min(tile.myUnits, tile.oppUnits);
				tile.myUnits -= clashedUnits;
				tile.oppUnits -= clashedUnits;
			}

			if (tile.myUnits > 0) {
				tile.owner = ME;
			}
			else if (tile.oppUnits > 0) {
				tile.owner = OPP;
			}
		}

		// Resolve recylcers
		for (const auto &tile : tiles) {
			if (tile.recycler && tile.scrap_amount > 0) {
				int recyclerOwner = tile.owner;
				auto recycleTile = [&](int x, int y) {
					int idx = tileIndex(x, y);
					Tile &t = tiles[idx];
					if (t.scrap_amount > 0) {
						t.scrap_amount -= 1;
						if (recyclerOwner == ME) {
							myMatter++;
						}
						else if (recyclerOwner == OPP) {
							oppMatter++;
						}
					}
				};
				recycleTile(tile.x, tile.y);
				for (int d = 0; d < 4; d++) {
					if (checkInBounds(tile.x + dx[d], tile.y + dy[d], width, height)) {
						recycleTile(tile.x + dx[d], tile.y + dy[d]);
					}
				}
			}
		}

		// Resolve grass
		for (auto &tile : tiles) {
			if (tile.scrap_amount <= 0) {
				tile.scrap_amount = 0;
				tile.myUnits = 0;
				tile.oppUnits = 0;
				tile.recycler = false;
				tile.owner = NONE;
			}
		}

		// Resolve base income
		myMatter += 10;
		oppMatter += 10;
	}

	double computeFitness() const {
		int myOwnedTiles = 0, oppOwnedTiles = 0;
		int myUnitsCount = 0;
		for (const auto &tile :tiles) {
			if (tile.owner == ME && tile.scrap_amount > 0) {
				myOwnedTiles++;
				myUnitsCount += tile.myUnits;
			}
			if (tile.owner == OPP && tile.scrap_amount > 0) {
				oppOwnedTiles++;
			}
		}
		
		double sumOpponentDistances = 0.0;
		auto manhattanDistance = [&](int x1, int y1, int x2, int y2) -> int {
			return abs(x1 - x2) + abs(y1 - y2);
		};
		for (const Tile &tile : tiles) {
			if (tile.myUnits > 0 && tile.scrap_amount > 0) {
				int minDistOpponent = INF;
				for (const Tile &other : tiles) {
					if (other.scrap_amount <= 0) {
						continue;
					}
					if (other.owner == OPP) {
						int d = manhattanDistance(tile.x, tile.y, other.x, other.y);
						minDistOpponent = std::min(minDistOpponent, d);
					}
				}
				if (minDistOpponent == INF) {
					// To not award heavily when there are no opponent cells
					// (It just means our bot has won anyway)
					minDistOpponent = 0;
				}
				sumOpponentDistances += tile.myUnits * 1.0L * minDistOpponent;
			}
		}

		double sumNotOwnedDistances = 0.0L;
		for (const auto &tile : tiles) {
			if (tile.myUnits > 0 && tile.scrap_amount > 0) {
				int minDistNotOwned = INF;
				for (const Tile &other : tiles) {
					if (other.scrap_amount <= 0) {
						continue;
					}
					if (other.owner != ME) {
						int d = manhattanDistance(tile.x, tile.y, other.x, other.y);
						minDistNotOwned = std::min(minDistNotOwned, d);
					}
				}
				if (minDistNotOwned == INF) {
					// To not award heavily when there are no unclaimed cells
					// (It just means our bot has won anyway)
					minDistNotOwned = 0;
				}
				sumNotOwnedDistances += tile.myUnits * 1.0L * minDistNotOwned;
			}
		}

		auto inRangeOfRecycler = [&](const Tile &tile) -> int {
			if (tile.recycler && tile.owner == ME) {
				return tile.scrap_amount;
			}

			int minimumScraped = INF;
			for (int dir = 0; dir < 4; dir++) {
				int nx = tile.x + dx[dir];
				int ny = tile.y + dy[dir];
				if (!checkInBounds(nx, ny, width, height)) {
					continue;
				}

				int idx = tileIndex(nx, ny);
				const Tile &adjTile = tiles[idx];
				if (adjTile.recycler && adjTile.owner == ME) {
					minimumScraped = std::min(minimumScraped, adjTile.scrap_amount);
				}
			}

			if (minimumScraped == INF) {
				return -1;
			}
			else {
				return minimumScraped;
			}
		};

		double expectedResourcesGained = 0.0;
		for (const auto &tile : tiles) {
			if (tile.scrap_amount > 0 && inRangeOfRecycler(tile) >= 0) {
				expectedResourcesGained += inRangeOfRecycler(tile);
			}
		}

		double expectedTerritoryGained = 0.0;
		for (const auto &tile : tiles) {
			if (tile.scrap_amount > 0) {
				if (tile.owner == ME) {
					expectedTerritoryGained += 1.0;
				}
				else if (inRangeOfRecycler(tile)) {
					expectedTerritoryGained += 0.5;
				}
			}
		}

		// Let's say we start from the left, and we come across the following configuration
		// MMMMMOMOOOOO
		// Our units passed through each other, not an ideal situation as opponent can charge in our territory
		int cntL = 0, cntR = 0;
		for (int y = 0; y < height; y++) {
			int lastOwner = NONE;
			for (int x = 0; x < width; x++) {
				int idx = tileIndex(x, y);
				const Tile &tile = tiles[idx];
				if (lastOwner == ME && tile.owner == OPP) {
					cntL++;
				}
				if (lastOwner == OPP && tile.owner == ME) {
					cntR++;
				}
				lastOwner = tile.owner;
			}
		}
		int countAwkwardPositions = std::min(cntL, cntR);

		int minHeight = height + 1, maxHeight = -1;
		for (const Tile &tile : tiles) {
			if (tile.owner == ME && tile.myUnits > 0) {
				minHeight = std::min(minHeight, tile.y);
				maxHeight = std::max(maxHeight, tile.x);
			}
		}

		// Consider the case where our units have breached through opponent frontier
		// We should focus more on increasing our territory than attacking the opponent
		bool lateGameTraversal = false;
		if ((myOwnedTiles - oppOwnedTiles) > myOwnedTiles / 2) {
			lateGameTraversal = true;
		}

		// Penalize larger distances and awkward positions
		double weightOpponentDistance = -60.0;
		double weightUnownedDistance = -20.0;
		double weightAwkwardPosition = -15000.0;
		if (lateGameTraversal) {
			weightOpponentDistance = -10.0;
			weightUnownedDistance = -50.0;
		}
		// Reward larger yields
		double weightExpectedMaterial = 2.0;
		double weightExpectedTerritory = 2.5;
		double weightUnitsOwned = 100.0;
		double weightTilesOwned = 150.0;
		double weightVerticalSpread = 600.0;

		weightTilesOwned *= myOwnedTiles;
		weightUnitsOwned *= myUnitsCount;
		weightOpponentDistance *= sumOpponentDistances * sumOpponentDistances;
		weightUnownedDistance *= sumNotOwnedDistances * sumNotOwnedDistances;
		weightExpectedMaterial *= expectedResourcesGained * expectedResourcesGained;
		weightExpectedTerritory *= expectedTerritoryGained * expectedTerritoryGained;
		weightAwkwardPosition *= countAwkwardPositions;
		weightVerticalSpread *= maxHeight - minHeight;

		double fitness = weightTilesOwned + weightUnitsOwned + weightOpponentDistance + weightUnownedDistance +
						 weightExpectedMaterial + weightExpectedTerritory + weightAwkwardPosition + weightVerticalSpread;
		std::cerr << weightTilesOwned << " " << weightUnitsOwned << " " << weightOpponentDistance << " " << weightUnownedDistance << " "
				  << weightExpectedMaterial << " " << weightExpectedTerritory << " " << weightAwkwardPosition << " " << weightVerticalSpread << "\n";
		std::cerr << fitness / 1000.0L << "\n";
		return fitness;
	}

private:
	int myMatter;
	int oppMatter;
	int width, height;
	std::vector<Tile> tiles;
};

// ------------------------------------------------------
// GeneticAlgorithm Class
// ------------------------------------------------------
class GeneticAlgorithm {
public:
	struct Phenotype {
		std::vector<Action> actions;
	};

	GeneticAlgorithm(int _populationSize, int _generations, int _width, int _height, const std::vector<Tile>& tiles, int myMatter)
		: populationSize(_populationSize), generations(_generations), width(_width), height(_height) {
			population.resize(_populationSize);
			reservePopulation.resize(_populationSize);
			fitness.resize(_populationSize, 0.0);

			for (int i = 0; i < populationSize; i++) {
				population[i] = randomPhenotype(tiles, myMatter);
			}
			for (int i = 0; i < populationSize; i++) {
				reservePopulation[i] = randomPhenotype(tiles, myMatter);
			}
		}

	Phenotype randomPhenotype(const std::vector<Tile>& tiles, int myMatter) {
		Phenotype p;
		// Heuristic: use all of your resources
		int buildOrSpawn = myMatter / 10;
		
		// For building and spawning, consider cells that are closer to the opponent
		// Also, if there exists a recycler at some height
		// then we should not build a recycler at the same height, as it will clash and deplete our cells
		// In fact, we should not keep more than height/3? recyclers
		std::vector<bool> canBuildRecylcer(height, true);
		int countMyTiles = 0, countOppTiles = 0, countRecyclers = 0;
		for (const Tile &t : tiles) {
			if (t.owner == ME) {
				if (t.recycler) {
					canBuildRecylcer[t.y] = false;
					countRecyclers++;
				}
				countMyTiles++;
			}
			if (t.owner == OPP) {
				countOppTiles++;
			}
		}
		// Consider the case where our units have breached through opponent frontier
		// We should focus more on increasing our territory than attacking the opponent
		bool lateGameTraversal = false;
		if ((countMyTiles - countOppTiles) > countMyTiles / 2) {
			lateGameTraversal = true;
		}

		std::vector<Tile> oppTiles, oppUnits, myTilesSpawn, myTilesBuild;
		for (const Tile &t : tiles) {
			if (t.owner == OPP) {
				oppTiles.push_back(t);
				if (t.oppUnits > 0) {
					oppUnits.push_back(t);
				}
			}
			else if (t.owner == ME) {
				if (t.can_build && canBuildRecylcer[t.y] && countMyTiles - countOppTiles < height / 2 
					&& !lateGameTraversal && countRecyclers < height / 2) {
					myTilesBuild.push_back(t);
				}
				if (t.can_spawn && !t.in_range_of_recycler) {
					myTilesSpawn.push_back(t);
				}
			}
		}

		auto minDistToOppUnit = [&](Tile T) -> int {
			int res = width * height;
			for (const Tile &t : oppUnits) {
				res = std::min(res, abs(t.x - T.x) + abs(t.y - T.y));
			}
			return res;
		};

		// Only spawn robots at most 1 + *min_element away from opponent frontier
		std::sort(myTilesSpawn.begin(), myTilesSpawn.end(), [&](Tile A, Tile B) {
			return minDistToOppUnit(A) < minDistToOppUnit(B);
		});
		while (myTilesSpawn.size() 
			&& minDistToOppUnit(myTilesSpawn[0]) + 1 < minDistToOppUnit(myTilesSpawn.back())) {
			myTilesSpawn.pop_back();
		}
		while (myTilesSpawn.size() > height) {
			// Limit to the vertical spread of the spawning points
			myTilesSpawn.pop_back();
		}
		
		// Only build recyclers right next to opponent units to block them
		// or build far away to not block robot paths
		// Consider at most five build positions, and build at most three times
		std::sort(myTilesBuild.begin(), myTilesBuild.end(), [&](Tile A, Tile B) {
			return minDistToOppUnit(A) < minDistToOppUnit(B);
		});
		std::vector<Tile> temporary = myTilesBuild;
		while (!myTilesBuild.empty() && minDistToOppUnit(myTilesBuild.back()) != 0) {
			myTilesBuild.pop_back();
		}
		if (myTilesBuild.size() < 5) {
			int tempSize = (int)temporary.size();
			while (temporary.size() && myTilesBuild.size() < std::min(5, tempSize)) {
				myTilesBuild.push_back(temporary.back());
				temporary.pop_back();
			}
		}
		std::sort(myTilesBuild.begin(), myTilesBuild.end(), [&](Tile A, Tile B) {
			return minDistToOppUnit(A) < minDistToOppUnit(B);
		});
		myTilesBuild.erase(std::unique(myTilesBuild.begin(), myTilesBuild.end()), myTilesBuild.end());

		std::vector<int> considerAction;
		for (int i = 0; i < std::min(3, (int)myTilesBuild.size()); i++) {
			considerAction.push_back(0);
		}
		for (int i = 0; i < 2 * buildOrSpawn; i++) {
			considerAction.push_back(1);
		}
		shuffle(considerAction.begin(), considerAction.end(), rng);
		for (int i = 0; i < buildOrSpawn; i++) {
			if (considerAction[i] == 0) {
				// Build
				shuffle(myTilesBuild.begin(), myTilesBuild.end(), rng);
				Tile &toBuild = myTilesBuild.back();
				Action a;
				a.type = ActionType::BUILD;
				a.x = toBuild.x;
				a.y = toBuild.y;
				p.actions.push_back(a);
				myTilesBuild.pop_back();
			}
			else {
				// Spawn
				shuffle(myTilesSpawn.begin(), myTilesSpawn.end(), rng);
				Tile &toSpawn = myTilesSpawn.back();
				Action a;
				a.type = ActionType::SPAWN;
				a.amount = 1;
				a.x = toSpawn.x;
				a.y = toSpawn.y;
				p.actions.push_back(a);
			}
		}

		// Move
		auto minDistToOppTile = [&](std::pair<int, int> T) -> int {
			int res = width * height;
			for (const Tile &t : oppTiles) {
				res = std::min(res, abs(t.x - T.first) + abs(t.y - T.second));
			}
			return res;
		};
		std::vector<Tile> myUnits;
		for (const Tile &tile : tiles) {
			if (tile.owner == ME && tile.scrap_amount > 0 && tile.myUnits > 0) {
				myUnits.push_back(tile);
			}
		}

		for (const Tile &t : myUnits) {
			if (t.myUnits <= 0) continue;
			if (randRange(0, 4) < 2) {
				Action a;
				a.type = ActionType::MOVE;
				a.amount = randRange(1, t.myUnits);
				a.fromX = t.x;
				a.fromY = t.y;

				std::vector<std::pair<int, int>> possibleTo;
				std::vector<int> territory;
				for (int d = 0; d < 4; d++) {
					if (checkInBounds(t.x + dx[d], t.y + dy[d], width, height) && t.scrap_amount > 0) {
						possibleTo.push_back(std::make_pair(t.x + dx[d], t.y + dy[d]));
						territory.push_back(t.owner);
					}
				}
				
				// Heuristic: Increase chances to move to cells closer to opponent
				int maxDistance = 0;
				for (auto i : possibleTo) {
					maxDistance = std::max(maxDistance, minDistToOppTile(i));
				}
				maxDistance++;
				std::vector<std::pair<int, int>> samplePool;
				for (int i = 0; i < (int)possibleTo.size(); i++) {
					int populate = maxDistance - minDistToOppTile(possibleTo[i]);
					if (territory[i] == NONE) populate++;
					populate *= populate;
					for (int j = 0; j < populate; j++) {
						samplePool.push_back(possibleTo[i]);
					}
				}
				shuffle(samplePool.begin(), samplePool.end(), rng);

				if (samplePool.size()) {
					int possibleToIdx = randRange(0, (int)samplePool.size() - 1);
					a.toX = samplePool[possibleToIdx].first;
					a.toY = samplePool[possibleToIdx].second;
					p.actions.push_back(a);
				}
			}
		}

		// Reorder actions in order of BUILD, SPAWN, MOVE to allow easy crossover of phenotypes
		std::sort(p.actions.begin(), p.actions.end(), [&](Action A, Action B) {
			if (static_cast<int>(A.type) == static_cast<int>(B.type)) {
				if (A.type == ActionType::MOVE) {
					return std::make_pair(A.fromX, A.fromY) < std::make_pair(B.fromX, B.fromY);
				}
				else {
					return std::make_pair(A.x, A.y) < std::make_pair(B.x, B.y);
				}
			}
			return static_cast<int>(A.type) < static_cast<int>(B.type);
		});
		return p;
	}

	void evolvePopulation(GameState &gameState, const std::vector<Action> &dummyOppActions) {
		// Evaluate each candidate on some gameState clone and take fitness
		for (int i = 0; i < populationSize; i++) {
			GameState stateClone = gameState;

			// Apply BUILD Phase
			stateClone.applyMultipleActions(population[i].actions, true, true);
			stateClone.applyMultipleActions(dummyOppActions, false, true);
			// Apply SPAWN and MOVE Phase
			stateClone.applyMultipleActions(population[i].actions, true, false);
			stateClone.applyMultipleActions(dummyOppActions, false, false);

			stateClone.resolveGameState();
			fitness[i] = stateClone.computeFitness();
		}
		
		std::vector<int> indices(populationSize);
		std::iota(indices.begin(), indices.end(), 0);
		std::sort(indices.begin(), indices.end(), [&](int a, int b) {
			return fitness[a] > fitness[b];
		});

		// Crossover and mutation, replace the last half with new children from the first half children
		int half = populationSize / 2;
		auto rouletteWheelSelection = [&]() -> int {
			// Consider the first half
			double minimum_fitness = fitness[indices[half - 1]];
			double sum_of_fitness = 0.0L;
			for (int i = 0; i < half; i++) {
				sum_of_fitness += fitness[indices[i]];
			}
			// The probability of selection is fitness / sum_of_fitness
			// But minimum_fitness here can be negative
			// So, we can normalize it to 
			// (fitness - minimum_fitness + 1.0) / (sum_of_fitness - half * minimum_fitness + half)

			auto probabilityOfIndices = [&](int i) -> double {
				return (fitness[indices[i]] - minimum_fitness + 1.0L) / (sum_of_fitness - half * minimum_fitness + half);
			};
			std::uniform_real_distribution<> distDouble(0.0, 1.0);
			double prob = distDouble(rng);
			double cummulative_probability = 0.0L;
			for (int i = 0; i < half; i++) {
				cummulative_probability += probabilityOfIndices(i);
				if (cummulative_probability - prob > EPS) {
					return i;
				}
			}

			return half - 1;
		};

		for (int i = half; i < populationSize; i++) {
			int p1 = indices[rouletteWheelSelection()];
			int p2 = indices[rouletteWheelSelection()];
			Phenotype child = phenotypeCrossover(population[p1], population[p2]);
			mutatePhenotype(child);
			population[indices[i]] = child;
		}
	}

	void runEvolution(GameState &gameState, const std::vector<Action> &dummyOppActions)  {
		for (int gen = 0; gen < generations; gen++) {
			evolvePopulation(gameState, dummyOppActions);
		}
	}

	Phenotype getBestPhenotype() const {
		int bestIdx = 0;
		double bestFit = -INF;
		for (int i = 0; i < populationSize; i++) {
			if (fitness[i] > bestFit) {
				bestFit = fitness[i];
				bestIdx = i;
			}
		}
		return population[bestIdx];
	}

	void createDummyOpponentActions(std::vector<Action> &dummyOppActions, const std::vector<Tile> &tiles, int oppMatter) {
		// Opponent actions are just spawning and moving unit
		dummyOppActions.clear();

		auto minDistToMyTiles = [&](int x, int y) -> int {
			int minDist = width * height;
			for (const Tile &t : tiles) {
				if (t.owner == ME && t.scrap_amount > 0) {
					int d = abs(t.x - x) + abs(t.y - y);
					if (d < minDist) {
						minDist = d;
					}
				}
			}
			return minDist;
		};

		std::vector<Tile> oppTilesSpawn;
		for (const Tile &t : tiles) {
			if (t.owner == OPP && t.can_spawn) {
				oppTilesSpawn.push_back(t);
			}
		}
		std::sort(oppTilesSpawn.begin(), oppTilesSpawn.end(), [&](Tile a, Tile b) {
			return minDistToMyTiles(a.x, a.y) < minDistToMyTiles(b.x, b.y);
		});

		while (oppTilesSpawn.size()) {
			Tile &bk = oppTilesSpawn.back();
			Tile &fr = oppTilesSpawn[0];
			if (minDistToMyTiles(bk.x, bk.y) < minDistToMyTiles(fr.x, fr.y) + 2) {
				break;
			}
			oppTilesSpawn.pop_back();
		}

		for (int i = 0; oppTilesSpawn.size() && i < oppMatter / 10; i++) {
			Action a;
			a.type = ActionType::SPAWN;
			a.x = oppTilesSpawn[i % oppTilesSpawn.size()].x;
			a.y = oppTilesSpawn[i % oppTilesSpawn.size()].y;
			a.amount = 1;
			dummyOppActions.push_back(a);
		}

		for (const Tile &t : tiles) {
			if (t.owner == OPP && t.oppUnits > 0) {
				int currentDist = minDistToMyTiles(t.x, t.y);
				int bestDist = currentDist;
				int bestX = t.x, bestY = t.y;
				for (int dir = 0; dir < 4; dir++) {
					int nx = t.x + dx[dir];
					int ny = t.y + dy[dir];
					if (!checkInBounds(nx, ny, width, height)) {
						continue;
					}
					int d = minDistToMyTiles(nx, ny);
					if (d < bestDist) {
						bestDist = d;
						bestX = nx;
						bestY = ny;
					}
				}

				if (bestDist < currentDist) {
					Action a;
					a.type = ActionType::MOVE;
					a.fromX = t.x;
					a.fromY = t.y;
					a.toX = bestX;
					a.toY = bestY;
					a.amount = randRange(1, t.oppUnits);
					dummyOppActions.push_back(a);
				}
			}
		}
	}

private:
	int populationSize;
	int generations;
	int width, height;
	std::vector<Phenotype> population, reservePopulation;
	std::vector<double> fitness;

	Phenotype phenotypeCrossover(const Phenotype &p1, const Phenotype &p2, int probability = 2) {
		// Uniform crossover with probability (1/probability) from p2 side
		Phenotype child;
		size_t maxSize = std::max(p1.actions.size(), p2.actions.size());
		for (size_t i = 0; i < maxSize; i++) {
			if (randRange(0, probability - 1) == 0 && i < p2.actions.size()) {
				child.actions.push_back(p2.actions[i]);
			}
			else if (i < p1.actions.size()) {
				child.actions.push_back(p1.actions[i]);
			}
		}
		return child;
	}

	void mutatePhenotype(Phenotype &p) {
		// Good mutation, takes actions that are from a randomPhenotype
		Phenotype toCrossover = reservePopulation[randRange(0, populationSize - 1)];
		p = phenotypeCrossover(p, toCrossover, 40); // 2.5% Mutation Rate
	}
};

const int POPULATION_SIZE = 16;
const int GENERATIONS = 20;

// ------------------------------------------------------
// Main function
// ------------------------------------------------------
int main() {
	std::ios::sync_with_stdio(false);
	std::cin.tie(nullptr);

	int width, height;
	std::cin >> width >> height;
	std::cin.ignore();

	while (true) {
		int myMatter, oppMatter;
		std::cin >> myMatter >> oppMatter;
		std::cin.ignore();

		std::vector<Tile> tiles;
		tiles.reserve(width * height);

		std::vector<Tile> myTiles, oppTiles, neutralTiles, myUnitTiles, oppUnitTiles;
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				int scrap_amount, owner, units, recycler, can_build, can_spawn, in_range_of_recycler;
				std::cin >> scrap_amount >> owner >> units >> recycler >> can_build >> can_spawn >> in_range_of_recycler;
				std::cin.ignore();

				Tile tile;
				tile.x = x;
				tile.y = y;
				tile.scrap_amount = scrap_amount;
				tile.recycler = (recycler == 1);
				tile.can_build = (can_build == 1);
				tile.can_spawn = (can_spawn == 1);
				tile.in_range_of_recycler = (in_range_of_recycler == 1);
				tile.owner = owner;

				if (owner == ME) {
					tile.myUnits = units;
					tile.oppUnits = 0;
					myTiles.push_back(tile);
					if (units > 0) {
						myUnitTiles.push_back(tile);
					}
				}
				else if (owner == OPP) {
					tile.myUnits = 0;
					tile.oppUnits = units;
					oppTiles.push_back(tile);
					if (units > 0) {
						oppUnitTiles.push_back(tile);
					}
				}
				else {
					tile.myUnits = 0;
					tile.oppUnits = 0;
					neutralTiles.push_back(tile);
				}
				tiles.push_back(tile);
			}
		}

		GameState gameState(myMatter, oppMatter, width, height, tiles);
		GeneticAlgorithm ga(POPULATION_SIZE - (width * height > 200 ? 5 : 0), GENERATIONS - (width * height > 200 ? 3 : 0), width, height, tiles, myMatter);

		std::vector<Action> dummyOppActions;
		ga.createDummyOpponentActions(dummyOppActions, tiles, oppMatter);
		ga.runEvolution(gameState, dummyOppActions);
		std::vector<Action> bestActions = (ga.getBestPhenotype()).actions;

		if (bestActions.empty()) {
			std::cout << "WAIT" << std::endl;
		}
		else {
			std::ostringstream output;
			for (const Action &act : bestActions) {
				switch(act.type) {
					case ActionType::BUILD:
						output << "BUILD " << act.x << " " << act.y << ";";
						break;
					case ActionType::SPAWN:
						output << "SPAWN " << act.amount << " " << act.x << " " << act.y << ";";
						break;
					case ActionType::MOVE:
						output << "MOVE " << act.amount << " " << act.fromX << " " << act.fromY << " "
							   << act.toX << " " << act.toY << ";";
						break;
				}
			}
			std::cout << output.str() << std::endl;
		}
	}

	return 0;
}



