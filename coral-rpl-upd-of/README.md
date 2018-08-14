Remember, that the OF (MORHOF2) works with etx, as soon as the color of the node's parents is not RED, or  both parents are RED. 
If one parent has red color it is chosen.
It is obvious that you can set/unset the color of the node in runtime, and this is where interesting things can happen.
You can also alter the algorithm inside MRHOF2 (or create a clone of MRHOF2) to support more colors, 
or do more complicated things.
Another option is to not use at all the etx value, and use the color as the weight. In this case you 
can "poison" certain roots, making the unpreferable. Instead roots without the "poisoned" stations will be chosen.
