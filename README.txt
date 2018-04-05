 /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
/                                \
\  /\/\/\/\  /\    /\  /\/\/\/\  /
/  \/\/\/\/  \/    \/  \/\/\/\/  \
\  /\        /\    /\     \/     /
/  \/        \/    \/     /\     \
\  /\/\/\    /\    /\     \/     /
/  \/\/\/    \/    \/     /\     \
\  /\         /\  /\      \/     /
/  \/         \/\/\/      /\     \
\  /\/\/\/\    \/\/    /\/\/\/\  /
/  \/\/\/\/     \/     \/\/\/\/  \
\                                /
 \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

This program simulates a grid where robots reproduce and evolve. Their behavior
is dictated by their program in the form of code for a virtual machine. The code
is the main subject of evolution. One instruction is executed per turn. The goal
of the robots is to collect the three naturally occurring chemicals (red, green,
and blue) to process into useful chemicals. These synthetic chemicals are used
to survive and (asexually) reproduce.

Command Line Interface
----------------------

Use this format to run evi:
<executable> <mode> <visual?> <ticks>
executable is ./evi usually
mode is 'r' or 'w'. r mode reads from a save then continually writes to it every
<ticks> ticks during simulation. w mode writes to a new save after simulating
the world associated for <ticks> ticks, then exits.
visual? is either 'y' indicating true or any other value to indicate false. when
it is true, the world is drawn every tick and the simulation pauses for a bit.

To cancel the simulation, press CTRL+C. The simulation will finish cycling for
the number of ticks given at the beginning then will exit. At the end of the
simulation, code for every living species with nine or more members is dumped
into stderr.


Example: 
 ./evi r y world 1000 2> species.log

Note: I'll make a better interface later.
