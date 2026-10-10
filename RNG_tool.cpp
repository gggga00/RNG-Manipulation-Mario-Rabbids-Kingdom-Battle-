#include <iostream>
#include <iomanip>
#include <cmath>
#include <tuple>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <fstream>
#include <utility>
#include <numeric>
#include <array>

using namespace std;

constexpr uint32_t u31 = 1u << 31;

constexpr int MIN_W = 1;
constexpr int MIN_H = 1;
constexpr int MAX_W = 128;
constexpr int MAX_H = 128;

// Bit flags for cardinal neighbours.
constexpr uint8_t N_UP    = 1u << 0;
constexpr uint8_t N_RIGHT = 1u << 1;
constexpr uint8_t N_DOWN  = 1u << 2;
constexpr uint8_t N_LEFT  = 1u << 3;

// Burn stats
constexpr float myMaxRadius = 30.0;
constexpr int myNumOfAreas = 2;
constexpr int myNumOfPointsForCompletelyRandomPath = 6;
constexpr int myAreaMinSize = 3;
constexpr int myAreaMaxSize = 5;
constexpr int myNumOfPointsPerArea = 12;
constexpr float myMinDistanceBetweenPoints = 4.0;
constexpr float myMaxDistanceBetweenPoints = 10.0;
constexpr int myMaxConsecutiveSkips = 100;
constexpr float myInitAngle = 120.0;
constexpr float myAngleRange = 450.0;
constexpr float myBurnLenght = 30.0;
constexpr float myBurnSpeed = 9.0;
constexpr float myBurnAccelerationDistance = 2.0;
constexpr float myBurnDecelerationDistance = 3.0;
constexpr bool myDoNotEndInCover = true;
constexpr float myAlliesAvoidanceProbability = 0.0;
constexpr float myEnemiesAvoidanceProbability = 0.0;
constexpr float myPropagationRadius = 1.0;
constexpr float myBoundingRadius = 0.95;
constexpr int myMaxAlliesPropagationTimes = 2;
constexpr int myMaxEnemiesPropagationTimes = 1;
constexpr float myBlockedWaitInMillisec = 500;
constexpr float myTimeToStayInPreviewIfPropagated = 0.3;
//Special Burn stats
constexpr float PirrabidPlantPropagationRadius = 1.7;
constexpr float	PirrabidPlantBoundingRadius = 1.2;
constexpr float BucklerPropagationRadius = 1.7;
constexpr float	BucklerBoundingRadius = 1.2;
constexpr float SmasherPropagationRadius = 1.7;
constexpr float	SmasherBoundingRadius = 1.2;
constexpr float SmasherBurnSpeed = 8.0;
constexpr float SpecialBucklerPropagationRadius = 1.7;
constexpr float	SpecialBucklerBoundingRadius = 1.2;
constexpr int SpecialBucklerAreaMinSize = 2;
constexpr int SpecialBucklerAreaMaxSize = 2;
constexpr int SpecialBucklerMaxConsecutiveSkips = 25;

struct Affine {
    int mult;
    int add;
};

struct Tile {
    bool on = false;
    uint8_t neighbours = 0;
    int x;
    int y;
};

class Map {
public:
    Map(int width = 31, int height = 31) { resize(width, height); }

    int width() const { return width_; }
    int height() const { return height_; }

    int getIndex(int x, int y) {return index(x, y);}

    bool inBounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < width_ && y < height_;
    }

    bool isOn(int x, int y) const {
        return inBounds(x, y) && cells_[index(x, y)].on;
    }

    Map operator&(Map b) {
        int w = min(width_, b.width());
        int h = min(height_, b.height());

        Map out;
        out.resize(w, h);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                out.set(x, y, isOn(x, y) & b.isOn(x, y));
            }
        }
        return out;
    }


    uint8_t neighbourMask(int x, int y) const {
        return inBounds(x, y) ? cells_[index(x, y)].neighbours : 0;
    }

    void set(int x, int y, bool on) {
        if (!inBounds(x, y)) return;
        cells_[index(x, y)].on = on;
    }

    void setNeighbours(int x, int y, uint32_t neighbours) {
        // UP = +1
        // RIGHT = +2
        // DOWN = +4
        // LEFT = +8
        if (!inBounds(x, y)) return;
        cells_[index(x, y)].neighbours = neighbours;
    }

    vector<Tile> getNeighbours(int x, int y) {
        // Correct order: LEFT, RIGHT, DOWN, UP
        vector<Tile> neighbourList;
        if (!inBounds(x, y)) return neighbourList;
        uint32_t neighbours = cells_[index(x, y)].neighbours;
        if((neighbours & 8) && inBounds(x-1, y)) {
            Tile left;
            left.x = x-1;
            left.y = y;
            neighbourList.push_back(left);
        }
        if((neighbours & 2) && inBounds(x+1, y)) {
            Tile right;
            right.x = x+1;
            right.y = y;
            neighbourList.push_back(right);
        }
        if((neighbours & 4) && inBounds(x, y+1)) {
            Tile down;
            down.x = x;
            down.y = y+1;
            neighbourList.push_back(down);
        }
        if((neighbours & 1) && inBounds(x, y-1)) {
            Tile up;
            up.x = x;
            up.y = y-1;
            neighbourList.push_back(up);
        }
        return neighbourList;
    }


    void resize(int newWidth, int newHeight) {
        newWidth = min(max(newWidth, MIN_W), MAX_W);
        newHeight = min(max(newHeight, MIN_H), MAX_H);

        vector<Tile> newCells(static_cast<size_t>(newWidth * newHeight));
        const int copyW = min(width_, newWidth);
        const int copyH = min(height_, newHeight);

        if (width_ > 0 && height_ > 0) {
            for (int y = 0; y < copyH; ++y) {
                for (int x = 0; x < copyW; ++x) {
                    newCells[static_cast<size_t>(y * newWidth + x)] =
                        cells_[static_cast<size_t>(y * width_ + x)];
                }
            }
        }

        width_ = newWidth;
        height_ = newHeight;
        cells_ = move(newCells);
        recomputeConnections();
    }

    void clear() {
        for (Tile& cell : cells_) {
            cell.on = false;
            cell.neighbours = 0;
        }
    }

    bool save(const string& path) const {
        ofstream out(path);
        if (!out) {
            cout << "Could not open file for writing." << endl;
            return false;
        }

        out << "GRIDMAP 1\n";
        out << width_ << ' ' << height_ << '\n';
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                out << (isOn(x, y) ? '1' : '0');
            }
            out << '\n';
        }

        if (!out.good()) {
            cout << "Error while writing file." << endl;
            return false;
        }

        return true;
    }

    bool load(const string& path) {
        ifstream in(path);
        if (!in) {
            cout << "Could not open file." << endl;
            return false;
        }

        string header;
        if (!(in >> header) || header != "GRIDMAP") {
            cout << "Invalid map file header." << endl;
            return false;
        }

        int version = 0;
        int fileWidth = 0;
        int fileHeight = 0;
        if (!(in >> version) || version != 1 || !(in >> fileWidth >> fileHeight)) {
            cout << "Unsupported or malformed map file." << endl;
            return false;
        }

        if (fileWidth < MIN_W || fileWidth > MAX_W ||
            fileHeight < MIN_H || fileHeight > MAX_H) {
            cout << "Map dimensions are outside the supported range." << endl;
            return false;
        }

        vector<Tile> loaded(static_cast<size_t>(fileWidth * fileHeight));
        for (int y = 0; y < fileHeight; ++y) {
            string row;
            if (!(in >> row) || static_cast<int>(row.size()) != fileWidth) {
                cout << "Malformed map rows." << endl;
                return false;
            }

            for (int x = 0; x < fileWidth; ++x) {
                const char value = row[static_cast<size_t>(x)];
                if (value != '0' && value != '1') {
                    cout << "Map contains an invalid tile value." << endl;
                    return false;
                }
                loaded[static_cast<size_t>(y * fileWidth + x)].on = (value == '1');
                loaded[static_cast<size_t>(y * fileWidth + x)].x = x;
                loaded[static_cast<size_t>(y * fileWidth + x)].y = y;
            }
        }

        width_ = fileWidth;
        height_ = fileHeight;
        cells_ = move(loaded);
        recomputeConnections();
        return true;
    }

private:
    int width_ = 0;
    int height_ = 0;
    vector<Tile> cells_;

    size_t index(int x, int y) const {
        return static_cast<size_t>(y * width_ + x);
    }

    void recomputeConnections() {
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                Tile& cell = cells_[index(x, y)];
                cell.neighbours = 0;

                if (!cell.on) continue;

                if (isOn(x, y - 1)) cell.neighbours |= N_UP;
                if (isOn(x + 1, y)) cell.neighbours |= N_RIGHT;
                if (isOn(x, y + 1)) cell.neighbours |= N_DOWN;
                if (isOn(x - 1, y)) cell.neighbours |= N_LEFT;
            }
        }
    }
};

struct Weapon {
    float hitChance, critChance; 
    int minDmg, maxDmg, baseDmg, critDmg;

    Weapon () {
        hitChance = 1;
        minDmg = 190;
        maxDmg = 340;
        baseDmg = 265;
        critDmg = 401;
        critChance = 0.3;
    }

    bool setStats() {
        int index = 0;
        int value;
        string input;
        char letter;
        bool minSkipped = false;
        while (index < 6) {
            switch (index) {
                case 0: 
                    cout << "Hit Chance (in %): "; break;
                case 1:
                    cout << "Min Damage: "; break;
                case 2:
                    cout << "Max Damage: "; break;
                case 3:
                    cout << "Base Damage: "; break;
                case 4:
                    cout << "Crit Damage: "; break;
                case 5:
                    cout << "Crit Chance (in %): "; break;
            }
            getline(cin >> ws, input);
            letter = input[0];
            letter = tolower(letter);

            if (letter == 'x') {
                if (index <= 1) {minSkipped = true;}
                if ((index <= 2) && !minSkipped) {maxDmg = minDmg + 10;}
                if ((index <= 3) && !minSkipped) {baseDmg = (minDmg + maxDmg)/2;}
                return false;}
            if (letter  == 'b') {if (index > 0) {index--;} continue;}
            if (letter == 'n') {
                if (index == 1) {minSkipped = true;}
                if ((index == 2) && !minSkipped) {maxDmg = minDmg + 10;}
                if ((index == 3) && !minSkipped) {baseDmg = (minDmg + maxDmg)/2;}
                index++; 
                continue;
            }

            try {
                size_t pos;
                value = stof(input, &pos);
                
                if (pos != input.size()) {cout << "Invalid input" << endl; continue;}
                switch (index) {
                    case 0: 
                        hitChance = value/(100.0); break;
                    case 1:
                        minDmg = value; break;
                    case 2:
                        maxDmg = value; break;
                    case 3:
                        baseDmg = value; break;
                    case 4:
                        critDmg = value; break;
                    case 5:
                        critChance = value/(100.0); break;

                }
                index++;
            }
            catch (...) {cout << "Invalid input" << endl; continue;}
            }
            return true;
        }
    
    void printStats () {
        cout << "Min Damage: " << minDmg << endl;
        cout << "Max Damage: " << maxDmg << endl;
        cout << "Base Damage: " << baseDmg << endl;
        cout << "Crit Damage: " << critDmg << endl;
        cout << "Crit Chance: " << critChance << endl;
        cout << "Hit Chance: " << hitChance << endl;
    }
};

