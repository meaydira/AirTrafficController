# AirTrafficController
An Air Traffic Controller Simulation using pThreads

Usage:
g++ main.cpp -o main ./main -s 30 -p 0.4 -n 10

This project has been prepared for COMP304 Course : Implementing an Air Traffic Contoller Simulation using POSIX threads.

About Air Traffic Controller

Air traffic control (ATC) is a control service to handle planes’ departing or landing by preventing collisions since only 1 plane at a time can use the runway.
When a plane comes to land or take-off, it gets in the queue waits for permission from ATC to complete its action.
So, the aim in this project was to create a simulator of planes and an ATC in which there is no collision or deadlock.
Implementation

Version 1
For the first part, my implementation had 3 major methods that has been run by pthreads. One method for landing planes, one method for departing planes and and one method for ATC. There are 2 queues in the first part, and TakeoffQueue.
I have defined this queues of type Plane*, since pushing to a queue doesn’t copy objects by reference and it led to problems when using cond and lock variables. So I decided to use Plane*.
A plane has id, arrival time, action (I have declared an enum actionType = {NORMAL,DEPART, EMERGENCY} ) , and priority (priority is not used until last part) .
The main method, creates first landing and departing planes, and then runs a loop until the end of simulation, which creates a landing thread with possibility p and a departing plane with the possibility of 1-p.
A landing function creates a plane, initializes its mutex and cond variables. If it’s the first plane , it notifies ATC thread and air traffic controller starts working. Then, the thread adds the plane to LandingQueue and waits signal from the ATC to proceed. When the signal comes, it records the runway, and turnaround time and adds to the String log.
This procedure is almost the same for Departing Planes.
To prevent race conditions, I have used mutexes for Queues, id counters and isFirst variable, and log. Also every plane has its own mutex and cond variables.

When it comes to ATC method, it checks if the LandingQueue is empty. If its not empty, it gives permission to the next landing plane, and if its empty, it gives permission to the next taking off plane.
Version 2- Solving Starvation for Departing Planes
Since version 1 caused starvation, I have just updated my ATC method and added a global threshold variable which is 5.
In this version, the ATC method gives permission to the next plane in TakeOff Queue if TakeoffQueue’s size is equal or greater than threshold, or the LandingQueue is empty.
This version causes starvation for the planes in the air. Let’s assume that the p is smaller than 0.5 , which means after a point TakeOff Queue’size will keep being longer than 5 planes and ATC will not let anymore Landing Planes and this situation causes starvation. Even if the p is greater than 0.5, there is a high chance that TakeOff Queue’size will keep being longer than 5 planes since ATC thread sleeps 2*t after each action and the main thread keeps generating new threads after sleeping t.

Version 3- Emergency Planes
In this version, there is Emergency Landing. For this part, I have added a new queue called Emergency Queue, when emergencyLand method is called, it creates a new plane and add it to EmergencyQueue.
And the ATC thread first checks EmergencyQueue, if its not empty, then it gives permission to the next plane in this queue for landing.

Version 4- Preventing Starvation
For this version, I have came up with a solution which introduces a new variable for planes, priority. Priority is an integer number which helps ATC to decide what to do next. Since we favor landing planes, when a new plane is added to the LandingQueue, it’s initial priority is 5. And for a take-off plane, its 1.
When ATC gives any plane the permission to land or take-off, all planes’ priorities in the LandindQueue, and TakeoffQueue are increased by 1.
When deciding, ATC first checks emergency queue, if its empty, it compares the first planes’ priorities in each queue and gives permission to the one with higher priority.
If their priorities are equal, it gives permission to the queue with greater size.
By this solution, no queue is exposed to starvation and according to the plane’s wait time, each queue is served.
