# RNG-Manipulation-Mario-Rabbids-Kingdom-Battle-
Work in progress  
  
This is a tool for RNG manipulation in Mario+Rabbids: Kingdom Battle.  
  
RNG in Mario+Rabbids is represented as a 32-bit integer. It uses a Linear Congruential Generator to update the value.  
Specifically: x = x * 214013 + 2531011. The initial seed is generated using the system tick, which is determined by the cpu clock cycle.  
  
The RNG state is progressed by 30 different functions, some of which progress it multiple (potentially variable amount of) times. We still don't know the purpose of all functions, but here are most of them:  
- Enemy action related (Hoppers, Valkyries, Bwaario, Lava Queen)
- Crit + Hit on Boomshot for extra AOE targets
- Crit + Hit (Is called on some battle menu actions on movement, targetting and more, even if no crit/hit roll is needed)
- Hidden Explosive Cover preset
- Load tips
- Damage roll (Is called on some battle menu actions on movement, targetting and more, even if no damage roll is needed)
- (Explosive/) Cover damage effect?
- Random explosive cover effect
- Coin direction on spawn
- Bounce tie breaker		
- 4 related to Burn
- Versus Coin
- 4 related to Pyroclasts
- Another related to Pyroclast??
- Related to Pyroclasts/Honey Bombs/Ink Coconuts???
- Boo teleport preset
- Boo char teleport
- Tornado
- Rabbid Kong wave block hits
- Load areas/End turn related?? (Is called in 3-2, 3-6, UC2)
- DKA DLC Final Boss Phase 2 RK  
and 2 more whose purpose is to be determined. 
  
  
Notably not included are:
- Animations (e.g. rabbid peach move, luigi sentry, ...)
- Color of garden ziggies
- Versus item box
- Action cam
- Coin spawn amount (on block hit)

	
# What the Code can and can not do so far  

It can:  
- Find out the rng state
- Manipulate the rng state to whatever we want
- Control behavior of anything rng related (see list of 30 functions) PROVIDED we now at least one rng state/value that has our desired outcome
- Control behavior of hit, crit and damage without that extra condition
(Here are some more that are included in the code, but not fully implemented for use: Random explosive cover, Coin direction, RK Wave block hits)  
  
It can not currently:  
- Control behavior of most other rng related things without that extra condition
  
  
  
# How it works:  
The programm will first try to figure out what RNG state your game is on.  
It does so by going through a series of tests where you can input damage values you got on shooting an enemy. Each damage value will give the programm more information about the current RNG value.  
To make sure this test goes as smoothly as possible avoid calling in an inconsistent amount.   
The best practice is to pick a set sequence of actions you repeat slowly and as closesly as possible.   
Do not move with Beep-O in battle, if it can be avoided. If you have to, go one tile at a time and not diagonal, otherwise it can call RNG in potentially unpredicatable ways.  
After inputting the damage value you got, restarting the battle and repeating enough times the code will give you a RNG state.  
You can then progress to the second part of the code, which will let you see predicted next damage values and how you can manipulate them to get a desired outcome.  
  
The way we can progress the RNG state in a controlled manner is by scrolling of the (non used) movement action onto an ability and back onto the movement action.   
This will progress RNG by exactly: 2 * Number of entities in range of used character (for each weapon)  
Your own team (but not the respective character) count as an entity as well in this context. 
  
  
  
# Step by step process:  
1. Start the programm
A binary is included that you can run in the terminal in case you don't have a cpp compiler)
2. Enter the Step Size
This refers to how many RNG steps you do in one cycle. You can manually count or use the patch file, which will count for you and display the amount of steps taken (since start of the game) as the min damage preview on reaction shots.
3. Enter the hit chance for the target you are shooting
4. Enter the stats of the weapon you are using

5. In game shot your target and input the damage value into the programm
Make sure these are the values before applying buffs/debuffs. A miss is represented with '-1'. 
6. In game restart the battle, shot your target again, input the damage value again, ... until the programm gives you your RNG state

7. Input 'x' to continue to the next part

8. You can now overwrite the RNG state
The programm will automatically input the RNG state that the game will use for the next shot (provided team and the steps taken until that shot are the same). You have the option to overwrite that value, if necessary.

9. You can now reenter some values if necessary. Additionally input MStep size.
MStep size refers to '2 * Number of entities in range of used character (for each weapon)' as described above.

10. The code will now give you the predicted damage values for your next shot, depending on how many times you scrolled of and back on to move.

11. Input how many times you did scroll off and back onto move, and repeat as often as desired.