void SaveVector(const string& filename, const vector<uint32_t>& data) {
    vector<uint32_t> states = {};
    for(uint32_t a : data) {
        states.push_back((a < 2147483648) ? a : a-2147483648);
    }
    sort(states.begin(), states.end());
    ofstream file(filename, ios::binary);
    if (!file)
        throw runtime_error("Failed to open file for writing.");

    uint64_t size = states.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    if (size > 0)
    {
        file.write(reinterpret_cast<const char*>(states.data()),
                   size * sizeof(uint32_t));
    }

    if (!file)
        throw runtime_error("Failed while writing file.");
}
void SaveVectorUnsorted(const string& filename, const vector<uint32_t>& data) {
    vector<uint32_t> states = {};
    for(uint32_t a : data) {
        states.push_back((a < 2147483648) ? a : a-2147483648);
    }
    ofstream file(filename, ios::binary);
    if (!file)
        throw runtime_error("Failed to open file for writing.");

    uint64_t size = states.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    if (size > 0)
    {
        file.write(reinterpret_cast<const char*>(states.data()),
                   size * sizeof(uint32_t));
    }

    if (!file)
        throw runtime_error("Failed while writing file.");
}
vector<uint32_t> LoadVector(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file)
        throw runtime_error("Failed to open file for reading.");

    uint64_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));

    if (!file)
        throw runtime_error("Failed to read vector size.");

    vector<uint32_t> data(size);

    if (size > 0)
    {
        file.read(reinterpret_cast<char*>(data.data()),
                  size * sizeof(uint32_t));

        if (!file)
            throw runtime_error("Failed to read vector data.");
    }

    return data;
}

tuple<vector<uint32_t>, vector<uint32_t>> MergeStatePairsSorted (const vector<uint32_t>& start1, const vector<uint32_t>& end1, const vector<uint32_t>& start2, const vector<uint32_t>& end2) {
    if (start1.size() != end1.size() || start2.size() != end2.size()) {
        throw runtime_error("State pairs do not share size");
    }

    vector<uint32_t> startOut, endOut;
    uint32_t index = 0, size1 = end1.size(), size2 = end2.size();
    
    for(uint32_t i = 0; i < size2; i++) {
        while(((start2[i] & 0x7fffffff) != (end1[index] & 0x7fffffff)) && (index < size1)) {
            index++;
        }
        if(index < size1) {
            startOut.push_back(start1[index]);
            endOut.push_back(end2[i]);
        }
    }
    return {startOut, endOut};
}

tuple<vector<uint32_t>, vector<uint32_t>> MergeStatePairs (const vector<uint32_t>& start1, const vector<uint32_t>& end1, const vector<uint32_t>& start2, const vector<uint32_t>& end2) {
    if (start1.size() != end1.size() || start2.size() != end2.size()) {
        throw runtime_error("State pairs do not share size");
    }

    vector<uint32_t> startOut, endOut;
    uint32_t index, size1 = end1.size(), size2 = end2.size();
    
    for(uint32_t i = 0; i < size1; i++) {
        index = find(start2.begin(), start2.end(), end1[i]) - start2.begin();
        if(index < size2) {
            startOut.push_back(start1[i]);
            endOut.push_back(end2[index]);
        }
    }
    return {startOut, endOut};
}

// rngState = *(int *)(DAT_710321c5b0 + 0x364) * 214013 + 2531011;
// rngValue = (float)(rngState >> 8 & 0x7fff00 | 0x3f800000)
uint32_t lcg(uint32_t seed) {return seed * 214013 + 2531011;}

uint32_t reverseLcg(uint32_t seed) {return (seed - 2531011) * 3115528533;}

uint32_t getLcgAdd(uint32_t steps) {    // steps assumed to be positive for burn performance
    uint32_t mult = 1, add = 0, a = 214013, b = 2531011, n = steps;
    while (n) {
        if (n & 1) {add = a * add + b;}
        b = a*b + b;
        a = a*a;
        n = n >> 1;
    }
    return add;
}

Affine getLcgConsts(int steps) {
    uint32_t mult = 1, add = 0, a, b, n;
    if(steps >= 0) { // (...(((x * 214013) + 2531011) * 214013 + ...) = x*214013^s + 2531011*sum_i(214013^(s-1-i)) 
        a = 214013;
        b = 2531011;
        n = steps;
    } else { // (...(((x - 2531011) * 3115528533) - 2531011) * ...) = x*311552833^s - 2531011*311552833*sum_i(3115528533^(s-1-i))
        a = 3115528533;
        b = -2531011 * 3115528533;
        n = -steps;
    }

    while (n) {
        if (n & 1) {
            add = a * add + b;
            mult = a * mult;
        }
        b = a*b + b;
        a = a*a;
        n = n >> 1;
    }
    Affine out;
    out.add = add;
    out.mult = mult;
    return out;
}

uint32_t lcgWrapper(uint32_t seed, int steps) {
    Affine affine = getLcgConsts(steps);
    return (seed * affine.mult) + affine.add;
}

int getLcgSteps(uint32_t start, uint32_t end) {
    int i = 0;
    while(start != end) {
        start = lcg(start);
        i++;
    }
    return i;
}

float stateToValue(uint32_t state) {
    uint32_t hexValue = (float) (state >> 8 & 0x7fff00 | 0x3f800000);
    float f;
    memcpy(&f, &hexValue, sizeof(f));
    return f;
    // 0x3f800000 = 0011 1111 1000 0000 0000 0000 0000 0000
    // 0x007fff00 = 0000 0000 0111 1111 1111 1111 0000 0000
    //     (xxxx x)xxx          0[000 0000 0000 0000] 0000 0000 0000 0000
    // 3f 8(xxx] 00 = 0011 1111 1[000 0000 0000 0000] 0000 0000 \0000 0000
}

tuple<uint32_t,uint32_t,uint32_t,uint32_t> valueToState(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));

    uint32_t mantissa = bits & 0x007fffff;

    uint32_t state = (mantissa & 0x007fff00) << 8;
    uint32_t stateMax = state | 0xFFFF;

    uint32_t altState = state | (1u << 31);
    uint32_t altStateMax = stateMax | (1u << 31);

    return {state, stateMax, altState, altStateMax};
}

tuple<uint32_t, uint32_t> valueToMantissa(tuple<float, float> vals) {
    float minVal, maxVal;
    uint32_t bits, minMantissa, maxMantissa;

    tie(minVal, maxVal) = vals;
    if ((minVal == 1 && maxVal == 2) || (maxVal <= minVal) || (maxVal == 0)) {minMantissa = 1; maxMantissa = 1;} 
    else {
        memcpy(&bits, &minVal, sizeof(bits));
        minMantissa = bits & 0x7fff00;

        if(maxVal >= 1.999969482421875) {
            maxMantissa = 0x7fff00;
        } 
        else {
            memcpy(&bits, &maxVal, sizeof(bits));
            maxMantissa = bits & 0x7fff00;
        }
    }
    return make_tuple(minMantissa, maxMantissa);
}

int getTilesetIndex(vector<Tile>& tileset, int x, int y) {
    for(int j = 0; j < tileset.size(); j++) {
        if((x == tileset[j].x) && (y == tileset[j].y)) {
            return j;
        }
    }
    return -1;
}

Map generateCandidateTiles(Map map, vector<Tile>& candidateTiles) {
    Map out(map.width(), map.height());
    int i = 0, dx, dy;
    vector<Tile> neighbours;
    Tile start = candidateTiles[0];
    out.set(start.x, start.y, true);
    while (i < candidateTiles.size())
    {
        neighbours = map.getNeighbours(candidateTiles[i].x, candidateTiles[i].y);
        for(int k = 0; k < neighbours.size(); k++) {
            Tile neighbour = neighbours[k];
            dx = neighbour.x - start.x;
            dy = neighbour.y - start.y;
            if(dx*dx + dy*dy > (myMaxRadius/2)*(myMaxRadius/2)) { // check in range
                continue;
            }
            if(getTilesetIndex(candidateTiles, neighbour.x, neighbour.y) != -1) { // check if already in set
                continue;
            }
            candidateTiles.push_back(neighbour);
            out.set(neighbour.x, neighbour.y, true);
        }
        i++;
    }
    return out;
}

void MapToBurnLCGConsts(vector<Tile> candidateTiles, Map candidateMap, Map valMap, Map usedMap, array<Affine, 32768>& transition3, array<Affine, 32768>& transition4) {
    int tileStateSize, tempX, tempY, area3Steps, area4Steps, x = 0, y = 0, arrInd = 0;
    int size = candidateTiles.size() - 1;
    float f = 0;
    uint32_t mantissa, hexValueMin, hexValueMax = 0;
    Tile corner;

    for(int ind = 0; ind < size; ind++) {
        corner = candidateTiles[ind];

        if(usedMap.isOn(corner.x, corner.y)) {     // account for already used tiles
            area3Steps = 0;
            area4Steps = 0;
            goto area_failed;
        }

        hexValueMin = hexValueMax;
        if(ind == size-1) {
            hexValueMax = 32768;
        } else {
            while(int(f) != ind+1) {
                hexValueMax++;  
                hexValueMax = hexValueMax;
                mantissa = (0x3f800000 | (hexValueMax << 8));
                memcpy(&f, &mantissa, sizeof(f));
                f = f*size - size;
            }
        }

        // count when area check fails
        x = 0;
        while (x < (myAreaMaxSize-1)) {
            y = 0;
            while (y < (myAreaMaxSize-1)) {
                tempX = corner.x + x;
                tempY = corner.y - y;
                if(!candidateMap.isOn(tempX, tempY)) { // use byte array instead
                    area4Steps = x*(myAreaMaxSize-1) + y;
                    if((y < myAreaMinSize) && (x < myAreaMinSize)) {
                        area3Steps = x*myAreaMinSize + y;
                        goto area_failed;
                    } else {
                        if (y >= myAreaMinSize) {
                            x++;
                        }
                        goto restOf3Size;
                    }

                }
                // lcg step taken here
                if(!valMap.isOn(tempX, tempY)) { // use byte array instead
                    area4Steps = x*(myAreaMaxSize-1) + y + 1;
                    if((y < myAreaMinSize) && (x < myAreaMinSize)) {
                        area3Steps = x*myAreaMinSize + y + 1;
                        goto area_failed;
                    } else {
                        if (y >= myAreaMinSize) {
                            x++;
                        }
                        goto restOf3Size;
                    }

                }
                y++;
            }
            x++;
        }
        area3Steps = 9;
        area4Steps = 16; // add number to list
        goto area_failed;

        restOf3Size:
        while (x < myAreaMinSize) {
            y = 0;
            while (y < myAreaMinSize) {
                tempX = corner.x + x;
                tempY = corner.y - y;
                if(!candidateMap.isOn(tempX, tempY)) {
                    area3Steps = x*myAreaMinSize + y;
                    goto area_failed;
                }
                // lcg step taken here;
                if(!valMap.isOn(tempX, tempY)) {
                    area3Steps = x*myAreaMinSize + y + 1;
                    goto area_failed;
                }
                y++;
            }
            x++;
        }
        area3Steps = 9;

        area_failed:

	    fill(transition3.begin() + hexValueMin, transition3.begin() + hexValueMax, getLcgConsts(area3Steps + 1));
	    fill(transition4.begin() + hexValueMin, transition4.begin() + hexValueMax, getLcgConsts(area4Steps + 1));
    }
}

