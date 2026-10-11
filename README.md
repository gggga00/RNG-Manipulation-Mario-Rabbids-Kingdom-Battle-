# RNG-Manipulation-Mario-Rabbids-Kingdom-Battle-



This project is still work in progress  

  

# Basics

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

  

and 3 more whose purpose is to be determined. 

  

  

Notably not included are:

- Animations (e.g. Rabbid Peach move, Luigi sentry, ...)

- Color of Garden Ziggies

- Versus item box

- Action cam

- Coin spawn amount (on block hit)




# What the Code does





The code helps with 3 major actions:

1. Figuring out your current RNG state
2. Manipulating your RNG state to be a desired one
3. Creating RNG routes (currently not supported for the executable)


Quitting out of one with ‘x’ will put you in the next.

Inputting ‘?’ at any point will show you the current command list.



  

# 1. Figuring out your RNG state

The “STATE FINDER” mode can be used to do exactly that.

One way to figure out your state is by repeatedly shooting a target and tracking the damage values.

Input the stats of the weapon into the program. 
It will also ask for a ‘step count’. That refers to how many steps pass between each damage roll.

To make sure this test goes as smoothly as possible avoid calling RNG an inconsistent amount. The best practice is to pick a set sequence of actions you repeat slowly and as closesly as possible. Do not move with Beep-O in battle, if it can be avoided. If you have to, go one tile at a time and not diagonal, otherwise it can call RNG in potentially unpredicatable ways.  

Input the damage value, restart the battle, shoot again and repeat. (Make sure to exclude de-/buffs from the damage value. -1 refers to a miss)

After enough inputs the code will give you your RNG state. (With Yoshi’s base primary you will usually get it after 5 repetitions)


# 2. Manipulating RNG

The “MANIP HELPER” will help with that.

To figure out how many steps you need to reach your desired RNG state the program needs both your current RNG state and what qualifies as a desired state. Route files (which you can make as described in 3) will tell the program exactly what qualifies as a desired state. See the ‘Route Files’ folder for examples. The code can then tell you how many steps you need to reach the closest state that fulfills the requirements determined by the route files.

Alternatively to route files you can also search for damage values directly.

The steps needed may be substantial, depending on the type of RNG you are looking for. Progressing thousands of steps by just scrolling on and off the move action is not advised. 

Instead use different things that call RNG a lot to get close, then redetermine your state. From there you can move hit/crit check and damage roll calls from moving onto the move action to progress it a clearly set amount of steps. Specifically it will be '2 x Number of entities in range of used character (for each weapon)' steps.

In the program you can manually progress the state listed, to better keep track of your current state.


# 3. Routing

This is currently only supported within the code itself. It provides various functions for state to value and step count calculations.
Make a uint32_t vector, which will contain the valid states for your route. Pass it through the various functions to limit the entries to fitting states, progress the entries by a various amount of steps, and more. Use the SaveVector() function to turn the vector into a route file. This will also sort it, which is necessary for the manip code.


# Step by step example for manipulating rng

1. Start the programm  
Binaries are included that you can run in the terminal in case you don't have a cpp compiler. They can be found in the assets of the respective release, but I recommend working with the actual c++ code instead, as it is updated more frequently.

2. Enter the Step Size
This refers to how many RNG steps you do in one cycle. You can manually count or use the various patch files to help count for you.

3. Enter the hit chance for the target you are shooting

4. Enter the stats of the weapon you are using

5. In game shot your target and input the damage value into the programm
Make sure these are the values before applying buffs/debuffs. A miss is represented with '-1'. 

6. In game restart the battle, shot your target again, input the damage value again, ... until the programm gives you your RNG state

7. Input 'x' to continue to the next part

8. Input the current the RNG state.
The program will automatically input the currently determined RNG state. You have the option to overwrite that, if necessary.

9. You can now reenter some more values if necessary. If you can’t progress steps one at a time increase Msteps to a number of steps you can progress.

10. Select the route mode you want to go for. If you used ‘Search for goal states’ input the name of the route file you want to use for it (make sure it is located in the same folder). 

11a. If you used ‘Search for damage’, the code will give you the predicted damage values for your next shot, depending on how many times you scrolled of and back on to move.

11b. If you used ‘Search for goal states’  the code will give you the next desired rng state you can reach.
