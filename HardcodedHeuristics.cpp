#include <bits/stdc++.h>
using namespace std;

mt19937 rng(time(NULL));
const int MAXN = 30;
int n, m;
bool flip = 0;

set<pair<int, int>> builtRecycler;

inline void printBuildRecycler(int x, int y, int type) {
    builtRecycler.insert({x, y});
    if (flip) {
        y = m - 1 - y;
        x = n - 1 - x;
    }
    cerr << type << "BUILD " << y << " " << x << endl;
    cout << "BUILD " << y << " " << x << ";";
}

inline void printMoveUnit(int cnt, int x, int y, int tx, int ty, int type) {
    if (flip) {
        y = m - 1 - y;
        x = n - 1 - x;
        ty = m - 1 - ty;
        tx = n - 1 - tx;
    }
    if (cnt <= 0) return;
    cerr << type << "MOVE " << cnt << " " << y << " " << x << " " << ty << " " << tx << endl;
    cout << "MOVE " << cnt << " " << y << " " << x << " " << ty << " " << tx << ";";
}

inline void printSpawnUnit(int cnt, int x, int y, int type) {
    if (flip) {
        y = m - 1 - y;
        x = n - 1 - x;
    }
    if (cnt <= 0) return;
    cerr << type << "SPAWN " << cnt << " " << y << " " << x << endl;
    cout << "SPAWN " << cnt << " " << y << " " << x << ";";
}

inline void printWait() {
    cout << "WAIT" << endl;
}

const int dx[] = {1, 0, -1, 0};
const int dy[] = {0, 1, 0, -1};

struct cell_info {
    int scrap_amount;
    int owner; // 1 = me, 0 = foe, -1 = neutral
    int units;
    int recycler;
    int can_build;
    int can_spawn;
    int in_range_of_recycler;

    void read() {
        int a;
        cin >> a; scrap_amount = a;
        cin >> a; owner = a;
        cin >> a; units = a;
        cin >> a; recycler = a;
        cin >> a; can_build = a;
        cin >> a; can_spawn = a;
        cin >> a; in_range_of_recycler = a;
    }
} v[MAXN][MAXN];

inline int cellToInt(int x, int y) {
    return x * MAXN + y;
}

inline pair<int, int> intToCell(int a) {
    return {a / MAXN, a % MAXN};
}

int par[MAXN * MAXN], sz[MAXN * MAXN], szour[MAXN * MAXN];

inline int f(int x) {
    return par[x] = (par[x] == x ? x : f(par[x]));
}

inline void uni(int a, int b, int c, int d) {
    int pa = f(cellToInt(a, b));
    int pb = f(cellToInt(c, d));
    if (pa != pb) {
        sz[pb] += sz[pa];
        szour[pb] += szour[pa];
        par[pa] = pb;
    }
}

inline bool reachable(int x, int y, int tx, int ty) {
    return f(cellToInt(x, y)) == f(cellToInt(tx, ty));
}

inline int toBeExplored(int x, int y) {
    return sz[f(cellToInt(x, y))];
}

inline bool impossibleToConq(int x, int y) {
    return szour[f(cellToInt(x, y))] == 0;
}

inline int dist(pair<int, int> a, pair<int, int> b) {
    return abs(a.first - b.first) + abs(a.second - b.second);
}

int my_matter, opp_matter;

void read();
void initTurnOne();
void init(int turnNumber);
void solve(int turnNumber);

int main() {
    cin >> n >> m;
    swap(n, m);

    for (int turnNumber = 1; turnNumber <= 200; turnNumber++) {
        read();
        if (turnNumber == 1) initTurnOne();
        init(turnNumber);
        solve(turnNumber);
    }
}

void read() {
    cin >> my_matter >> opp_matter;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            v[i][j].read();
        }
    }
}

pair<int, int> enemyCenter, ourCenter;

void initTurnOne() {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (v[i][j].owner == 1) {
                if (j > m / 2) {
                    flip = 1;
                }
            }
        }
    }
}

set<pair<int, int>> goodForRecyclers;
map<int, vector<pair<int, int>>> areasmp;
vector<vector<pair<int, int>>> areas;