void MapToBurnLCGConstsX(vector<Tile> candidateTiles, Map candidateMap, Map valMap, Tile startingPos, array<Affine, 32768>& transition3, array<Affine, 32768>& transition4) {
    int tileStateSize, tempX, tempY, area3Steps, area4Steps, x = 0, y = 0, arrInd = 0;
    float dx, dy, d2;
    int size = candidateTiles.size() - 1;
    float f = 0;
    uint32_t mantissa, hexValueMin, hexValueMax = 0;

    Tile corner;
    Affine zero;
    zero.add = 0;
    bool succ3, succ4;

    for(int ind = 0; ind < size; ind++) {
        succ3 = false;
        succ4 = false;
        corner = candidateTiles[ind];

        hexValueMin = hexValueMax;

        if(ind == size-1) {
            hexValueMax = 32768;
        } else {
            while(int(f) != ind+1) {
                hexValueMax++;  
                hexValueMax = hexValueMax;
                mantissa = (0x3f800000 | (hexValueMax << 8));
                memcpy(&f, &mantissa, sizeof(f));
                f = f*size - size;
            }
        }

        if(hexValueMax == 0 || hexValueMax > 32768) {
            hexValueMax = 32768;
        }


        // count when area check fails
        x = 0;
        while (x < (myAreaMaxSize-1)) {
            y = 0;
            while (y < (myAreaMaxSize-1)) {
                tempX = corner.x + x;
                tempY = corner.y - y;
                if(!candidateMap.isOn(tempX, tempY)) { // use byte array instead
                    area4Steps = x*(myAreaMaxSize-1) + y;
                    if((y < myAreaMinSize) && (x < myAreaMinSize)) {
                        area3Steps = x*myAreaMinSize + y;
                        goto area_failed;
                    } else {
                        if (y >= myAreaMinSize) {
                            x++;
                        }
                        goto restOf3Size;
                    }

                }

                // lcg step taken here

                if(!valMap.isOn(tempX, tempY)) { // use byte array instead
                    area4Steps = x*(myAreaMaxSize-1) + y + 1;
                    if((y < myAreaMinSize) && (x < myAreaMinSize)) {
                        area3Steps = x*myAreaMinSize + y + 1;
                        goto area_failed;
                    } else {
                        if (y >= myAreaMinSize) {
                            x++;
                        }
                        goto restOf3Size;
                    }

                }
                y++;
            }
            x++;
        }

        area3Steps = 9;
        dx = corner.x + 1 - startingPos.x;
        dy = corner.y - 1 - startingPos.y;
        d2 = dx*dx + dy*dy;
        succ3 = (myMinDistanceBetweenPoints*myMinDistanceBetweenPoints/4 < d2) && (d2 < myMaxDistanceBetweenPoints*myMaxDistanceBetweenPoints/4);

        area4Steps = 16;
        dx = corner.x + 1.5 - startingPos.x;
        dy = corner.y - 1.5 - startingPos.y;
        d2 = dx*dx + dy*dy;
        succ4 = (myMinDistanceBetweenPoints*myMinDistanceBetweenPoints/4 < d2) && (d2 < myMaxDistanceBetweenPoints*myMaxDistanceBetweenPoints/4);
        goto area_failed;


        restOf3Size:
        while (x < myAreaMinSize) {   // x,y in correct order?
            y = 0;
            while (y < myAreaMinSize) {
                tempX = corner.x + x;
                tempY = corner.y - y;
                if(!candidateMap.isOn(tempX, tempY)) {
                    area3Steps = x*myAreaMinSize + y;
                    goto area_failed;
                }
                // lcg step taken here;
                if(!valMap.isOn(tempX, tempY)) {
                    area3Steps = x*myAreaMinSize + y + 1;
                    goto area_failed;
                }
                y++;
            }
            x++;
        }

        area3Steps = 9;
        dx = corner.x + 1 - startingPos.x;
        dy = corner.y - 1 - startingPos.y;
        d2 = dx*dx + dy*dy;
        succ3 = (myMinDistanceBetweenPoints*myMinDistanceBetweenPoints/4 < d2) && (d2 < myMaxDistanceBetweenPoints*myMaxDistanceBetweenPoints/4);


        area_failed:
        if(succ3) {
            fill(transition3.begin() + hexValueMin, transition3.begin() + hexValueMax, zero);
        } else {
	        fill(transition3.begin() + hexValueMin, transition3.begin() + hexValueMax, getLcgConsts(area3Steps + 1));
        }

        if (succ4) {
            fill(transition4.begin() + hexValueMin, transition4.begin() + hexValueMax, zero);
        } else {
	        fill(transition4.begin() + hexValueMin, transition4.begin() + hexValueMax, getLcgConsts(area4Steps + 1));
        }
    }
}


int validateArea(vector<Tile>& candidateTiles, uint32_t rngState, Tile bottomleft, int randomAreaSize, vector<int> invIndex) {
    int tempX, tempY, tempIndex;
    sort(invIndex.begin(), invIndex.end());
    for (int i = 0; i < randomAreaSize; i++) {
        for (int j = 0; j < randomAreaSize; j++) {
            tempX = bottomleft.x + j;
            tempY = bottomleft.y + i;
            tempIndex = getTilesetIndex(candidateTiles, tempX, tempY);
            if(tempIndex == -1) {
                return 0;
            }
            rngState = lcg(rngState);
            if(!(myEnemiesAvoidanceProbability <= stateToValue(rngState)) || !(myAlliesAvoidanceProbability <= stateToValue(rngState))) {
                return 0; // check enemy/ally avoidance, but it is 0 anyway
            }
            if (!binary_search(invIndex.begin(), invIndex.end(), tempIndex)) { // check idk
                return 0;
            }
            if (false) { //check idk
                return 0;
            }
        }
    }
    return 1; // return the Tile index?/pos? of center not 1
}

int generateCandidateArea (vector<Tile>& outWaypoints, vector<int> invIndex, vector<Tile>& candidateTiles, uint32_t& rngState, int randomAreaSize) {
    if (myMaxConsecutiveSkips != 0) {
        int d2, dx, dy, center, consecutiveSkips = 0;
        int size = candidateTiles.size()-1;
        Tile randomTile;
        while (consecutiveSkips < myMaxConsecutiveSkips) {
            rngState = lcg(rngState);
            randomTile = candidateTiles[stateToValue(rngState)*size - size];
            if(getTilesetIndex(outWaypoints, randomTile.x, randomTile.y) != -1) {
                consecutiveSkips += 1;
                continue;
            }
            center = validateArea(candidateTiles, rngState, randomTile, randomAreaSize, invIndex);
            if(center) {
                dx = candidateTiles[center].x - outWaypoints[-1].x;
                dy = candidateTiles[center].y - outWaypoints[-1].y;
                d2 = dx*dx + dy*dy;
                if((myMinDistanceBetweenPoints*myMinDistanceBetweenPoints < d2) && (d2 < myMaxDistanceBetweenPoints*myMaxDistanceBetweenPoints)) {
                    outWaypoints.push_back(randomTile);
                    return 1;
                }
            }
            consecutiveSkips += 1;
        }
    }
    return 0;
}

int generateWaypoints (vector<Tile>& outWaypoints, vector<int> invIndex, vector<Tile>& candidateTiles, uint32_t& rngState) {
    if((myNumOfAreas >= 1) && (myMaxConsecutiveSkips >= 0)) {
        int consecutiveSkips = 0, numOfAreas = 0;
        int randomAreaSize;
        int dAreaSize = myAreaMaxSize - myAreaMinSize;
        while ((numOfAreas < myNumOfAreas) && (consecutiveSkips <= myMaxConsecutiveSkips)) {
            rngState = lcg(rngState);
            randomAreaSize = int((stateToValue(rngState)*dAreaSize - dAreaSize)) + myAreaMinSize;
            if (myDoNotEndInCover && (numOfAreas == (myNumOfAreas - 1))) {
                randomAreaSize = 3;
            }
            if (generateCandidateArea(outWaypoints, invIndex, candidateTiles, rngState, randomAreaSize) == 0) { // check if valid area generated
                consecutiveSkips += 1;
            }
            else {
                consecutiveSkips = 0;
                numOfAreas += 1;
            }
        }
    }
    return 0;
}

int generateFallbackWaypoints(vector<Tile>& outWaypoints, vector<int> invIndex, vector<Tile>& candidateTiles, uint32_t& rngState) {
    float rngVal;
    int numCandidates = candidateTiles.size() - 1;
    int tileIndex;
    int numOfPoints = 0;
    sort(invIndex.begin(), invIndex.end()); 

    if((myNumOfPointsForCompletelyRandomPath > 0) && (myMaxConsecutiveSkips > 0)) {
        while (numOfPoints < myNumOfPointsForCompletelyRandomPath)
        {
            int consecutiveSkips = 0;
            rngState = lcg(rngState);
            rngVal = stateToValue(rngState);
            tileIndex = int(rngVal*numCandidates - numCandidates);

            if(!binary_search(invIndex.begin(), invIndex.end(), tileIndex)) { // check for valid selection
                outWaypoints.push_back(candidateTiles[tileIndex]);
                consecutiveSkips = 0;
                numOfPoints += 1;
                continue;
            }

            consecutiveSkips += 1;
            if(consecutiveSkips >= myMaxConsecutiveSkips) {return consecutiveSkips;}
        }
    }

    return 0;
}

int accurateBurnSim(Map map, vector<int> invIndex, vector<Tile>& out, uint32_t rngState, int target_x, int target_y) {
    Tile target;
    target.x = target_x;
    target.y = target_x;
    vector<Tile> candidateTiles = {target};
    generateCandidateTiles(map, candidateTiles);

    if(generateWaypoints(out, invIndex, candidateTiles, rngState)) {
        return 1;
    } else {
        generateFallbackWaypoints(out, invIndex, candidateTiles, rngState);
    }

    return 0;
}

void printMapX(Map map, vector<Tile>& candidateTiles, array<unsigned char, 32768>& transition3) {
    int temp;
    if (candidateTiles.size() == 0) {
        for (int y = 0; y < map.height(); ++y) {
            for (int x = 0; x < map.width(); ++x) {
                cout << (map.isOn(x, y) ? '1' : '0');
            }
            cout << '\n';
        }
    } else {
        for (int y = 0; y < map.height(); ++y) {
            for (int x = 0; x < map.width(); ++x) {
                if(map.isOn(x, y)) {
                    temp = getTilesetIndex(candidateTiles, x, y);
                    if(temp == -1) {
                        cout << setw(4) << "o";
                    } else {
                        cout << setw(4) << transition3[int(temp/(candidateTiles.size()-1.0) * 32768) + 1] - 1;
                    }
                } else {
                    cout << setw(4) << '-';
                }
            }
            cout << '\n';
        }
    }
}

