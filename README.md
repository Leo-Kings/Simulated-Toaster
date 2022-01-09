Leo King

Summary:
This is a simulated toaster oven working with state machines. I made a toaster that has 3 different modes that can
be flipped through with button 3. This used the SETUP state which let's you get to changing state which 
lets you change modes or time/temperature. After finishing setting up we can press button 4 to start cooking
depending on what mode, time, and temperature we originally were on. The important parts of the lab is
definitely the state machine. You needed to know when/why the states are called by the while loop. The 
events (Button 3 and 4 and the potentiometer) are processed to turn the enable the while loop to call the
state machine. The events are in the timers with the hertz being the tick per second. (Ex. 30hz is 30 ticks
per second)

Approach:
I need the basics of what a toaster oven can do, in this case, I simulated a bake, toast, and broil state.
Then I started implementing the basic structures and led. I also need to figure out how to write macro functions 
to how the oven would respond to event. I had a major problem writing the state machine with the switch function 
because I forgot to call break at the end of a case. I found out after 4 hours the reason my code was running 
through the states too quickly was because my selection was being called twice because after a case my code 
would go to the next case instead of returning to the while loop. The timer was difficult to implement because 
I didn't know the frequency of at which the microcontroller effectively takes as a second.


  
