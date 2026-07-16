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
#include <bit>

using namespace std;


struct Weapon
{
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
            transform(input.begin(), input.end(), input.begin(), ::tolower);
            letter = input[0];

            if (letter == 'x') {
                if (index <= 1) {minSkipped = true;}
                if ((index <= 2) && !minSkipped) {maxDmg = minDmg + 10;}
                if ((index <= 3) && !minSkipped) {baseDmg = (minDmg + maxDmg)/2;}
                return false;}
            if (letter  == 'b') {if (index > 0) {index--;} continue;}
            if (letter == 'n') {
                cout << minSkipped << index << endl;
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

void SaveVector(const string& filename, const vector<uint32_t>& data)
{
    ofstream file(filename, ios::binary);
    if (!file)
        throw runtime_error("Failed to open file for writing.");

    uint64_t size = data.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    if (size > 0)
    {
        file.write(reinterpret_cast<const char*>(data.data()),
                   size * sizeof(uint32_t));
    }

    if (!file)
        throw runtime_error("Failed while writing file.");
}

vector<uint32_t> LoadVector(const string& filename)
{
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


// rngState = *(int *)(DAT_710321c5b0 + 0x364) * 214013 + 2531011;
// rngValue = (float)(rngState >> 8 & 0x7fff00 | 0x3f800000)
uint32_t lcg(uint32_t seed) {return seed * 214013 + 2531011;}

uint32_t reverseLcg(uint32_t seed) {return (seed - 2531011) * 3115528533;}

tuple<uint32_t, uint32_t> getLcgConsts(int steps) {
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
    return make_tuple(add, mult);
}

uint32_t lcgWrapper(uint32_t seed, int steps) {
    uint32_t mult, add;
    tie(add, mult) = getLcgConsts(steps);
    return (seed * mult) + add;
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
        bits = bit_cast<uint32_t>(minVal);
        minMantissa = bits & 0x7fff00;

        if(maxVal >= 1.999969482421875) {
            maxMantissa = 0x7fff00;
        } 
        else {
            bits = bit_cast<uint32_t>(maxVal);
            maxMantissa = bits & 0x7fff00;
        }
    }
    return make_tuple(minMantissa, maxMantissa);
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

int valueToCoinSpawnTile(float value, int numValidTiles) {
    return (value * numValidTiles) - numValidTiles;
}
int valueToCoinSpawnDirection(float value) {
    return (value * 6.2831855) - 6.2831855;
}
int valueToCoinDistanceFromCenter(float value) {
    return (value * 0.75) + -0.75 + 0.25;
}

bool valueToRKWaveBlockHit(float value) {
    return (value - 1.0 <= 1.5);
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


uint32_t printState(uint32_t state, int iteration, int stepSize = 1, bool row = true, bool value = false, int minDmg = -1, int maxDmg = -1, int baseDmg = -1) {
    uint32_t temp = state;
    float val;
    if (iteration < 0) {stepSize = -stepSize;}
    iteration *= stepSize;
    for(int j = 0; abs(j) <= iteration; j += stepSize) {
        if(row) {cout << j << ":  ";}
        cout << int(temp);
        val = stateToValue(temp);
        if(value) {cout << " (value: " << val << ")";}
        if(minDmg != -1) {
            cout << " (dmg: " << valueToDmg(val, minDmg, maxDmg, baseDmg) << ")"; 
        }
        cout << endl;
        temp = lcgWrapper(temp, stepSize);
    }

    return temp;
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

uint32_t stateProgressHandler(uint32_t State = 0, int StepSize = 0, int MStepSize = 2, bool value = false) {
    uint32_t state = lcgWrapper(State, StepSize);
    uint32_t prevState = state;
    int stepSize = StepSize;
    int MstepSize = MStepSize;
    Weapon weapon;

    string strIn = " ";
    char t;
    int m1, m2, m3, j = 0;

    cout << "Set state: ";
    cin >> strIn;
    transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
    t = strIn[0];
    if (t != 'n' && t != 'x') {
        state = stoi(strIn);
    }
    prevState = state;

    cout << "Step size: ";
    cin >> strIn;
    transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
    t = strIn[0];
    if (t != 'x') {
        if (t != 'n') {stepSize = stoi(strIn);}
        cout << "MStep size: ";
        cin >> strIn;
        transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
        t = strIn[0];
        if (t != 'n' && t != 'x') {MstepSize = stoi(strIn);}
    }
    weapon.setStats();
    cout << endl;

    getNextDmg(state, MstepSize, weapon.minDmg, weapon.critDmg, -1, -1, 100, weapon.critChance, weapon.hitChance, weapon.maxDmg, weapon.baseDmg);

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
        if (t == 'b') {state = prevState; continue;}
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
            if (t != 'n' && t != 'x') {
                state = stoi(strIn);
            }
            cout << "Step size: ";
            cin >> strIn;
            transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
            t = strIn[0];
            if (t != 'x') {
                if (t != 'n') {stepSize = stoi(strIn);}
                cout << "MStep size: ";
                cin >> strIn;
                transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
                t = strIn[0];
                if (t != 'n' && t != 'x') {MstepSize = stoi(strIn);}
            }
            weapon.setStats();
            cout << endl;
            continue;
        }

        j = stepSize + stoi(strIn) * MstepSize;
        prevState = state;
        state = lcgWrapper(state, j);
        cout << "RNG-Steps taken: " << j << endl;
        getNextDmg(state, MstepSize, weapon.minDmg, weapon.critDmg, -1, -1, 100, weapon.critChance, weapon.hitChance, weapon.maxDmg, weapon.baseDmg);
    }
    return state;
}


vector<uint32_t> possibleStates(vector<uint32_t>& states, int stepSize = 1, tuple<float, float> vals = make_tuple(0, 0), tuple<float, float> exVals = make_tuple(0, 0)) {
    vector<uint32_t> out;
    uint32_t lcgAdd, lcgMult, exLcgAdd, exLcgMult, minMant, maxMant, exMinMant, exMaxMant, nextState, m;
    tie(minMant, maxMant) = valueToMantissa(vals);
    tie(exMinMant, exMaxMant) = valueToMantissa(exVals);

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
            tie(lcgAdd, lcgMult) = getLcgConsts(stepSize);
            for (uint32_t state : states) {
                nextState = state * lcgMult + lcgAdd;
                out.push_back(nextState);
            }
            return out;
        }
        

        for (uint32_t state : states) {
            tie(exLcgAdd, exLcgMult) = getLcgConsts(stepSize - 1);
            nextState = state * exLcgMult + exLcgAdd;
            m = (nextState >> 8) & 0x7fff00;
            if (exMinMant <= m && m <= exMaxMant) {
                out.push_back(nextState * 214013u + 2531011u);
            }
        }
        return out;
    } 


    if (exMaxMant == 1) {
        tie(lcgAdd, lcgMult) = getLcgConsts(stepSize);
        for (uint32_t state : states) {
            nextState = state * lcgMult + lcgAdd;
            m = (nextState >> 8) & 0x7fff00;
            if (minMant <= m && m <= maxMant) {
                out.push_back(nextState);
            }
        } 
        return out;
    }


    tie(exLcgAdd, exLcgMult) = getLcgConsts(stepSize - 1);
    for (uint32_t state : states) {
        nextState = state * exLcgMult + exLcgAdd;
        m = (nextState >> 8) & 0x7fff00;
        if (exMinMant <= m && m <= exMaxMant) {
            nextState = nextState * 214013u + 2531011u;
            m = (nextState >> 8) & 0x7fff00;
            if (minMant <= m && m <= maxMant) {
                out.push_back(nextState);
            }
        }
    }
    return out;
}

vector<uint32_t> stateFinder(int stepSize = 1) {
    vector<uint32_t> states, prevStates;
    tuple<float, float> hitCrit, dmg;
    string strIn;
    Weapon weapon;
    uint32_t temp = 4294967296, lcgAdd, lcgMult;
    char t;

    cout << "Step size: ";
    cin >> strIn;
    transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
    t = strIn[0];
    if (t != 'n' && t != 'x') {
        stepSize = stoi(strIn);
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
            cout << "weapon = Show weapon stats" << endl;
            cout << "change = Change values" << endl;
            cout << endl;
            continue;
        }
        if (t == 'x') {break;}
        if (t == 'b') {states = prevStates; temp = states.size(); continue;}
        if (t == 'n') {states = possibleStates(states, stepSize); continue;}
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
            cout << "States for next shot: " << endl;
            for (uint32_t state : states) {
                tie(lcgAdd, lcgMult) = getLcgConsts(stepSize);
                cout << int(state*lcgMult + lcgAdd) << endl;
            }
            cout << endl;
            cout << endl;
            continue;
        }
        if (t == 'w') {weapon.printStats(); cout << endl; continue;}
        if (t == 'c') {
            cout << "Step size: ";
            cin >> strIn;
            transform(strIn.begin(), strIn.end(), strIn.begin(), ::tolower);
            t = strIn[0];
            if (t != 'n' && t != 'x') {
                stepSize = stoi(strIn);
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
        prevStates = states;
        states = possibleStates(states, stepSize, dmg, hitCrit);
        temp = states.size();

        if (temp <= 10) {
            cout << "States for next shot: " << endl;
            for (uint32_t state : states) {
                tie(lcgAdd, lcgMult) = getLcgConsts(stepSize);
                cout << int(state*lcgMult + lcgAdd) << endl;
            }
            cout << endl;
        }
    }
    return states;
}


int main() {
    uint32_t state, temp;
    vector<uint32_t> states = {0}, tempStates;

    int stepSize = 148;
    int MstepSize = 8;
    int offset = 2;


    states = stateFinder();
    if (states.size()) {stateProgressHandler(states[0], 164, 8);}
    else {stateProgressHandler();}


    /** 
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
     *          +2 per entity in range of char for each weapon
     * 
     * 
     * AOE: +1 per in AOE
     * 
     * Move action: M: 56/52, RL: 56, Y:68      (28, 20, 28)
     *          +4
     *          +2/per enemy (weapon) reaching char
     *          +m before +m after???

     * 
     */
}