tuple<vector<uint32_t>, vector<uint32_t>> fastBurnSimFirstAreaEmpty (Tile startingPos, Map goal3, Map goal4, vector<Tile>& candidateTiles, Map candidateMap, Map valMap) {
    vector<uint32_t> seeds = {};
    vector<uint32_t> tempStates = {};
    
    int d2, dx, dy, dAreaSize = myAreaMaxSize - myAreaMinSize;
    int size = candidateTiles.size()-1;
    float rngVal;
    Tile randomTile;
    Affine affine;

    uint32_t tempState, stateInd;

    array<Affine, 32768> transition3;
    array<Affine, 32768> transition4;
    MapToBurnLCGConstsX(candidateTiles, candidateMap, valMap, startingPos, transition3, transition4);

    uint32_t i = 0;
    while(i < u31) {
        tempState = i * 214013 + 2531011;   //area roll start
        for(int consecutiveSkipsA = 0; consecutiveSkipsA <= myMaxConsecutiveSkips; consecutiveSkipsA++) {
            if((tempState & 0x7fffffff) < 1073741824) {
                tempState = tempState * 214013 + 2531011;   //tile roll start
                for (int consecutiveSkips = 0; consecutiveSkips < myMaxConsecutiveSkips; consecutiveSkips++) {
                    stateInd = (tempState >> 16) & 0x7fff;
                    affine = transition3[stateInd];     // avoidance + 1 (tile or area)
                    if (affine.add == 0) {
                        randomTile = candidateTiles[stateToValue(tempState)*size - size];
                        tempState = tempState * 1744563881 + 2006221698; // 10 steps
                        if(goal3.isOn(randomTile.x, randomTile.y)) {
                            seeds.push_back(i);
                            tempStates.push_back((tempState - 2531011) * 3115528533);   // reverseLcg
                        }
                        goto area_done;
                    } else { 
                        tempState = tempState * affine.mult + affine.add;
                    }
                }
            } else {
                tempState = tempState * 214013 + 2531011;
                for (int consecutiveSkips = 0; consecutiveSkips < myMaxConsecutiveSkips; consecutiveSkips++) {
                    stateInd = (tempState >> 16) & 0x7fff;
                    affine = transition4[stateInd];
                    if (affine.add == 0) {
                        randomTile = candidateTiles[stateToValue(tempState)*size - size];
                        tempState = tempState * -389939651 + 586225427; // 17 steps
                        if(goal4.isOn(randomTile.x, randomTile.y)) {
                            // reverse lcg for tempState?
                            seeds.push_back(i);
                            tempStates.push_back(tempState);
                        }
                        goto area_done;
                    } else { 
                        tempState = tempState * affine.mult + affine.add;
                    }
                }
            }
        }

        area_done:
        i++;
    }
    return {seeds, tempStates};
}

tuple<vector<uint32_t>, vector<uint32_t>> fastBurnSimFirstArea (vector<uint32_t> states, Tile startingPos, Map goal3, Map goal4, vector<Tile>& candidateTiles, Map candidateMap, Map valMap) {
    vector<uint32_t> seeds = {};
    vector<uint32_t> tempStates = {};
    
    int d2, dx, dy, dAreaSize = myAreaMaxSize - myAreaMinSize;
    int size = candidateTiles.size()-1;
    float rngVal;
    Tile randomTile;
    Affine affine;

    uint32_t tempState, stateInd;

    array<Affine, 32768> transition3;
    array<Affine, 32768> transition4;
    MapToBurnLCGConstsX(candidateTiles, candidateMap, valMap, startingPos, transition3, transition4);

    for(uint32_t i : states) {
        tempState = i * 214013 + 2531011;   //area roll start
        for(int consecutiveSkipsA = 0; consecutiveSkipsA <= myMaxConsecutiveSkips; consecutiveSkipsA++) {
            if((tempState & 0x7fffffff) < 1073741824) {
                tempState = tempState * 214013 + 2531011;   //tile roll start
                for (int consecutiveSkips = 0; consecutiveSkips < myMaxConsecutiveSkips; consecutiveSkips++) {
                    stateInd = (tempState >> 16) & 0x7fff;
                    affine = transition3[stateInd];     // avoidance + 1 (for tile or area)
                    if (affine.add == 0) {
                        randomTile = candidateTiles[stateToValue(tempState)*size - size];
                        tempState = tempState * 1744563881 + 2006221698; // 10 steps
                        if(goal3.isOn(randomTile.x, randomTile.y)) {
                            seeds.push_back(i);
                            tempStates.push_back((tempState - 2531011) * 3115528533);   // reverse Lcg (remove +1 for tile/area)
                        }
                        goto area_done;
                    } else { 
                        tempState = tempState * affine.mult + affine.add;
                    }
                }
            } else {
                tempState = tempState * 214013 + 2531011;   //tile roll start
                for (int consecutiveSkips = 0; consecutiveSkips < myMaxConsecutiveSkips; consecutiveSkips++) {
                    stateInd = (tempState >> 16) & 0x7fff;
                    affine = transition4[stateInd];     // avoidance + 1 (for tile or area)
                    if (affine.add == 0) {
                        randomTile = candidateTiles[stateToValue(tempState)*size - size];
                        tempState = tempState * -389939651 + 586225427; // 17 steps
                        if(goal4.isOn(randomTile.x, randomTile.y)) {
                            seeds.push_back(i);
                            tempStates.push_back((tempState - 2531011) * 3115528533);   // reverse Lcg (remove +1 for tile/area)
                        }
                        goto area_done;
                    } else { 
                        tempState = tempState * affine.mult + affine.add;
                    }
                }
            }
        }

        area_done:
        i++;
    }
    return {seeds, tempStates};
}


void MapToBurnRNGStepLUT(vector<Tile> candidateTiles, Map candidateMap, Map valMap, Map usedMap, array<unsigned char, 32768>& transition3) {
    /*
    consecutive skips {
    	lcg()
    	- used tile
    	
        for(^2) {
    	    if tileIndex == -1: continue		//check if on the map. Pipe exits?, Pipe blocks? Height difference
    	    lcg()
    	    - avoidance				            //val Map. Own character included
    	    - ?
    	    - same elevation
        }

    	- distance
    }
    */

    int tempX, tempY, area3Steps, area4Steps, x = 0, y = 0, arrInd = 0;
    int size = candidateTiles.size() - 1;
    float f = 0;
    uint32_t mantissa, hexValueMin, hexValueMax = 0;
    Tile corner;

    for(int ind = 0; ind < size; ind++) {
        corner = candidateTiles[ind];

        hexValueMin = hexValueMax;
        if(ind == size-1) {
            hexValueMax = 32768;
        } else {
            while(int(f) != ind+1) {
                hexValueMax++;  
                hexValueMax = hexValueMax;
                mantissa = (0x3f800000 | (hexValueMax << 8));
                memcpy(&f, &mantissa, sizeof(f));
                f = f*size - size;
            }
        }

        if(usedMap.isOn(corner.x, corner.y)) {     // account for already used tiles
            area3Steps = 0;
            area4Steps = 0;
            goto area_failed;
        }

        // count when area check fails
        x = 0;
        while (x < 3) {
            y = 0;
            while (y < 3) {
                tempX = corner.x + x;
                tempY = corner.y - y;
                if(!candidateMap.isOn(tempX, tempY)) { // use byte array instead
                    area3Steps = x*myAreaMinSize + y;
                    goto area_failed;
                }
                // lcg step taken here
                if(!valMap.isOn(tempX, tempY)) { // use byte array instead
                    area3Steps = x*myAreaMinSize + y + 1;
                    goto area_failed;
                }
                y++;
            }
            x++;
        }
        area3Steps = 9;

        area_failed:
	    fill(transition3.begin() + hexValueMin, transition3.begin() + hexValueMax, area3Steps + 1);
    }
}

tuple<vector<uint32_t>, vector<uint32_t>> fastBurnSimLastArea(vector<uint32_t> states, vector<Tile> candidateTiles, Map candidateMap, Map valMap, Map valMapFB, Map usedMap, tuple<float, float> goalVals) {
    uint32_t tempState, tempStateStart;
    int consecutiveSkipsC, numOfPoints, tempX, tempY;
    float rngVal;
    int size = candidateTiles.size()-1;
    float fsize = size;
    Tile rngTile;

    array<unsigned char, 32768> transition3;
    MapToBurnRNGStepLUT(candidateTiles, candidateMap, valMap, usedMap, transition3);

    bool isEmpty = states.empty();
    uint32_t limit = (isEmpty ? u31 : states.size());
    uint32_t stateInd, trimmedState;

    float minGoalVal, maxGoalVal;
    tie(minGoalVal, maxGoalVal) = goalVals;
    uint32_t minState, maxState;

    memcpy(&minState, &minGoalVal, sizeof(minState));
    minState = ((minState & 0x7fff00) << 8);

    memcpy(&maxState, &maxGoalVal, sizeof(maxState));
    maxState = ((maxState-1 & 0x7fff00) << 8) + 0x10000;


    vector<uint32_t> seeds = {};
    vector<uint32_t> tempStates = {};
    Affine affine;


    constexpr uint64_t N        = 1ull << 31;
    constexpr uint64_t SECTIONS = 2048;
    constexpr uint64_t CHUNK    = N / SECTIONS;
    constexpr uint64_t OVERLAP  = 172000;
    vector<uint32_t> stateToBigStep(CHUNK + OVERLAP);

    uint32_t stateStart = 0;

    for (uint64_t section = 0; section < SECTIONS; ++section) {
        const uint64_t sectionStart = section * CHUNK;
        const uint64_t sectionEnd = min(sectionStart + CHUNK, N);

        uint64_t calcStart;
        uint64_t calcEnd;

        if (section == 0) {
            calcStart = 0;
            calcEnd = min(CHUNK + OVERLAP, N);
        } else {
            // Entries [sectionStart, sectionStart + OVERLAP) are already in the buffer
            calcStart = sectionStart + OVERLAP;
            calcEnd   = min(sectionStart + CHUNK + OVERLAP, N);
            memmove(stateToBigStep.data(), stateToBigStep.data() + CHUNK, OVERLAP * sizeof(uint32_t));
        }

        for (uint64_t i = calcStart; i < calcEnd; ++i) {
            // size always 3
            stateStart = stateStart * 214013u + 2531011u;   //tile roll start
            tempState = stateStart;
            short stepsTotal = 1;
            for (int consecutiveSkipsB = 0; consecutiveSkipsB < myMaxConsecutiveSkips; ++consecutiveSkipsB) {
                uint32_t stateInd = (tempState >> 16) & 0x7fff;
                char step = transition3[stateInd];     // avoidance + 1 (tile or area)
                stepsTotal += step;
                affine = getLcgConsts(step);
                tempState = tempState * affine.mult + affine.add;
            }
            stateToBigStep[i - sectionStart] = stepsTotal;
        }

        // Progress
        for (uint64_t i = sectionStart; i < sectionEnd; ++i) {
            uint32_t lutIndex = i+1;    // area roll start
            for (int consecutiveSkipsA = 0; consecutiveSkipsA <= myMaxConsecutiveSkips; ++consecutiveSkipsA) {
                lutIndex += stateToBigStep[lutIndex - sectionStart];
            }
      
            // Fallback waypoint generation
            tempState = getLcgAdd(lutIndex-1);
            numOfPoints = 0;
            consecutiveSkipsC = 0;
            while(numOfPoints < myNumOfPointsForCompletelyRandomPath)
            {
                tempState = tempState * 214013 + 2531011;
                rngVal = stateToValue(tempState);
                trimmedState = tempState & 0x7fffffff;
                if((minState <= trimmedState) && (trimmedState < maxState)) {
                    seeds.push_back(getLcgAdd(i));
                    tempStates.push_back(tempState);
                    break;
                }

                rngTile = candidateTiles[int(rngVal*size - size)]; // avoid value calc
                // check for valid selection (if valid interrupt, since we want to control the first waypoint)
                if(valMapFB.isOn(rngTile.x, rngTile.y)) { // byte array for valid indices instead
                    break;
                }
                consecutiveSkipsC++;
                if(consecutiveSkipsC >= myMaxConsecutiveSkips) {break;}
            }
        }
    }

    return {seeds, tempStates};
}

