/*
 * DİLARA BOZYILAN
 * 59748
 * COMP304 - P2
 * Air Traffic Controller Implementation with Pthreads
 * */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <queue>
#include <sys/time.h>
#include <fstream>

using namespace std;
double simulation_time= 60 ;
float P = 0.5;
int t =1 ;
double report_time = 20;
static int even_id= 0;
static int odd_id= 1;
time_t simulation_start; //keeps the simulation begin time.
int threshold = 5;   //This variable is for part two.
static time_t simulation_end;
string Logs = "      ID    Type       Request T.           Runway T.       Turnaround T.\n-----------------------------------------------------------------------------\n";

enum actionType{DEPART = 0, EMERGENCY = 1, NORMAL = 2};

struct Plane
{
    /*
     * a Plane has an id, cond and lock variables
     * and action type (landing, departing or emergency landing)
     * arrival which keeps the time when plane is created
     * arrival_time = arrival - simulation_start
     * runway time = when plane completes its action - simulation start
     * turnaround time = when plane completes its action - arrivaş
     * also a priority number, which keeps the priority of the plane in the queue (not used for part 1 and 2)
     * */
    int id;
    double arrival_time;
    pthread_cond_t cond;
    pthread_mutex_t lock;
    actionType action;
    int priority;
};


//This method is part of my optimized version to prevent starvation
void increasePriorities(queue<Plane *> queue);

//id mutex for readint even_id and odd_id
pthread_mutex_t id_mutex;

//queues and their mutexes
queue<Plane*> LandingQ;
queue<Plane*> TakeoffQ;
queue<Plane*> EmergencyQ;

pthread_mutex_t landingQueue_mutex;
pthread_mutex_t takeoffQueue_mutex;
pthread_mutex_t emergencyQueue_mutex;

//Cond and Lock Variables
pthread_cond_t firstPlaneCond;
pthread_mutex_t firstPlaneLock;

//mutex for log
pthread_mutex_t log_mutex;