void init(int turnNumber) {
    builtRecycler.clear();
    goodForRecyclers.clear();

    if (flip) {
        // Flip the board (mirroring)
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m / 2; j++) {
                swap(v[i][j], v[n - 1 - i][m - 1 - j]);
            }
        }
        if (m & 1) {
            for (int i = 0; i < (n + 1) / 2; i++) {
                swap(v[i][m / 2], v[n - 1 - i][m / 2]);
            }
        }
    }

    for (int i = 0; i < MAXN * MAXN; i++) {
        par[i] = i;
        sz[i] = 0;
        szour[i] = 0;
    }

    // Initialize union-find and track centers
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (v[i][j].owner != 1) {
                sz[cellToInt(i, j)]++;
            }
            if (v[i][j].owner == 1 && v[i][j].units == 0 && turnNumber == 1) {
                ourCenter = {i, j};
            }
            if (v[i][j].owner == 0 && v[i][j].units == 0 && turnNumber == 1) {
                enemyCenter = {i, j};
            }
        }
    }

    // Mark good spots for recyclers and unify connected cells
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (v[i][j].scrap_amount != 0 && v[i][j].recycler == 0) {
                int cr = v[i][j].scrap_amount;
                int sum = cr;
                for (int d = 0; d < 4; d++) {
                    int ni = i + dx[d];
                    int nj = j + dy[d];
                    if (0 <= ni && ni < n && 0 <= nj && nj < m) {
                        if (v[ni][nj].scrap_amount != 0 && v[ni][nj].recycler == 0) {
                            sum += min(cr, v[ni][nj].scrap_amount);
                            uni(i, j, ni, nj);
                        }
                    }
                }
                if (sum >= 20) {
                    goodForRecyclers.insert({i, j});
                }
            }
            if (v[i][j].owner == 1 && v[i][j].recycler == 1) {
                builtRecycler.insert({i, j});
            }
        }
    }

    areasmp.clear();
    areas.clear();

    // Build areas from union-find
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (v[i][j].scrap_amount != 0) {
                int gt = f(cellToInt(i, j));
                areasmp[gt].push_back({i, j});
            }
        }
    }
    for (auto& i : areasmp) {
        areas.push_back(i.second);
    }
}

int countDoneUp = 0, countDoneDown = 0;
int downNotDone = 1, upNotDone = 1;
int recyclersPlanted = 0;