tuple<vector<uint32_t>, vector<uint32_t>> fastBurnSimLastAreaXXX(vector<uint32_t> states, vector<Tile> candidateTiles, Map candidateMap, Map valMap, Map valMapFB, Map usedMap, tuple<float, float> goalVals) {
    uint32_t tempState, tempStateStart;
    int consecutiveSkipsC, numOfPoints, tempX, tempY;
    float rngVal;
    int size = candidateTiles.size()-1;
    float fsize = size;
    Tile rngTile;

    array<unsigned char, 32768> transition3;
    MapToBurnRNGStepLUT(candidateTiles, candidateMap, valMap, usedMap, transition3);

    bool isEmpty = states.empty();
    uint32_t limit = (isEmpty ? u31 : states.size());
    uint32_t stateInd, trimmedState;

    float minGoalVal, maxGoalVal;
    tie(minGoalVal, maxGoalVal) = goalVals;
    uint32_t minState, maxState;

    memcpy(&minState, &minGoalVal, sizeof(minState));
    minState = ((minState & 0x7fff00) << 8);

    memcpy(&maxState, &maxGoalVal, sizeof(maxState));
    maxState = ((maxState-1 & 0x7fff00) << 8) + 0x10000;


    vector<uint32_t> seeds = {};
    vector<uint32_t> tempStates = {};
    Affine affine;


    constexpr uint64_t N        = 1ull << 31;
    constexpr uint64_t SECTIONS = 2048;
    constexpr uint64_t CHUNK    = N / SECTIONS;
    constexpr uint64_t OVERLAP  = 172000;
    vector<uint32_t> stateToBigStep(CHUNK + OVERLAP);

    uint32_t stateStart = 0;

    for (uint64_t section = 0; section < SECTIONS; ++section) {
        const uint64_t sectionStart = section * CHUNK;
        const uint64_t sectionEnd = min(sectionStart + CHUNK, N);

        uint64_t calcStart;
        uint64_t calcEnd;

        if (section == 0) {
            calcStart = 0;
            calcEnd = min(CHUNK + OVERLAP, N);
        } else {
            // Entries [sectionStart, sectionStart + OVERLAP) are already in the buffer
            calcStart = sectionStart + OVERLAP;
            calcEnd   = min(sectionStart + CHUNK + OVERLAP, N);
            memmove(stateToBigStep.data(), stateToBigStep.data() + CHUNK, OVERLAP * sizeof(uint32_t));
        }

        for (uint64_t i = calcStart; i < calcEnd; ++i) {
            // size always 3
            stateStart = stateStart * 214013u + 2531011u;   //tile roll start
            tempState = stateStart;
            short stepsTotal = 1;
            for (int consecutiveSkipsB = 0; consecutiveSkipsB < myMaxConsecutiveSkips; ++consecutiveSkipsB) {
                uint32_t stateInd = (tempState >> 16) & 0x7fff;
                char step = transition3[stateInd];     // avoidance + 1 (tile or area)
                stepsTotal += step;
                affine = getLcgConsts(step);
                tempState = tempState * affine.mult + affine.add;
            }
            stateToBigStep[i - sectionStart] = stepsTotal;
        }

        // Progress
        for (uint64_t i = sectionStart; i < sectionEnd; ++i) {
            uint32_t lutIndex = i+1;    // area roll start
            for (int consecutiveSkipsA = 0; consecutiveSkipsA <= myMaxConsecutiveSkips; ++consecutiveSkipsA) {
                lutIndex += stateToBigStep[lutIndex - sectionStart];
            }
      
            // Fallback waypoint generation
            tempState = getLcgAdd(lutIndex-1);
            numOfPoints = 0;
            consecutiveSkipsC = 0;
            while(numOfPoints < myNumOfPointsForCompletelyRandomPath)
            {
                tempState = tempState * 214013 + 2531011;
                rngVal = stateToValue(tempState);
                trimmedState = tempState & 0x7fffffff;
                if((minState <= trimmedState) && (trimmedState < maxState)) {
                    seeds.push_back(getLcgAdd(i));
                    tempStates.push_back(tempState);
                    break;
                }

                rngTile = candidateTiles[int(rngVal*size - size)]; // avoid value calc
                // check for valid selection (if valid interrupt, since we want to control the first waypoint)
                if(valMapFB.isOn(rngTile.x, rngTile.y)) { // byte array for valid indices instead
                    break;
                }
                consecutiveSkipsC++;
                if(consecutiveSkipsC >= myMaxConsecutiveSkips) {break;}
            }
        }
    }

    return {seeds, tempStates};
}



int damageCalc(int baseDmg, float highGround = 0, float enemyTypeBonus = 0, float MPower = 0, float weaken = 0, float reactMult = 0, float distanceFallOff = 1, float shield = 0, float protect = 0) {
    int damage = baseDmg;
    damage = int((1 + highGround) * damage + 0.5);
    damage = int((1 + enemyTypeBonus) * damage + 0.5);
    damage = int((1 + MPower - weaken) * damage + 0.5);
    damage = int((1 + reactMult) * damage + 0.5);
    damage = int(distanceFallOff * damage + 0.5);
    damage = int((1 - shield) * damage + 0.5);
    damage = int((1 - protect) * damage + 0.5);
    return damage;
}
tuple<int, int> reverseDamageCalc(int damage, float highGround = 0, float enemyTypeBonus = 0, float MPower = 0, float weaken = 0, float reactMult = 0, float distanceFallOff = 1, float shield = 0, float protect = 0) {
    int baseDmgApprox = int((damage / (1 - protect) / (1 + shield) / distanceFallOff / (1 + reactMult) / (1 + MPower - weaken) / (1 + enemyTypeBonus) / (1 + highGround)) + 0.5);
    int lowDmg = baseDmgApprox;
    int highDmg = baseDmgApprox;
    while(damageCalc(lowDmg-1) == damage) {
        lowDmg -= 1;
    }
    while(damageCalc(highDmg+1) == damage) {
        highDmg += 1;
    }
    return make_tuple(lowDmg, highDmg);
}

tuple<float, float> hitCritToValue(int res, float critChance, float hitChance = 1.0) {
    if (res == 0) { // no hit
        return make_tuple(1.0, 2.0 - hitChance);
    }
    if (res == 1) { // hit, no crit
        return make_tuple(2.0 - hitChance, 2.0 - hitChance*critChance);
    }
    // hit, crit
    return make_tuple(2.0 - hitChance*critChance, 2.0);
}
int valueToHitCrit(float value, float critChance = 0, float hitChance = 1) {
    if (value < 2.0 - hitChance) { // no hit
        return 0;
    }
    if (value < 2.0 - hitChance*critChance) { // hit, no crit
        return 1;
    }
    // hit, crit
    return 2;
}

tuple<float, float> dmgToValue(int damage, int minDmg, int maxDmg = -1, int baseDmg = -1) {
    int dmgRange;
    if (maxDmg == -1) {
        dmgRange = 5;
    } else {
        dmgRange = (maxDmg - minDmg)/2;
    }
    if(baseDmg == -1) {
        baseDmg = minDmg + dmgRange;
    }
    float temp = damage - baseDmg;
    return make_tuple(max(1.0, ((temp - 0.5) + 3*dmgRange) / (2*dmgRange)), min(2.0, ((temp + 0.5) + 3*dmgRange) / (2*dmgRange)));
}
int valueToDmg(float value, int minDmg, int maxDmg = -1, int baseDmg = -1) {
    int dmgRange;
    if (maxDmg == -1) {
        dmgRange = 5;
    } else {
        dmgRange = (maxDmg - minDmg)/2;
    }
    if(baseDmg == -1) {
        baseDmg = minDmg + dmgRange;
    }
        // weird workaround
    value = max(float(1), min(float(2), value));
    value = value * 2*dmgRange - 3*dmgRange;
    float rounding = 0.5;
    if (value < 0) {
        rounding = -0.5;
    }
    int damage = static_cast<int>(value + rounding);
    damage = damage + baseDmg;
    return damage;
}

tuple<int, int> valueToBounceAngle(float value1, float value2) {
    float dx = value1 - 0.99;
    float dy = value2 - 0.99;
    float dz = 0;
    float length = dx*dx + dz*dz + dy*dy;
    length = 1 / sqrt(length);
    dx *= length;
    dy *= length;
    dz *= length;
    dx = int((5 * dx) + 0.5);
    dy = int((5 * dy) + 0.5);
    return make_tuple(dx, dy);
}

int valueToVSCoinFlip(float value) {
    int res = int(value*2 - 2);
    return res; //0 = Player 1; 1 = Player 2
}
tuple<float, float> VSCoinFlipToValue(int coinFlip) {
    if(coinFlip) {return make_tuple(1, 1.499969482421875);} //Player 1
    return make_tuple(1.5, 2); //Player 2
}

tuple<int, int, int> valueToCoinSpawn(int numValidTiles, float value1, float value2, float value3) {
    int LandingTile = (value1 * numValidTiles) - numValidTiles;
    int dirInLTile = (value2 * 6.2831855) - 6.2831855;
    int centerdistInLTile = (value3 * 0.75) + -0.75 + 0.25;
    return make_tuple(LandingTile, dirInLTile, centerdistInLTile);
}

bool valueToRKWaveBlockHit(float value) {
    return !(value - 1.0 <= 0.5);   // <=1.5 means hit; >1.5 means miss
}

string valueToCoverEffect(float value) {
    // correctness not confirmed yet
    int effect = 2 + static_cast<int>((value*3.5) - 3.5)*2;
    switch (effect) {
    case 2: //bounce
        return "Bounce";
        break;
    case 3: //burn
        return "Burn";
        break;
    case 4: //freeze
        return "Freeze";
        break;
    case 5: //honey
        return "Honey";
        break;
    case 6: //ink
        return "Ink";
        break;
    case 7: //push
        return "Push";
        break;
    case 8: //vamp
        return "Vamp";
        break;
    case 9: //stone
        return "Stone";
        break;
    case 10: //none
        return "None";
        break;
    default:
        break;
    }
    return " ";
}