//this method is given.
int pthread_sleep (int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;

    pthread_mutex_lock (&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock (&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    return res;

}

//prints queue and its elements
void printQueue(queue<Plane*> original){
    queue<Plane*> temp = original;

    while(!temp.empty()){
        printf("%d <--", (*temp.front()).id);
        temp.pop();
    }

    printf("\n");
}


//gets queue elements as a string
string getQueueDisplay(queue<Plane*> original){
    string res;
    char result[10000];
    queue<Plane*> temp = original;

    while(!temp.empty()){ sprintf(result,"%d <--", (*temp.front()).id);
        res+= result;
        temp.pop();
    }

    sprintf(result,"\n");
    res+= result;

    return res;
}

//prints all elements with their priority number in a queue
void printQueuePriorities(queue<Plane*> original){
    queue<Plane*> temp = original;

    while(!temp.empty()){
        printf("%d <--", (*temp.front()).priority);
        temp.pop();
    }

    printf("\n");
}

void* emergencyLand(void *s){
    //create plane and initialize variables
    struct Plane plane;
    //gives its id
    pthread_mutex_lock(&id_mutex);
    plane.id = even_id;
    even_id+=2;
    pthread_mutex_unlock(&id_mutex);


    //init mutex and cond
    pthread_mutex_init(&plane.lock, NULL);
    pthread_cond_init(&plane.cond, NULL);


    //set action type and priority
    plane.action = EMERGENCY;
    plane.priority = 5;

    //record arrival
    double arrival = time(NULL);
    plane.arrival_time = (arrival - simulation_start);

    //push to the queue
    EmergencyQ.push(&plane);

    //wait for signal from ATC
    pthread_mutex_lock(&plane.lock);
    pthread_cond_wait(&plane.cond, &plane.lock);
    pthread_mutex_unlock(&plane.lock);

    //record runway and turnaround time
    time_t current = time(NULL);
    double runway_t = current - simulation_start;
    double turnaround_t = current - arrival;

    //add to the log string
    char log[1000];
    sprintf(log, "     %3d      %c         %4.0f                %4.0f               %4.0f\n", plane.id, 'E', plane.arrival_time, runway_t, turnaround_t);
    pthread_mutex_lock(&log_mutex);
    Logs+=string(log);
    pthread_mutex_unlock(&log_mutex);

    //Destroy mutex and cond
    pthread_mutex_destroy(&plane.lock);
    pthread_cond_destroy(&plane.cond);

    //exit
    pthread_exit(NULL);
}

void* land(void *action){

    struct Plane plane;

    //init id
    pthread_mutex_lock(&id_mutex);
    plane.id = even_id;
    even_id+=2;
    pthread_mutex_unlock(&id_mutex);

    //init mutex and cond
    pthread_mutex_init(&plane.lock, NULL);
    pthread_cond_init(&plane.cond, NULL);

    //init other variables
    plane.action = NORMAL;
    plane.priority = 5;

    //record time
    double arrival = time(NULL);
    plane.arrival_time = (arrival - simulation_start);

    //add to the queue
    LandingQ.push(&plane);

    //if its the first plane, notify ATC
    if(plane.id == 0 && TakeoffQ.empty()){
        printf("Notifyin ATC \n");
        pthread_mutex_lock(&firstPlaneLock);
        pthread_cond_signal(&firstPlaneCond);
        pthread_mutex_unlock(&firstPlaneLock);
    }
    /*
    if(isFirst ){
        isFirst = false;
        printf("Notifyin ATC \n");
        pthread_mutex_lock(&firstPlaneLock);
        pthread_cond_signal(&firstPlaneCond);
        pthread_mutex_unlock(&firstPlaneLock);
    }
     */


    //wait for signal from ATC
    pthread_mutex_lock(&plane.lock);
    pthread_cond_wait(&plane.cond, &plane.lock);
    pthread_mutex_unlock(&plane.lock);
    //  printf("Plane with id:  %d is landing. \n", plane.id);


    //Record runway and turnaround
    time_t current = time(NULL);
    double runway_t = current - simulation_start;
    double turnaround_t = current - arrival;

    //add to the log string
    char log[1000];
    sprintf(log, "     %3d      %c         %4.0f                %4.0f               %4.0f\n", plane.id, 'L', plane.arrival_time, runway_t, turnaround_t);
    pthread_mutex_lock(&log_mutex);
    Logs+=string(log);
    pthread_mutex_unlock(&log_mutex);

    //destroy mutex and cond
    pthread_mutex_destroy(&plane.lock);
    pthread_cond_destroy(&plane.cond);

    //Exit
    pthread_exit(NULL);
}


void* take_off(void *s){


    struct Plane plane;

    //init mutexes
    pthread_mutex_init(&plane.lock, NULL);
    pthread_cond_init(&plane.cond, NULL);


    //init id and variables
    pthread_mutex_lock(&id_mutex);
    plane.priority = 1;
    plane.id = odd_id;
    odd_id+=2;
    pthread_mutex_unlock(&id_mutex);
    plane.action = DEPART;

    //record time
    time_t arrival = time(NULL);
    plane.arrival_time = arrival - simulation_start;
  //  printf("Arrival time: %f   %f \n", arrival, plane.arrival_time );



    //add to the queue
    TakeoffQ.push(&plane);

    //if its the firstPlane, notify ATC
    if(plane.id == 1 && LandingQ.empty()){
        printf("Notifyin ATC \n");
        pthread_mutex_lock(&firstPlaneLock);
        pthread_cond_signal(&firstPlaneCond);
        pthread_mutex_unlock(&firstPlaneLock);
    }

   // printf("Plane with id:  %d is waiting for signal from ATC. \n", plane.id);
  //  printf("Waiting address: %d \n", &plane.cond);

  //Wait for signal
        pthread_mutex_lock(&plane.lock);
        pthread_cond_wait(&plane.cond, &plane.lock);
        pthread_mutex_unlock(&plane.lock);

  //  printf("Plane with id:  %d is departing. \n", plane.id);

  //record runway and turnaround
    time_t current = time(NULL);
    double runway_t = current - simulation_start;
    double turnaround_t = current - arrival;



    //add log
    char log[1000];
    sprintf(log, "     %3d      %c         %4.0f                %4.0f               %4.0f\n", plane.id, 'D', plane.arrival_time, runway_t, turnaround_t);
    pthread_mutex_lock(&log_mutex);
    Logs+=string(log);
    pthread_mutex_unlock(&log_mutex);

    //destroy mutex and cond

    pthread_mutex_destroy(&plane.lock);
    pthread_cond_destroy(&plane.cond);

    pthread_exit(NULL);
}



void* handleTakeOff(){
    /*
     * This method is called by ATC, it takes the front plane of TakeOffQueue and signal's its cond variable
     * And removes the plane from queue
     * */


    if(TakeoffQ.empty()){
        printf("Takeoff Queue is Empty \n");
    }else{
        pthread_mutex_lock(&takeoffQueue_mutex);
        Plane* next = TakeoffQ.front();
        TakeoffQ.pop();
        pthread_mutex_lock(&takeoffQueue_mutex);

      //  printf("ATC: Signaling plane with id:  %d ", (*next).id);
      //  printf("Signaling address: %d \n", &((*next).cond));
        pthread_mutex_lock(&(*next).lock);
        pthread_cond_signal(&(*next).cond);
        pthread_mutex_unlock(&(*next).lock);
    }
}


void landEmergency(){
    /*
 * This method is called by ATC, it takes the front plane of EmergencyQueue and signal's its cond variable
 * And removes the plane from queue
 * */
    if(EmergencyQ.empty()){
        printf("Emergency Queue is Empty \n");
    }else{
        pthread_mutex_lock(&emergencyQueue_mutex);
        Plane* next = EmergencyQ.front();
        EmergencyQ.pop();
        pthread_mutex_lock(&emergencyQueue_mutex);

        pthread_mutex_lock(&(*next).lock);
        pthread_cond_signal(&(*next).cond);
        pthread_mutex_unlock(&(*next).lock);
    }
}

void landNormal(){
    /*
    * This method is called by ATC, it takes the front plane of LandingQueue and signal's its cond variable
    * And removes the plane from queue
    * */
    if(LandingQ.empty()){
        printf("Landing Queue is Empty \n");
    }else{
        pthread_mutex_lock(&landingQueue_mutex);
        Plane* next = LandingQ.front();
        LandingQ.pop();
        pthread_mutex_lock(&landingQueue_mutex);

     //   printf("ATC: Signaling plane with id:  %d ", (*next).id);
      //  printf("Signaling address: %d \n", &((*next).cond));
        pthread_mutex_lock(&(*next).lock);
        pthread_cond_signal(&(*next).cond);
        pthread_mutex_unlock(&(*next).lock);
    }
}

void increasePriorities(queue<Plane *> q) {
    /*
     *This method is used in part4, to prevent starvation.
     * This method takes a queue and increases every plane element's priority by 1 in that queue.
     */
    queue<Plane *> temp = q;
    while(!temp.empty()){
        (*temp.front()).priority++;
        temp.pop();
    }
}

/**ATC METHOD FOR PART 1 **/
void* air_traffic_control_part1(void *s){

    //init mutex and cond variable for first plane
    pthread_mutex_init(&firstPlaneLock, NULL);
    pthread_cond_init(&firstPlaneCond, NULL);
    //TODO: should these be initalized in here or main thread?

    //waits for signal from firstPlane
    printf("ATC is waiting \n");
    pthread_cond_wait(&firstPlaneCond, &firstPlaneLock);
    printf("Waiting is over lets go \n");

    //until simulation ends
    while( time(NULL) <= simulation_end ){

        //if landing queue is empty, depart next plane. If not, give permission to the next landing plane.
        if(LandingQ.empty()){
            handleTakeOff();
        }else{
           landNormal();

        }
        //sleep for 2t
        pthread_sleep(2*t);
    }

    //exit
    pthread_exit(NULL);
}

/**ATC METHOD FOR PART 2 **/
void* air_traffic_control_part2(void *s){

    //init mutex and cond variable for first plane
    pthread_mutex_init(&firstPlaneLock, NULL);
    pthread_cond_init(&firstPlaneCond, NULL);

    //waits for signal from firstPlane
    printf("ATC is waiting \n");
    pthread_cond_wait(&firstPlaneCond, &firstPlaneLock);
    printf("Waiting is over lets go \n");


    while( time(NULL) <= simulation_end ){


        //if landing queue is empty OR there are more than threshold planes in the TakeoffQueue:
        // depart next plane.
        // If not, give permission to the next landing plane.
        if(LandingQ.empty() || TakeoffQ.size() >= threshold){
            handleTakeOff();
        }else{
            landNormal();
        }

        //sleep for 2t
        pthread_sleep(2*t);

    }


    //exit
    pthread_exit(NULL);
}



void* air_traffic_control_part3(void *s){


    //init mutex and cond variable for first plane
    pthread_mutex_init(&firstPlaneLock, NULL);
    pthread_cond_init(&firstPlaneCond, NULL);

    //waits for signal from firstPlane
    printf("ATC is waiting \n");
    pthread_cond_wait(&firstPlaneCond, &firstPlaneLock);
    printf("Waiting is over lets go \n");

    //until the end of simulation
    while( time(NULL) <= simulation_end ){

        //first check for emergency planes.
        if(EmergencyQ.size() != 0){
            printf("Landing Emergency \n");
            landEmergency();
        }else if( !LandingQ.empty() && TakeoffQ.size() < threshold){
            landNormal();
        }else{
            handleTakeOff();
        }

        //sleep for 2t
        pthread_sleep(2*t);

    }


    pthread_exit(NULL);
}



 void* air_traffic_control(void *s){

     //printf("ATC is waiting \n");
     pthread_mutex_init(&firstPlaneLock, NULL);

     pthread_cond_init(&firstPlaneCond, NULL);
     printf("ATC is waiting \n");
     pthread_cond_wait(&firstPlaneCond, &firstPlaneLock);
     printf("Waiting is over lets go \n");
     while( time(NULL) <= simulation_end ){

         /**
          * First check for emergency queue, then, if any queue is empty, give permission to other queue.
          * If neither queue is empty, then check for priorities of the first elements of both queues.
          * If TakeoffQueue.firstElement.priority > LandingQueue.firstElement.priority
          * depart next plane and increase priorities in both queues.
          * If TakeoffQueue.firstElement.priority == LandingQueue.firstElement.priority
          * give permission to the next plane with longer queue size.
          * and increase priorities in both queues
          * sleep for 2t secs
          *
          */
         if(!EmergencyQ.empty()){
                landEmergency();
         }else if(LandingQ.empty()){
             handleTakeOff();
         }else if(TakeoffQ.empty()){
             landNormal();
         }else if((*TakeoffQ.front()).priority > (*LandingQ.front()).priority ){

             handleTakeOff();

             //increase priorities in both queues.
             increasePriorities(LandingQ);
             increasePriorities(TakeoffQ);
         }else if((*TakeoffQ.front()).priority == (*LandingQ.front()).priority && TakeoffQ.size() > LandingQ.size()){

             handleTakeOff();
             increasePriorities(LandingQ);
             increasePriorities(TakeoffQ);

         }else{
            // printf("ATC: Landing normal plane \n");
             landNormal();
             increasePriorities(LandingQ);
             increasePriorities(TakeoffQ);
         }
         pthread_sleep(2*t);

         }


     pthread_exit(NULL);
}


int main(int argc, char *argv[]){

    /*
     *
     * The main method method parses command line arguments
     * initializes mutexes and conds
     * creates ATC thread and first landing-departing planes
     * while simulation time, it creates new plane threads according to possibility P,
     * and then prints the logs
     * prints the queues
     *
   */

    pthread_t landingPlane, takeOffPlane;  //first departing and landing plane threads
    int landing_thread,taking_off_thread;
    pthread_t air_traffic_control_thread;
    void *pointer;
    float number;
    int time_counter = 0;

    //parse command line arguments

    if(string(argv[1]) == "-s"){
        simulation_time = atoi(argv[2]);
        printf("Simulation time : %f " ,simulation_time);
    }

    if(string(argv[3]) == "-p"){
        P = atof(argv[4]);
        printf("Prob: : %f " ,P);
    }

    if(string(argv[5]) == "-n"){
        report_time = atoi(argv[6]);
        printf("Report time : %f " ,report_time);
    }


    //record simulation start and end
    simulation_start = time(NULL);  //get time and store it in simulation_start to use when needed
    simulation_end = simulation_start + simulation_time; //for while loop

     //create ATC thread
     /**
      * If you change the method name below to
      * air_traffic_control_part1,air_traffic_control_part2 or air_traffic_control_part3
      * You can see the results for previous versions
      *
      */
     pthread_create(&air_traffic_control_thread, NULL, air_traffic_control, (void *)pointer);

     //create landing and takeoff threads
     landing_thread = pthread_create(&landingPlane, NULL, land, (void *)pointer);
     taking_off_thread = pthread_create(&takeOffPlane, NULL, take_off, (void *)pointer);

         //print error if there is an error while creating threads.
         if (landing_thread) {
             printf("Error:unable to create landing thread ,");
            exit(-1);
         }

         //print error if there is an error while creating threads.
         if (taking_off_thread) {
             printf("\n Error:unable to create taking off thread \n");
              exit(-1);
           }



         //until simulation ends
    while(time(NULL) < simulation_end){

        //create a random number between 0-1
        number = (float)rand()/((float)RAND_MAX);

       if(time_counter != 0 && time_counter % 40 == 0 ){
           pthread_t emergency;
           pthread_create(&emergency, NULL, emergencyLand, (void *)pointer);
       }else if(number <= P){
            pthread_t landing;
            pthread_create(&landing, NULL, land, (void *)pointer);

        }else{
            pthread_t taking_off;
            pthread_create(&taking_off, NULL, take_off, (void *)pointer);

        }

          if(time_counter >= report_time){
              printf("At %d secs in theground: \n", time_counter);
              printQueue(TakeoffQ);
              printf("At %d secs in the air: \n", time_counter);
              printQueue(LandingQ);
          }

          time_counter++;

          pthread_sleep(1);
    }


    //join ATC thread
   // pthread_join(air_traffic_control_thread, &pointer);


    //print logs and queues
    printf("%s \n",Logs.c_str());

    printf("Landing Queue : \n");
    printQueue(LandingQ);
    printf("Departing Queue : \n");
    printQueue(TakeoffQ);

   /* printf("Landing Queue : \n");
    printQueuePriorities(LandingQ);
    printf("Departing Queue : \n");
    printQueuePriorities(TakeoffQ); */


   //print logs to Logs.out
    ofstream out("Logs.out");

    out << "Results with simulation time : " <<simulation_time << ", p = "<< P<< " and n = " << report_time <<"\n";
    out << Logs;
    out << "Landing Queue : \n";
    out << getQueueDisplay(LandingQ);
    out << "Departing Queue : \n";
    out << getQueueDisplay(TakeoffQ);
    out.close();

    exit(0);

}