void solve(int turnNumber) {
    // Turn 1 logic
    if (turnNumber == 1) {
        if (n * m >= 100 && goodForRecyclers.count(ourCenter)) {
            printBuildRecycler(ourCenter.first, ourCenter.second, 0);
            recyclersPlanted++;
        }
        printMoveUnit(1, ourCenter.first, ourCenter.second - 1, ourCenter.first, ourCenter.second, 0);
        printMoveUnit(1, ourCenter.first - 1, ourCenter.second, ourCenter.first - 1, ourCenter.second + 1, 0);
        printMoveUnit(1, ourCenter.first + 1, ourCenter.second, ourCenter.first + 1, ourCenter.second + 1, 0);
        printMoveUnit(1, ourCenter.first, ourCenter.second + 1, enemyCenter.first, enemyCenter.second, 0);
        printWait();
        return;
    }

    // Early recycler planting logic
    if (recyclersPlanted == 0 && n * m >= 100) {
        bool done = false;
        for (int j = 0; !done && j < m; j++) {
            for (int i = 0; !done && i < n; i++) {
                if (v[i][j].owner == 1 && v[i][j].units == 0 && v[i][j].recycler == 0 &&
                    goodForRecyclers.count({i, j})) 
                {
                    printBuildRecycler(i, j, 1);
                    my_matter -= 10;
                    done = true;
                }
            }
        }
        recyclersPlanted++;
    } 
    else if (recyclersPlanted == 1 && n * m >= 180 && turnNumber >= 6 && turnNumber <= 10) {
        bool done = false;
        for (int j = 0; !done && j < m; j++) {
            for (int i = 0; !done && i < n; i++) {
                if (v[i][j].owner == 1 && v[i][j].units == 0 && v[i][j].recycler == 0 &&
                    goodForRecyclers.count({i, j})) 
                {
                    printBuildRecycler(i, j, 2);
                    my_matter -= 10;
                    done = true;
                }
            }
        }
        recyclersPlanted++;
    }

    // Sort areas by size
    sort(areas.begin(), areas.end(), [&](vector<pair<int, int>>& A, vector<pair<int, int>>& B) {
        return A.size() > B.size();
    });

    // Main loop over each area
    for (auto& cells : areas) {
        bool hasOpp = false, hasOppUnit = false;
        int cnta = 0, cntb = 0;

        // Various containers for different cell types
        vector<pair<int, int>> freeUnit;
        vector<pair<int, int>> importantCellNonAlert;
        vector<pair<int, int>> notExplored;
        vector<pair<int, int>> ourCell;
        vector<pair<int, int>> ourCellNoUnit;
        vector<pair<int, int>> oppCell;
        vector<pair<pair<int, int>, int>> importantUnit;
        vector<pair<pair<int, int>, int>> importantCellAlert;

        set<int> colu;
        colu.insert(-1);
        colu.insert(n);

        int cntunitup = 0, cntunitdn = 0;

        // Classify cells in this area
        for (auto& e : cells) {
            int x = e.first;
            int y = e.second;

            if (v[x][y].owner == 0) {
                oppCell.push_back(e);
                hasOpp = true;
                if (v[x][y].units > 0) {
                    hasOppUnit = true;
                    cntb += v[x][y].units;
                }
            }

            if (v[x][y].owner != 1) {
                notExplored.push_back(e);
            }

            if (v[x][y].owner == 1) {
                colu.insert(x);
                cnta += v[x][y].units;
                if (x < n / 2) cntunitup += v[x][y].units;
                else cntunitdn += v[x][y].units;
                ourCell.push_back(e);

                bool important = false;
                int countOpp = 0;
                for (int d = 0; d < 4; d++) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (0 <= nx && nx < n && 0 <= ny && ny < m) {
                        if (v[nx][ny].scrap_amount > 0 && v[nx][ny].owner == 0) {
                            important = true;
                            countOpp += v[nx][ny].units;
                        }
                    }
                }

                if (important) {
                    if (v[x][y].units > 0) {
                        if (countOpp > 0) {
                            importantUnit.push_back({{x, y}, countOpp - v[x][y].units});
                        } else {
                            freeUnit.push_back({x, y});
                        }
                    } else {
                        if (countOpp > 0) {
                            importantCellAlert.push_back({{x, y}, countOpp});
                        } else {
                            importantCellNonAlert.push_back({x, y});
                        }
                    }
                } else {
                    ourCellNoUnit.push_back({x, y});
                    if (v[x][y].units > 0) {
                        freeUnit.push_back({x, y});
                    }
                }
            }
        }

        // If no opponent units here, we can expand/spawn
        if (!hasOppUnit) {
            if (freeUnit.empty()) {
                int times = my_matter / 10;
                if (oppCell.empty()) times = 1; // fallback to at least 1 spawn

                for (auto& i : ourCell) {
                    if (times == 0) break;
                    if (my_matter >= 10) {
                        printSpawnUnit(1, i.first, i.second, 3);
                        my_matter -= 10;
                        times--;
                    }
                }
            }

            sort(notExplored.begin(), notExplored.end(), [&](pair<int, int> X, pair<int, int> Y) {
                // Sort so that cells with an opponent owner come first, else by decreasing column index
                if (v[X.first][X.second].owner == 0) {
                    if (v[Y.first][Y.second].owner == 0) {
                        return X.second > Y.second;
                    } else {
                        return true;
                    }
                } else if (v[Y.first][Y.second].owner == 0) {
                    return false;
                }
                return X.second > Y.second;
            });

            for (auto& i : notExplored) {
                if (freeUnit.empty()) continue;
                sort(freeUnit.begin(), freeUnit.end(), [&](pair<int, int> A, pair<int, int> B) {
                    // Sort free units by distance to target, focusing on row difference first
                    if (abs(A.first - i.first) == abs(B.first - i.first)) {
                        return dist(A, i) > dist(B, i);
                    }
                    return abs(A.first - i.first) > abs(B.first - i.first);
                });

                pair<int, int> get = freeUnit.back();
                freeUnit.pop_back();
                printMoveUnit(1, get.first, get.second, i.first, i.second, 4);
                v[get.first][get.second].units--;
                if (v[get.first][get.second].units) {
                    freeUnit.push_back(get);
                }
            }
            continue;
        }

        // If there are opponent units
        // Sort alert cells by ascending opponent count
        sort(importantCellAlert.begin(), importantCellAlert.end(),
             [&](pair<pair<int, int>, int> X, pair<pair<int, int>, int> Y) {
                 return X.second < Y.second;
             });

        // Possibly build recyclers or spawn to defend
        for (auto& i : importantCellAlert) {
            int countUnit = 0;
            for (int d = 0; d < 4; d++) {
                int nx = i.first.first + dx[d];
                int ny = i.first.second + dy[d];
                if (0 <= nx && nx < n && 0 <= ny && ny < m) {
                    if (v[nx][ny].owner == 0) {
                        countUnit += v[nx][ny].units;
                    }
                }
            }
            if (my_matter >= 10) {
                if (my_matter >= 20 * countUnit) {
                    printSpawnUnit(countUnit, i.first.first, i.first.second, 5);
                    my_matter -= countUnit;
                } else {
                    printBuildRecycler(i.first.first, i.first.second, 5);
                    my_matter -= 10;
                }
            }
        }

        // Sort important units
        sort(importantUnit.begin(), importantUnit.end(),
             [&](pair<pair<int, int>, int> X, pair<pair<int, int>, int> Y) {
                 return abs(X.second) < abs(Y.second);
             });

        // Attack or spawn additional units
        for (auto& i : importantUnit) {
            int x = i.first.first;
            int y = i.first.second;
            if (i.second >= 0) {
                int mn = min(my_matter / 10, i.second);
                printSpawnUnit(mn, x, y, 6);
                my_matter -= 10 * mn;
            } else {
                for (int d = 0; d < 4; d++) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (0 <= nx && nx < n && 0 <= ny && ny < m) {
                        if (v[nx][ny].scrap_amount > 0 && v[nx][ny].owner == 0) {
                            if (d != 1) {
                                printMoveUnit(v[nx][ny].units, x, y, nx, ny, 7);
                            } else {
                                printMoveUnit(-i.second, x, y, nx, ny, 7);
                            }
                        }
                    }
                }
            }
        }

        // If we have fewer units or losing balance, spawn more
        if (cnta == 0 || cntb - cnta >= 5) {
            sort(ourCell.begin(), ourCell.end(), [&](pair<int, int> A, pair<int, int> B) {
                return A.second > B.second;
            });
            int times = min(5, my_matter / 10 - 2);
            for (auto& i : ourCell) {
                if (times <= 0) break;
                if (my_matter >= 10) {
                    printSpawnUnit(1, i.first, i.second, 8);
                    my_matter -= 10;
                    times--;
                }
            }
        }

        // Spawn or build additional recyclers
        int approxrecycneeded = max(0, 3 - (my_matter / 10));
        sort(importantCellNonAlert.begin(), importantCellNonAlert.end(),
             [&](pair<int, int> A, pair<int, int> B) {
                 if (A.second == B.second) {
                     return abs(A.first - ourCenter.first) > abs(B.first - ourCenter.first);
                 }
                 return A.second > B.second;
             });

        for (auto& i : importantCellNonAlert) {
            if (my_matter < 10) break;
            if (approxrecycneeded > 0 && i.second > m / 2) {
                printBuildRecycler(i.first, i.second, 9);
                my_matter -= 10;
                approxrecycneeded--;
            } else {
                printSpawnUnit(1, i.first, i.second, 9);
                my_matter -= 10;
            }
        }

        // Spawn in remaining cells
        int cntunit = max(0, my_matter / 10 - 2);
        int countCells = min(n, (int)ourCell.size());
        if (countCells) {
            int perCell = cntunit / countCells;
            for (auto& i : ourCell) {
                printSpawnUnit(perCell, i.first, i.second, 10);
                my_matter -= perCell * 10;
            }
        }

        // Move free units toward the right or up/down
        sort(freeUnit.begin(), freeUnit.end(),
             [&](pair<int, int> A, pair<int, int> B) {
                 if (abs(A.first - ourCenter.first) == abs(B.first - ourCenter.first)) {
                     return A.second > B.second;
                 }
                 return abs(A.first - ourCenter.first) > abs(B.first - ourCenter.first);
             });

        for (auto& i : freeUnit) {
            int x = i.first;
            int y = i.second;
            int nx, ny;

            bool right = false, up = false, down = false;
            bool rightExist = false, upExist = false, downExist = false;

            // Check to the right
            nx = x;
            ny = y + 1;
            if (0 <= nx && nx < n && 0 <= ny && ny < m && v[nx][ny].scrap_amount > 0 && v[nx][ny].recycler == 0) {
                rightExist = true;
                if (v[nx][ny].owner == 1) {
                    right = true;
                }
            }

            // Check above
            nx = x - 1;
            ny = y;
            if (0 <= nx && nx < n && 0 <= ny && ny < m && v[nx][ny].scrap_amount > 0 && v[nx][ny].recycler == 0) {
                upExist = true;
                if (v[nx][ny].owner == 1) {
                    up = true;
                }
            }

            // Check below
            nx = x + 1;
            ny = y;
            if (0 <= nx && nx < n && 0 <= ny && ny < m && v[nx][ny].scrap_amount > 0 && v[nx][ny].recycler == 0) {
                downExist = true;
                if (v[nx][ny].owner == 1) {
                    down = true;
                }
            }

            if (!rightExist) {
                // Search deeper to the right if possible
                vector<pair<int, int>> cr = {make_pair(x, y)};
                vector<vector<bool>> visi(n, vector<bool>(m, false));
                pair<int, int> gt = {-1, -1};

                for (int depth = 1; depth <= 5 * n + 5 * m; depth++) {
                    for (auto& e : cr) {
                        visi[e.first][e.second] = true;
                    }
                    vector<pair<int, int>> nxStep;
                    for (auto& e : cr) {
                        for (int d = 0; d < 4; d++) {
                            int ax = e.first + dx[d];
                            int ay = e.second + dy[d];
                            if (0 <= ax && ax < n && 0 <= ay && ay < m) {
                                if (v[ax][ay].scrap_amount > 0 && v[ax][ay].recycler == 0 && !visi[ax][ay]) {
                                    nxStep.push_back({ax, ay});
                                }
                            }
                        }
                    }
                    sort(nxStep.begin(), nxStep.end(), [&](pair<int, int> A, pair<int, int> B) {
                        if (A.second == B.second) {
                            return abs(A.first - ourCenter.first) > abs(B.first - ourCenter.first);
                        }
                        return A.second > B.second;
                    });
                    for (auto& e : nxStep) {
                        if (v[e.first][e.second].owner != 1) {
                            gt = e;
                            break;
                        }
                    }
                    if (gt.first != -1) break;
                    cr = nxStep;
                }
                if (gt.first != -1) {
                    printMoveUnit(v[x][y].units, x, y, gt.first, gt.second, 13);
                }
            } else {
                // If there is a valid right
                if (right) {
                    if (!upExist && !downExist) {
                        printMoveUnit(v[x][y].units, x, y, x, y + 1, 11);
                    } else if (upExist && downExist) {
                        // If both up and down exist
                        if (up && down) {
                            printMoveUnit(v[x][y].units, x, y, x, y + 1, 11);
                        } else if (!up && !down) {
                            // Move vertically based on position
                            if (x < n / 2) {
                                printMoveUnit(v[x][y].units, x, y, x - 1, y, 11);
                            } else {
                                printMoveUnit(v[x][y].units, x, y, x + 1, y, 11);
                            }
                        } else if (!up) {
                            printMoveUnit(v[x][y].units, x, y, x - 1, y, 11);
                        } else if (!down) {
                            printMoveUnit(v[x][y].units, x, y, x + 1, y, 11);
                        }
                    } else if (upExist) {
                        if (!up) {
                            printMoveUnit(1, x, y, x - 1, y, 11);
                            v[x][y].units--;
                        }
                        printMoveUnit(v[x][y].units, x, y, x, y + 1, 11);
                    } else {
                        if (!down) {
                            printMoveUnit(1, x, y, x + 1, y, 11);
                            v[x][y].units--;
                        }
                        printMoveUnit(v[x][y].units, x, y, x, y + 1, 11);
                    }
                } else {
                    bool apparentlyDone = false;
                    if (v[x][y].units > 2) {
                        if (upExist && !up) {
                            printMoveUnit(1, x, y, x - 1, y, 11);
                            v[x][y].units--;
                        }
                        if (downExist && !down) {
                            printMoveUnit(1, x, y, x + 1, y, 11);
                            v[x][y].units--;
                        }
                    } else if (v[x][y].units == 1) {
                        // Single unit - do nothing special here
                    } else {
                        // 2 units?
                        if (!upExist || up) {
                            if (downExist && !down) {
                                printMoveUnit(1, x, y, x + 1, y, 11);
                                v[x][y].units--;
                            }
                        } else if (!downExist || down) {
                            if (upExist && !up) {
                                printMoveUnit(1, x, y, x - 1, y, 11);
                                v[x][y].units--;
                            }
                        } else {
                            int goUp = x - 1;
                            int goDown = x + 1;
                            if (!colu.count(goUp) && colu.count(goUp - 1) && colu.count(goUp + 1)) {
                                printMoveUnit(1, x, y, x - 1, y, 11);
                                v[x][y].units--;
                            } else if (!colu.count(goDown) && colu.count(goDown - 1) && colu.count(goDown + 1)) {
                                printMoveUnit(1, x, y, x + 1, y, 11);
                                v[x][y].units--;
                            } else if (x < ourCenter.first && countDoneUp > countDoneDown && downNotDone) {
                                // no action
                            } else if (x >= ourCenter.first && countDoneUp < countDoneDown && upNotDone) {
                                // no action
                            } else {
                                v[x][y].units--;
                                apparentlyDone = true;
                            }
                        }
                    }
                    printMoveUnit(v[x][y].units, x, y, x, y + 1, 11);

                    // Spawn if we have enough matter
                    if (my_matter >= 10 && !apparentlyDone) {
                        if (x < ourCenter.first && countDoneUp > countDoneDown && downNotDone) {
                            // no spawn
                        } else if (x >= ourCenter.first && countDoneUp < countDoneDown && upNotDone) {
                            // no spawn
                        } else {
                            if (x < ourCenter.first) countDoneUp++;
                            else countDoneDown++;
                            if (x == 0) upNotDone = 0;
                            if (x == n - 1) downNotDone = 0;
                            printSpawnUnit(1, x, y, 11);
                            my_matter -= 10;
                        }
                    }
                }
            }
        }

        // Build or spawn in remaining unoccupied cells near opponent
        vector<pair<pair<int, int>, int>> distcell;
        for (auto& i : ourCellNoUnit) {
            int mn = 1e9;
            for (auto& j : oppCell) {
                mn = min(mn, dist(i, j));
            }
            distcell.push_back({i, mn});
        }

        sort(distcell.begin(), distcell.end(),
             [&](pair<pair<int, int>, int> A, pair<pair<int, int>, int> B) {
                 return A.second < B.second;
             });

        for (auto& i : distcell) {
            if (my_matter < 10) break;
            bool ok = true;
            for (int d = 0; d < 4; d++) {
                if (builtRecycler.count({i.first.first + dx[d], i.first.second + dy[d]})) {
                    ok = false;
                }
            }
            if (!ok) continue;
            if (i.second <= 3 && builtRecycler.size() < n / 3) {
                builtRecycler.insert({i.first.first, i.first.second});
                printBuildRecycler(i.first.first, i.first.second, 12);
            } else {
                printSpawnUnit(1, i.first.first, i.first.second, 12);
            }
            my_matter -= 10;
        }
    }

    // Finally, wait
    printWait();
    return;
}

// TO DO: IMPROVE INITIAL SPREAD TO BE MORE DIFFUSIVE