uint32_t printState(uint32_t state, int iteration = 0, int stepSize = 1, bool showRow = true, bool showAsInt = false, bool showHex = false, bool showVal = true, int minDmg = -1, int maxDmg = -1, int baseDmg = -1, vector<int> candSizes = {-1}) {
    uint32_t temp = state;
    int candSize = candSizes.size();
    float val;
    if (iteration < 0) {stepSize = -stepSize;}
    iteration *= stepSize;
    for(int j = 0; abs(j) <= iteration; j += stepSize) {
        if(showRow) {cout << j << ":  ";}
        if(showAsInt) {cout << setw(11) << int(temp);}
        else {cout << setw(11) << temp;}
        val = stateToValue(temp);
        if (showHex) {cout << " (hex: 0x" << hex << uppercase << temp << dec << ")";}
        if(showVal) {cout << " (value: " << val << ")";}
        if(minDmg != -1) {
            cout << " (dmg: " << valueToDmg(val, minDmg, maxDmg, baseDmg) << ")"; 
        }
        if (candSizes[j%candSize] != -1) {
            cout << " (tile: " << int(val*candSizes[j%candSize] - candSizes[j%candSize]) << ")"; 
        }
        cout << endl;
        temp = lcgWrapper(temp, stepSize);
    }
    cout << endl;
    return temp;
}

void printMap(Map map, vector<Tile>& candidateTiles) {
    int temp;
    if (candidateTiles.size() == 0) {
        for (int y = 0; y < map.height(); ++y) {
            for (int x = 0; x < map.width(); ++x) {
                cout << (map.isOn(x, y) ? '1' : '0');
            }
            cout << '\n';
        }
    } else {
        for (int y = 0; y < map.height(); ++y) {
            for (int x = 0; x < map.width(); ++x) {
                if(map.isOn(x, y)) {
                    temp = getTilesetIndex(candidateTiles, x, y);
                    if(temp == -1) {
                        cout << setw(4) << "o";
                    } else {
                        cout << setw(4) << temp;
                    }
                } else {
                    cout << setw(4) << '-';
                }
            }
            cout << '\n';
        }
    }
}

void searchNextGoal(uint32_t state, vector<uint32_t> sortedNormedGoalStates, int Mstepsize = 1, int lookAhead = 2147483647) {
    uint32_t temp = (state < 2147483647) ? state : state-2147483648;
    vector<uint32_t, allocator<uint32_t>>::iterator start = sortedNormedGoalStates.begin();
    vector<uint32_t, allocator<uint32_t>>::iterator end = sortedNormedGoalStates.end();
    Affine affine = getLcgConsts(Mstepsize);
    
    int j = 0;
    while(!binary_search(start, end, temp)) {
        j++;
        temp = temp*affine.mult + affine.add;
        temp = (temp < 2147483648) ? temp : temp-2147483648; 
    }
    cout << endl << j << " (" << j*Mstepsize << "): " << temp << endl;
}


void getNextDmg(uint32_t state, int Mstepsize, int minDmg, int critDmg, int critHitSearch = -1, int dmgSearch = -1, int lookAhead = 1000, float critChance = 0.0, float hitChance = 1.0, int maxDmg = -1, int baseDmg = -1) {
    uint32_t temp = state;
    int critHit = valueToHitCrit(stateToValue(reverseLcg(temp)), critChance, hitChance);
    int dmg = valueToDmg(stateToValue(temp), minDmg, maxDmg, baseDmg);
    
    int j = 0;
    while(((dmg != dmgSearch) || (critHit != critHitSearch)) && (j <= lookAhead)) {
        critHit = valueToHitCrit(stateToValue(reverseLcg(temp)), critChance, hitChance);
        dmg = valueToDmg(stateToValue(temp), minDmg, maxDmg, baseDmg);
        cout << j << " (" << j*Mstepsize << "): ";
        if(critHit == 0) {cout << 0;} 
        else {if(critHit == 2) {cout << critDmg;} 
        else {cout << dmg;}}
        cout << endl;
        j++;
        temp = lcgWrapper(temp, Mstepsize);
    }
}

uint32_t rngManipHelper(uint32_t State = 0, int StepSize = 0, int MStepSize = 2, bool value = false) {
    uint32_t state = lcgWrapper(State, StepSize);
    uint32_t prevState1 = state, prevState2 = state, prevState3 = state;
    vector<uint32_t> goalStates;
    int stepSize = StepSize;
    int MstepSize = MStepSize;
    Weapon weapon;
    int mode = 0;

    string strIn = " ";
    char t;
    int m1, m2, m3, j = 0;


    cout << endl << "MANIP HELPER" << endl;
    cout << "Set state: ";
    cin >> strIn;
    transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
    t = strIn[0];
    if (t != 'x' && t != 'n') {state = stoi(strIn);}
    prevState1 = state;
    prevState2 = state;
    prevState3 = state;

    cout << "Step size: ";
    cin >> strIn;
    t = strIn[0];
    t = tolower(t);
    if (t != 'x') {
        if (t != 'n') {stepSize = stoi(strIn);}
        cout << "MStep size: ";
        cin >> strIn;
        transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
        t = strIn[0];
        if (t != 'n' && t != 'x') {MstepSize = stoi(strIn);}
    }

    cout << "Set search mode (0 = Search for damage; 1 = Search for goal States): ";
    cin >> strIn;
    transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
    t = strIn[0];
    if (t != 'x') {
        if (t != 'n') {
            mode = stoi(strIn);
        }
        if(mode == 1) {
            cout << "Set goal states (file name): ";
            cin >> strIn;
            t = strIn[0];
            t = tolower(t);
            if (t != 'n' && t != 'x') {goalStates = LoadVector(strIn);}
        }
    }
    weapon.setStats();
    cout << endl;


    if (mode == 0) {
        getNextDmg(state, MstepSize, weapon.minDmg, weapon.critDmg, -1, -1, 100, weapon.critChance, weapon.hitChance, weapon.maxDmg, weapon.baseDmg);
    } else {
        searchNextGoal(state, goalStates, MstepSize);
    }

    while (true)
    {
        cout << "Msteps taken: ";
        cin >> strIn;
        transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
        t = strIn[0];
        
        if (t == '?' || t == 'h') {
            cout << endl << "x = Stop" << endl;
            cout << "back = Revert to previous step" << endl;
            cout << "print = Show current State" << endl;
            cout << "weapon = Show weapon stats" << endl;
            cout << "change = Change values" << endl;
            cout << endl << "Mstepsize = 2 * Number of entities in range of character (for each weapon)" << endl << "Your team also counts towards entities." << endl << endl;
            continue;
        }
        if (t == 'x') {break;}
        if (t == 'b') {state = prevState1; prevState1 = prevState2; prevState2 = prevState3; continue;}
        if (t == 'p') {
            cout << "State: " << state;     
            if(value) {cout << " (" << stateToValue(state) << ")";}
            cout << endl << endl;
            continue;}
        if (t == 'w') {weapon.printStats(); cout << endl; continue;}
        if (t == 'c') {
            cout << "Set state: ";
            cin >> strIn;
            transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
            t = strIn[0];
            if (t != 'x' && t != 'n') {state = stoi(strIn);}
            cout << "Step size: ";
            cin >> strIn;
            t = strIn[0];
            t = tolower(t);
            if (t != 'x') {
                if (t != 'n') {stepSize = stoi(strIn);}
                cout << "MStep size: ";
                cin >> strIn;
                t = strIn[0];
                t = tolower(t);
                if (t != 'n' && t != 'x') {MstepSize = stoi(strIn);}
            }
            cout << "Set search mode (0 = Search for damage; 1 = Search for goal states): ";
            cin >> strIn;
            transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
            t = strIn[0];
            if (t != 'x') {
                if (t != 'n') {
                    mode = stoi(strIn);
                }
                if(mode == 1) {
                    cout << "Set goal states (file name): ";
                    cin >> strIn;
                    t = strIn[0];
                    t = tolower(t);
                    if (t != 'n' && t != 'x') {goalStates = LoadVector(strIn);}
                }
            }
            weapon.setStats();
            cout << endl;
            continue;
        }

        j = stepSize + stoi(strIn) * MstepSize;
        
        prevState3 = prevState2;
        prevState2 = prevState1;
        prevState1 = state;
        state = lcgWrapper(state, j);
        cout << "RNG-Steps taken: " << j << endl;
        if (mode == 0) {
            getNextDmg(state, MstepSize, weapon.minDmg, weapon.critDmg, -1, -1, 100, weapon.critChance, weapon.hitChance, weapon.maxDmg, weapon.baseDmg);
        } else {
            searchNextGoal(state, goalStates, MstepSize);
        }
    }
    return state;
}


vector<uint32_t> searchStates(vector<uint32_t>& states, int minStepSize = 1, int maxStepSize = 0, tuple<float, float> vals = make_tuple(0, 0), tuple<float, float> exVals = make_tuple(0, 0), bool routeMode = false) {
    // exVals are for checking restricting the previous value right before Vals
    vector<uint32_t> out;
    uint32_t lcgAdd, lcgMult, minMant, maxMant, exMinMant, exMaxMant, nextState, m;
    if(abs(maxStepSize) < abs(minStepSize)) {maxStepSize = minStepSize;}
    tie(minMant, maxMant) = valueToMantissa(vals);
    tie(exMinMant, exMaxMant) = valueToMantissa(exVals);
    Affine affine = getLcgConsts(minStepSize);
    lcgMult = affine.mult;
    lcgAdd = affine.add;

    if(routeMode) {
        minMant += 0x100;
        maxMant -= 0x100*(maxMant != 1);
        exMinMant += 0x100;
        exMaxMant -= 0x100*(exMaxMant != 1);
    }


    if(states.size() == 0) {
        uint32_t state, maxState, prevMantissa;
        if (maxMant == 1) {
            if (exMaxMant == 1) {return states;}
            
            exMinMant = exMinMant >> 8;
            exMaxMant = exMaxMant >> 8;
            out.reserve((exMaxMant-exMinMant + 1)*(1u<<16));
            for (uint32_t m = exMinMant; m <= exMaxMant; m++) {
                state = m << 16;
                maxState = state + (1u<<16);
                while(state < maxState) {
                    out.push_back(state * 214013u + 2531011u);
                    state += 1;
                }}
            return out;
        }


        minMant = minMant >> 8;
        maxMant = maxMant >> 8;
        out.reserve((maxMant-minMant + 1)*(1u<<16));
        if (exMaxMant == 1) {
            for (uint32_t m = minMant; m <= maxMant; m++) {
                state = m << 16;
                maxState = state + (1u<<16);
                while(state < maxState) {
                    out.push_back(state);
                    state += 1;
                }}
            return out;
        }


        exMaxMant = exMaxMant | 0xff;
        for (uint32_t m = minMant; m <= maxMant; m++) {
            // state 0[000 0000 0000 0000] 0000 0000 /0000 0000
            state = m << 16;
            maxState = state + (1u<<16);
            while(state < maxState) {
                prevMantissa = ((state - 2531011u) * 3115528533u >> 8) & 0x7fff00;
                if ((exMinMant <= prevMantissa) && (prevMantissa <= exMaxMant)) {
                    out.push_back(state);
                }
                state += 1;
            }}
        return out;
    }

    out.reserve(states.size());
    if (maxMant == 1) {
        if (exMaxMant == 1) {
            for (uint32_t state : states) {
                nextState = state * lcgMult + lcgAdd;
                for(int i = minStepSize; i <= maxStepSize; i++) {
                    out.push_back(nextState);
                    nextState = nextState * 214013 + 2531011;
                }
            }
            return out;
        }
        

        for (uint32_t state : states) {
            nextState = state * lcgMult + lcgAdd;
            for(int i = minStepSize; i <= maxStepSize; i++) {
                m = (((nextState - 2531011) * 3115528533) >> 8) & 0x7fff00;
                if (exMinMant <= m && m <= exMaxMant) {
                    out.push_back(nextState);
                }
                nextState = nextState * 214013 + 2531011;
            }
        }
        return out;
    } 


    if (exMaxMant == 1) {
        for (uint32_t state : states) {
            nextState = state * lcgMult + lcgAdd;
            for(int i = minStepSize; i <= maxStepSize; i++) {
                m = (nextState >> 8) & 0x7fff00;
                if (minMant <= m && m <= maxMant) {
                    out.push_back(nextState);
                }
                nextState = nextState * 214013 + 2531011;
            }
        }
        return out;
    }


    for (uint32_t state : states) {
        nextState = state * lcgMult + lcgAdd;
        for(int i = minStepSize; i <= maxStepSize; i++) {
            m = (nextState >> 8) & 0x7fff00;
            if (minMant <= m && m <= maxMant) {
                m = (((nextState - 2531011) * 3115528533) >> 8) & 0x7fff00;
                if (exMinMant <= m && m <= exMaxMant) {
                    out.push_back(nextState);
                }
            }
            nextState = nextState * 214013 + 2531011;
        }
    }
    return out;
}

vector<uint32_t> stateFinder(vector<uint32_t> initState = {}, int minStepSize = 1, int maxStepSize = -1, bool routeMode = false) {
    vector<uint32_t> states = initState, prevStates1, prevStates2, prevStates3;
    tuple<float, float> hitCrit, dmg;
    string strIn;
    Weapon weapon;
    uint32_t temp = 4294967296;
    Affine affine;
    char t;
    int mode = 0;

    cout << endl << "STATE FINDER" << endl;
    cout << "Min step size: ";
    cin >> strIn;
    t = strIn[0];
    t = tolower(t);
    if (t != 'x') {
        if (t != 'n') {
            minStepSize = stoi(strIn);
        }
        cout << "Max step size: ";
        cin >> strIn;
        t = tolower(t);
        t = strIn[0];
        if (t != 'x' && t != 'n') {
            maxStepSize = stoi(strIn);
        } else {
            maxStepSize = minStepSize;
        }
    }
    weapon.setStats();
    cout << endl;

    while (true)
    {
        cout << "Possible states: " << temp << endl;
        cout << "Damage: ";
        cin >> strIn;
        transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
        t = strIn[0];

        
        if (t == '?' || t == 'h') {
            cout << endl << "x = Stop" << endl;
            cout << "back = Revert to previous step" << endl;
            cout << "next = Skip a step" << endl;
            cout << "save = Save current States" << endl;
            cout << "load = Load into current States" << endl;
            cout << "print = List current States" << endl;
            cout << "weapon = Show (weapon) stats" << endl;
            cout << "change = Change values" << endl;
            cout << endl;
            continue;
        }
        if (t == 'x') {break;}
        if (t == 'b') {states = prevStates1; prevStates1 = prevStates2; prevStates2 = prevStates3; temp = states.size(); continue;}
        if (t == 'n') {states = searchStates(states, minStepSize, 0, make_tuple(0, 0), make_tuple(0, 0), routeMode); continue;}
        if (t == 's') {
            cout << "File name: ";
            cin >> strIn;
            SaveVector(strIn, states);
            cout << endl;
            continue;
        }
        if (t == 'l') {
            cout << "File name: ";
            cin >> strIn;
            states = LoadVector(strIn);
            cout << endl;
            temp = states.size();
            continue;
        }
        if (t == 'p') {
            temp = states.size();
            cout << "States in " << minStepSize << " steps: " << endl;
            for (uint32_t state : states) {
                affine = getLcgConsts(minStepSize);
                cout << int(state*affine.mult + affine.add) << endl;
            }
            cout << endl;
            cout << endl;
            continue;
        }
        if (t == 'w') {
            if (minStepSize < maxStepSize) {
                cout << endl << "Min step size: " << minStepSize << endl << "Max step size: " << maxStepSize << endl;
            } else {
                cout << endl << "Step size: " << minStepSize << endl;
            }
            weapon.printStats(); cout << endl; continue;}
        if (t == 'c') {
            cout << "Step size: ";
            cin >> strIn;
            t = strIn[0];
            t = tolower(t);
            if (t != 'x') {
                if (t != 'n') {
                    minStepSize = stoi(strIn);
                }
                cout << "Max step size: ";
                cin >> strIn;
                t = strIn[0];
                t = tolower(t);
                if (t != 'x' && t != 'n') {
                    maxStepSize = stoi(strIn);
                } else {
                    maxStepSize = minStepSize;
                }
            }
            weapon.setStats();
            cout << endl;
            continue;
        }

        temp = stoi(strIn);
        if (temp == weapon.critDmg) {
            hitCrit = hitCritToValue(2, weapon.critChance, weapon.hitChance);
            dmg = make_tuple(0, 0);
            cout << "Hit/Crit vals: " << fixed << setprecision(2) << get<0>(hitCrit) << " " << get<1>(hitCrit) << endl; 
        } else {if (temp == -1) {
            hitCrit = hitCritToValue(0, weapon.critChance, weapon.hitChance);
            dmg = make_tuple(0, 0);
            cout << "Hit/Crit vals: " << fixed << setprecision(2) << get<0>(hitCrit) << " " << get<1>(hitCrit) << endl;
        } else {
            hitCrit = hitCritToValue(1, weapon.critChance, weapon.hitChance);
            dmg = dmgToValue(temp, weapon.minDmg, weapon.maxDmg, weapon.baseDmg);
            cout << "Hit/Crit vals: " << fixed << setprecision(2) << get<0>(hitCrit) << " " << get<1>(hitCrit) << endl;
            cout << "Damage vals: " << fixed << setprecision(2) << get<0>(dmg) << " " << get<1>(dmg) << endl;
        }}
        prevStates3 = prevStates2;
        prevStates2 = prevStates1;
        prevStates1 = states;
        states = searchStates(states, minStepSize, maxStepSize, dmg, hitCrit, routeMode);
        temp = states.size();

        if (temp > 0 && temp <= 10) {
            cout << "States in " << minStepSize << " steps: " << endl;
            for (uint32_t state : states) {
                affine = getLcgConsts(minStepSize);
                cout << int(state*affine.mult + affine.add) << endl;
            }
            cout << endl;
        }
    }
    return states;
}


int main() {
    uint32_t state = 0, temp;
    vector<uint32_t> states = {}, tempStates = {};
    vector<uint32_t> start1, end1, start2, end2;
    vector<uint32_t> bounce50, bounce05, bounce51, bounce15, bounce52, bounce25, bounce42, bounce24, bounce43, bounce34, bounce44;
    float val;

    // UC3
    if(false) {

        Map muc3, valmuc3, valFBmuc3;
        muc3.load("uc3.grid");
        muc3.setNeighbours(18, 17, 12);
        muc3.setNeighbours(19, 17, 7);
        muc3.setNeighbours(18, 22, 9);
        muc3.setNeighbours(19, 22, 7);
        muc3.setNeighbours(6, 16, 9);
        muc3.setNeighbours(6, 17, 14);
        muc3.setNeighbours(6, 22, 11);
        muc3.setNeighbours(6, 23, 12);

        valmuc3.load("uc3_val.grid");
        valFBmuc3.load("uc3_valFB.grid");

        Tile RabbidLuigi;
        RabbidLuigi.x = 18;
        RabbidLuigi.y = 17;

        Tile Luigi;
        Luigi.x = 21;
        Luigi.y = 14;

        Tile Ziggy;
        Ziggy.x = 22;
        Ziggy.y = 15;


        vector<Tile> candidateTilesRL = {RabbidLuigi};
        vector<int> invIndexRL = {102, 139, 225, 153, 184, 10, 16, 24, 35, 49, 67, 84, 39, 52, 71, 89, 101, 113};
        vector<int> invIndexRLFB = {102, 139, 225, 153, 184};
        Map candidateMapRL = generateCandidateTiles(muc3, candidateTilesRL);
        tuple<float, float> rl = make_tuple(1.808, 1.811);
        
        vector<Tile> candidateTilesL = {Luigi};
        vector<int> invIndexL = {8, 124, 40, 153, 28, 21, 19, 25, 31, 37, 70, 60, 72, 87, 76, 91};
        vector<int> invIndexLFB = {8, 124, 40, 153};
        Map candidateMapL = generateCandidateTiles(muc3, candidateTilesL);
        tuple<float, float> l = make_tuple(1.5643, 1.5644);

        int rlSize = candidateTilesRL.size();
        cout << endl << "Rabbid Luigi: " << rlSize-1 << endl;
        //printMap(muc3, candidateTilesRL);
        int lSize = candidateTilesL.size();
        cout << endl << "Luigi: " << lSize-1 << endl;
        //printMap(muc3, candidateTilesL);

        vector<Tile> candidateTilesZ = {Ziggy};
        Map candidateMapZ = generateCandidateTiles(muc3, candidateTilesZ);
        cout << endl << "Ziggy: " << candidateTilesZ.size()-1 << endl;
        //printMap(muc3, candidateTilesZ);

        //tie(states, tempStates) = fastBurnSim(states, candidateTilesRL, candidateMapRL, valmuc3, valFBmuc3, rl);
        
        states = searchStates(states, 1, 0, rl, make_tuple(0, 0), false);
        states = searchStates(states, 2, 0, l, make_tuple(0, 0), false);
    }

    // 4-3-2
    if (false) {
        Map m43;
        m43.load("4-3-2.grid");
        Tile Buckler;
        Buckler.x = 30;
        Buckler.y = 10;
        Tile Lu3;
        Lu3.x = 31;
        Lu3.y = 9;
        vector<Tile> candidateTilesBuckler = {Buckler};
        Map candidateMapBuckler = generateCandidateTiles(m43, candidateTilesBuckler);
        cout << endl << "Buckler: " << candidateTilesBuckler.size()-1 << endl;
        //printMap(m43, candidateTilesBuckler);
        vector<Tile> candidateTilesLu3 = {Lu3};
        Map candidateMapLu3 = generateCandidateTiles(m43, candidateTilesLu3);
        cout << endl << "Luigi: " << candidateTilesLu3.size()-1 << endl;
        //printMap(m43, candidateTilesLu3);
    }

    // 4-6-1
    if(false) {
        Map m461;
        m461.load("4-6-1.grid");
        m461.setNeighbours(15, 3, 12);
        m461.setNeighbours(15, 4, 13);
        m461.setNeighbours(15, 5, 9);

        m461.setNeighbours(16, 3, 7);
        m461.setNeighbours(16, 4, 7);
        m461.setNeighbours(16, 5, 7);

        m461.setNeighbours(15, 9, 12);
        m461.setNeighbours(15, 10, 9);
        m461.setNeighbours(15, 15, 12);
        m461.setNeighbours(15, 16, 9);

        m461.setNeighbours(15, 8, 11);
        m461.setNeighbours(16, 9, 7);
        m461.setNeighbours(16, 10, 7);
        m461.setNeighbours(16, 15, 7);
        m461.setNeighbours(16, 16, 7);
        m461.setNeighbours(15, 17, 14);


        m461.setNeighbours(19, 17, 6);
        m461.setNeighbours(19, 18, 3);
        m461.setNeighbours(21, 22, 3);

        m461.setNeighbours(19, 16, 11);
        m461.setNeighbours(18, 17, 13);
        m461.setNeighbours(18, 18, 13);
        m461.setNeighbours(21, 23, 14);


        m461.setNeighbours(21, 25, 6);
        m461.setNeighbours(19, 25, 12);
        m461.setNeighbours(18, 25, 10);
        m461.setNeighbours(15, 25, 12);
        m461.setNeighbours(14, 25, 10);
        m461.setNeighbours(13, 25, 14);
        m461.setNeighbours(12, 25, 6);

        m461.setNeighbours(21, 24, 11);
        m461.setNeighbours(19, 24, 11);
        m461.setNeighbours(18, 24, 11);
        m461.setNeighbours(17, 25, 13);
        m461.setNeighbours(16, 25, 7);
        m461.setNeighbours(15, 24, 11);
        m461.setNeighbours(14, 24, 11);
        m461.setNeighbours(13, 24, 11);
        m461.setNeighbours(12, 24, 11);


        m461.setNeighbours(11, 26, 6);
        m461.setNeighbours(11, 27, 7);
        m461.setNeighbours(11, 28, 3);

        m461.setNeighbours(10, 26, 13);
        m461.setNeighbours(10, 27, 13);
        m461.setNeighbours(10, 28, 9);

        Tile Spawny;
        Spawny.x = 14;
        Spawny.y = 13;
        vector<Tile> candidateTilesS = {Spawny};
        Map candidateMapS = generateCandidateTiles(m461, candidateTilesS);
        cout << endl << "Spawny: " << candidateTilesS.size()-1 << endl;
        //printMap(m461, candidateTilesS);
    }

    // 2-2-1
    if(true) {
        Map m22, m22g3, m22g4, m22val, m22map, m22used;
        m22.load("2-2.grid");
        m22g3.load("2-2g3.grid");
        m22g4.load("2-2g4.grid");
        m22val.load("2-2val.grid");
        m22map.load("2-2map.grid"); //all tiles on map, unless out of bounds
        m22used.load("2-2used.grid");

        Tile Mario;
        Mario.x = 23;
        Mario.y = 11;
        vector<Tile> candidateTilesM = {Mario};
        Map candidateMapM = generateCandidateTiles(m22, candidateTilesM);


/*
        states = searchStates(states, 1, 0, {1.0, 1.5}, {0, 0}, true);
        states = searchStates(states, 73, 0, {1.6, 2.0}, {0, 0}, true);
        
        tempStates = searchStates(states, -437, 0, {1.0, 2.0}, {0, 0}, true);
        states = searchStates(states, 98, 0, {1.0, 2.0}, {0, 0}, true);

        tie(start1, end1) = fastBurnSimFirstArea(states, Mario, m22g3, m22g4, candidateTilesM, m22map, m22val);         
*/

        //cout << lcgWrapper(575475233, -597) << endl;
        //printState(lcgWrapper(575475233, -535), 550);

        // goal vals: 243/272
        cout << endl << "Mario: " << candidateTilesM.size()-1 << endl;
        printMap(m22, candidateTilesM);
        cout << endl;

        //tie(states, tempStates) = fastBurnSimFirstAreaEmpty(Mario, m22g3, m22g4, candidateTilesM, m22map, m22val); 
        start1 = LoadVector("2-2StartToFallbackStart");
        end1 = LoadVector("2-2StartToFallbackEnd");

        //tie(start2, end2) = fastBurnSimLastArea(end1, candidateTilesM, m22map, m22val, m22val, m22used, make_tuple(1.0 + 243.0/272.0, 1.0 + 244.0/272.0));
        start2 = LoadVector("2-2ToFallbackStart");
        end2 = LoadVector("2-2ToFallbackFakeEnd");

        cout << "Merging..." << endl;
        tie(states, tempStates) = MergeStatePairs(start1, end1, start2, end2);
        states = searchStates(states, -535, 0, {1.0, 2.0}, {0, 0});
        SaveVectorUnsorted("2-2BurnStart", states);
        SaveVectorUnsorted("2-2BurnEnd", tempStates);
    }


    states = stateFinder({}, 1, -1, true);
    while(true) {
        if(states.size() > 0) {state = rngManipHelper(states[0]);}
        else {state = rngManipHelper();}
        states = stateFinder({state});
    }

    /* 
     * m = 8/per w     c=2m       m = 6/per w
     * 
     * Blaster offset: 38, 34, 24 (20)
     * Rumblebang offset: 44, 38 (26), 26 (24)
     * offset: 34           (m/2*(w + targeted) + 4)????    (24) (20)
     * shot: 74             (offset + m*(w + t) - 2)???     (54) (42)
     * start of battle: 56  (m*w in range + c on M)         (24) (24)
     * 
     * 
     * 
     * start of battle: 64
     * mstep: 8
     * shot: 84
     * dmg offset: 38
     * crit/hit offset: 37
     * step: 148
     * 
     * 
     * 
     * 
     * 
     * 
     * 
     * 
     * 
     * vars?: Character, Weapon, Targets
     *
     * b778/b7fc: (2*(hit+dmg))*A
     * 72f8: (hit)*B*C[*2]*D*E      // on some shots
     * bc08: (hit+dmg)              
     *
     * a46c: ((2*bc08)*B)[*C]
     * aa9c: ((2*bc08)*B)[*C]
     * b0cc: (2*bc08)*B             
     * b16c: (2*bc08)*B             
     * b1e0: (2*bc08)*B*C[*D]
     * b700: (2*bc08)*B[*C]
     * 
     * 
     * 
     * 
     * M: p+s; RL: p; Y: p; 2 Z       > 5 in range
     * 1-1-1
     * start of battle (+c): 56 (48 + 8 = 6*8 + 8)
     * first target: 32
     * subsequent targets: 16
     * shot (+m): 26 (20 + 6 = 5*4 + 6)
     * cycle: 130
     * m = 8(1), 16(2) / 6(1), 12(2) * w      c = 2*m
     * 
     * 
     * 2-1-1: 98 112 118   m:6,  c:6 , e:14(6)
     * 1-1-1/2: 40 52 64   m:8,  c:12, e:12(8)
     * 1-2-1: 56 72 88     m:10, c:16, e:16(10)
     * 1-2-2: 50 66 84     m:12, c:18, e:16(10)
     * 3-4-1: 28 30 36     m:6,  c:6,  e:2(0)
     * M: p+s; RL: p+s; Y: p; 2 Z       > 6 in range
     * 1-1-1
     * start of battle (+c): 64 (48 + 16 = 6*8 + 16)
     * first target: 36
     * subsequent targets: 18
     * shot (+m): 30 (24 + 6 = 6*4 + 6)
     * cycle: 148
     * m = 8(1), 16(2) / 6(1), 12(2) * w
     * 
     * 
     * M: p+s; RL: p+s; Y: p+s; 2 Z       > 6+4 in range +2*3
     * 1-1-1
     * start of battle (+c): 72 (56 + 16 = 7*8 + 16)
     * first target: 40
     * subsequent targets: 20
     * shot (+m): 34 (28 + 6 = 7*4 + 6)
     * cycle: 166
     * m = M:8/6, Y:16/12
     * 
     * 
     * 1-2-1
     * M: p+s; RL: p+s; Y: p+s; 3 Z       > 8+6 in range +2*2+3
     * start of battle (+c): 106 ( +  =  + 14)
     * shot (+m): 44 (32 + 12 =  + )
     * cycle: 
     * m = M:14/12/10, RL:18, Y:20/10   (RY: default p+s 18)     c = m + [m]
     *
     * 1-2-1: Mps RLps Yps: 40 > 32            (2+1)*(5+2)  +  (4+5)  +  (5+5)
     * 1-2-1: Mp          : 15 > 15
     *        Mps         : 21 > 21
     * 1-2-1: Mp  RLps    : 24 > 24
     * 
     * 
     * 
     * 19 (2W, 3Z)          m=12: 46 79 21      94 27 21
     * 15 (1W, 3Z)            10: 69 97 18      57 85 18
     * 
     *                
     * Mp
     *  w/M: 68 96 18  w/M RPmv: 35 59 16  nM: 50 73 15  nM RPmv: 15
     *(m=10) 60 96 26      (m=8) 39 63 16      23 52 21           15
     *       70 06 26            73 05 24      04 35 23     66 97 23 
     * Mp RLps (+12)
     *       97 37 30            75 11 28      35 70 27     38 73 27
     *       29 81 42            45 81 28      66 11 37     99 34 27
     *       73 41 58            53 17 56      12 75 55     52 15 55
     * 
     *       0/0   0/12            0/0   0/12      0/0   0/12     0/0   0/12
     *       8/12  4/16            0/0   0/12      6/10  4/16     0/0   0/12
     *       8/28  20/32           8/28  20/32     8/28  20/32    8/28  20/32
     *              
     * 
     * 
     * 16
     * 03 20 17            26   27
     * 18 37 19
     * 55 74 19                  15 44 23        68 99 25
     *                               71 06 27
     * 21 20 17        m12  85 18 21
     * 18 56 26    50 83 21         78 13 27
     * 
     * 17 45 18        39 62 15       50 73 15
     *                 35 64 21       04 39 27   70 98 22
     * 22 45 15    95 18 15     00 23 15    83 06 15
     * 1-1-1, m=6:
     * (3): 336 362 20
     * (1): 538 556 12           (2) m=8: 58 74 = 16
     *       664 682 12        w/M(2) m=8: 26 45 = 19
     *       766 784 12
     * (3): 896 922 20 
     * (2): 28  50  16  
     *                  m=6: 98 120 16      m=8: 34 50 16
     * 2-1-1, m=6:
     * 1:193 202=9
     * 2:284 295=11          w/M (m=6): 43 56 = 13
     * 
     * 
     * 
     * 1-1-1 shot
     * 20   no RL p+s: 12
     * 16   wRM hammer in range: 17
     *  
     * 16
     * 
     * Mp Yp+s: 28
     * 35 75 12 > 28
     * 03 29 6  > 20
     * 
     * 1-2-1:
     * 19
     * 17
     * 
     * 
     *          +2/per entity with target in range
     *          +2/per entity with char in range
     *  
     *          +2/per entity in range of char (before shot)
     *          +1/per entity in range of team/weapon (before shot)
     * 
     *
     *
     * 

     * start of battle:        +c 
     *                         +?

     * [only if not moved?]
     * first target by char:   +2/per entity/weapon with char in range
     *                         +2 per entity in range of char (for each weapon)
     * 
     * first target on enemy:  +2/per entity/weapon with target in range  
     *                         +2/per entity in range of target
     * 
     * shot:                    +2/per entity in range of char (before shot)
     *                          +1/per entity in range of team/weapon (before shot)
     *                          +? 
     * 
     * c = m [+m]
     * 
     * m = Mov/tile preview (per valid tile):
     *          +2 per entity in range of char (for each weapon)
     * 
     * switch players in co-op: 20???????
     * AOE: +1 per in AOE
     * 
     * Move action: M: 56/52, RL: 56, Y:68      (28, 20, 28)
     *          +4
     *          +2/per enemy (weapon) reaching char
     *          +m before +m after???

     * 
     */

}