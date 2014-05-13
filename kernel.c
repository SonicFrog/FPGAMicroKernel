#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "kernel.h"
#include "system_m.h"
#include "interrupt.h"

// Maximum number of semaphores.
#define MAXSEMAPHORES 10
// Maximum number of processes.
#define MAXPROCESS 10

#define MAX_MONITORS 10

#define MAX_EVENTS 10

typedef struct {
    int next;
    Process p;
    int monitors[MAX_MONITORS];
} ProcessDescriptor;

typedef struct {
    int waitingList;
    int readyList;
    bool locked;
} MonitorDescriptor;

typedef struct {
    int waitingList;
    bool happened;
} EventDescriptor;

typedef struct {
    int n;
    int waitingList;
} SemaphoreDescriptor;


// Global variables

// Pointer to the head of list of ready processes
int readyList = -1;

// list of process descriptors
ProcessDescriptor processes[MAXPROCESS];
int nextProcessId = 0;

// list of semaphore descriptors
SemaphoreDescriptor semaphores[MAXSEMAPHORES];
int nextSemaphoreId = 0;

/***********************************************************
 ***********************************************************
            Utility functions for list manipulation
************************************************************
* **********************************************************/

// add element to the tail of the list
void addLast(int* list, int processId) {
    if(processId == -1)
    {
        return;
    }
    if (*list == -1){
        // list is empty
        *list = processId;
    }
    else {
        int temp = *list;
        while (processes[temp].next != -1){
            temp = processes[temp].next;
        }
        processes[temp].next = processId;
        processes[processId].next = -1;
    }

}

// add element to the head of list
void addFirst(int* list, int processId){
    if(processId)
    {
        return;
    }
    if (*list == -1){
        *list = processId;
    }
    else {
        processes[processId].next = *list;
        *list = processId;
    }
}

// remove element that is head of the list
int removeHead(int* list){
    if (*list == -1){
        printf("List is empty!");
        return(-1);
    }
    else {
        int head = *list;
        int next = processes[*list].next;
        processes[*list].next = -1;
        *list = next;
        return head;
    }
}

// returns head of the list
int head(int* list){
    if (*list == -1){
        printf("List is empty!\n");
        return(-1);
    }
    else {
        return *list;
    }
}

/***********************************************************
 ***********************************************************
                    Kernel functions
************************************************************
* **********************************************************/

void createProcess (void (*f)(), int stackSize) {
    if (nextProcessId == MAXPROCESS){
        printf("Error: Maximum number of processes reached!\n");
        exit(1);
    }

    Process process;
    int* stack = malloc(stackSize);
    process = newProcess(f, stack, stackSize);
    processes[nextProcessId].next = -1;
    processes[nextProcessId].p = process;

    memset(processes[nextProcessId].monitors, -1, MAX_MONITORS);
    // add process to the list of ready Processes
    addLast(&readyList, nextProcessId);
    nextProcessId++;

}


void yield(){
    int pId = removeHead(&readyList);
    addLast(&readyList, pId);
    Process process = processes[head(&readyList)].p;
    transfer(process);
}

int createSemaphore(int n){
    if (nextSemaphoreId == MAXSEMAPHORES){
        printf("Error: Maximum number of semaphores reached!\n");
        exit(1);
    }
    semaphores[nextSemaphoreId].n = n;
    semaphores[nextSemaphoreId].waitingList = -1;
    return nextSemaphoreId++;
}

void P(int s){

    semaphores[s].n =  semaphores[s].n - 1;
    if (semaphores[s].n < 0){
        int p = removeHead(&readyList);
        addLast(&(semaphores[s].waitingList), p);
        Process process = processes[head(&readyList)].p;
        transfer(process);
    }
}

void V(int s){
    semaphores[s].n =  semaphores[s].n + 1;
    if (semaphores[s].n <= 0){
        int p =  removeHead(&(semaphores[s].waitingList));
        addLast(&readyList, p);
    }
}

void start(){

    fprintf(stderr, "Starting kernel...\n");
    if (readyList == -1){
        fprintf(stderr, "Error: No process in the ready list!\n");
        exit(1);
    }
    Process process = processes[head(&readyList)].p;
    transfer(process);
}


/**
 * Monitor related kernel functions
 **/

MonitorDescriptor monitors[MAX_MONITORS];
int nextMonitorID = 0;

/**
 * Returns the id of the last monitor the given process entered
 * If the given process isn't in any monitors the return id is -1
 **/
int getLastMonitorId(ProcessDescriptor p)
{
    int i = 0;

    while(p.monitors[i] != -1 && i < MAX_MONITORS)
    {
        i++;
    }

    return p.monitors[i - 1];
}

/**
git  * Returns true if p is in the monitor identified by mid
 **/
bool hasMonitor(ProcessDescriptor p, int mid)
{
    size_t i = 0;

    for(i = 0 ; i < MAX_MONITORS ; i++)
    {
        if(p.monitors[i] == mid)
            return true;
    }
    return false;
}

/**
 * Adds the given monitor to the monitor list of the given process
 **/
void pushMonitor(int pid, int mid)
{
    size_t i = 0;
    ProcessDescriptor proc = processes[pid];

    if(pid == -1 || mid == -1)
        return;

    while(proc.monitors[i] != -1 && i < MAX_MONITORS)
    {
        i++; //Looking for the first -1 in monitor ids
    }

    if(i == MAX_MONITORS) //No space left to push monitor
    {
        fprintf(stderr, "Error: This process is in too much monitors\n");
        exit(1);
    }
    proc.monitors[i] = mid;
}

/**
 * Removes the last monitor added to the given process
 * and return the id of the monitor we just exited
 **/
int popMonitor(int pid)
{
    size_t i = 0;
    ProcessDescriptor* proc = &processes[pid];
    int ret = -1;

    if(pid == -1)
        return -1;

    while(proc->monitors[i] != -1 && i < MAX_MONITORS)
    {
        i++;
    }

    if(i == 0) //No monitor to pop
    {
        return -1;
    }

    ret = proc->monitors[i - 1];
    proc->monitors[i - 1] = -1;
    return ret;
}

int createMonitor()
{
    fprintf(stderr, "Creating monitor %d\n", nextMonitorID + 1);

    if(nextMonitorID == MAX_MONITORS)
    {
        fprintf(stderr, "Error: No more monitors available\n");
        exit(1);
    }
    monitors[nextMonitorID].waitingList = -1;
    monitors[nextMonitorID].readyList = -1;
    monitors[nextMonitorID].locked = false;
    return nextMonitorID++;
}

/**
 * Not checking if the process is already in the monitor
 * It's the programmer's responsibility to not enter the same monitor
 * twice and avoid deadlocking ?
 **/
void enterMonitor(int monitorID)
{
    fprintf(stderr, "Entering monitor %d\n", monitorID);
    if(monitorID >= nextMonitorID)
    {
        fprintf(stderr, "Error: using invalid monitor!!\n");
        exit(1);
    }

    bool hasLock = false;
    int p = head(&readyList);
    MonitorDescriptor *desc = &monitors[monitorID];
    
    hasLock = hasMonitor(processes[p], monitorID);

    pushMonitor(p, monitorID); //Add the monitor to the process's list

    if(desc->locked && !hasLock) 
    {
        //Transfer control if the monitor is locked and it's another
        //process that has the lock
        removeHead(&readyList);
        addLast(&(desc->readyList), p);
        transfer(processes[head(&readyList)].p);
    }
    else //Else lock it for ourselves
    {
        desc->locked = true;
    }
}

void wait() {
    int mid = getLastMonitorId(processes[head(&readyList)]);
    int p = removeHead(&readyList); //Get the current PID
    MonitorDescriptor *desc = &monitors[processes[p].monitors[mid]];
    int pid = head(&(desc->readyList)); //Get the PID to wake up

    fprintf(stderr, "%d waiting on %d\n", p, mid);

    addLast(&desc->waitingList, p);

    if(pid == -1) //No one can execute on this monitor
    {
        desc->locked = false;
    }
    else //Wake the first ready process for this monitor
    {
        addLast(&readyList, removeHead(&(desc->readyList)));
    }

    transfer(processes[head(&readyList)].p);
}

void notify() {
    int mid = getLastMonitorId(processes[head(&readyList)]);

    fprintf(stderr, "Notifying %d\n", mid);

    if(mid == -1)
    {
        fprintf(stderr, "A process that was in no monitor notified\n");
        exit(1);
    }
    
    //Put the first process in the waiting list of this monitor
    //in the ready list of the monitor
    addLast(&monitors[mid].readyList, removeHead(&(monitors[mid].waitingList)));
}

void notifyAll() {
    int mid = getLastMonitorId(processes[head(&readyList)]);

    fprintf(stderr, "Notifying all on %d\n", mid);

    if(mid == -1)
    {
        fprintf(stderr, "A process that was in no monitor notified a monitor\n");
        exit(1);
    }

    MonitorDescriptor *desc = &monitors[mid];

    while(head(&desc->waitingList) != -1)
    {
        addLast(&desc->readyList, removeHead(&desc->waitingList));
    }
}

void exitMonitor() {
    ProcessDescriptor desc = processes[head(&readyList)];
    int mid = popMonitor(head(&readyList));
    MonitorDescriptor *mon;

    fprintf(stderr, "%d exiting monitor %d\n", head(&readyList), mid);

    if(mid == -1)
    {
        fprintf(stderr, "A process that was in no monitors exited a monitor\n");
        exit(1);
    }

    mon = &monitors[mid];

    if(mon->readyList == -1 && !hasMonitor(desc, mid)) //If no process is ready to run
    {
        mon->locked = false; //Unlock the monitor
    }
    else if(!hasMonitor(desc, mid)) //We don't have the monitor
    {
        //Put the first ready to run process in the kernel queue
        addLast(&readyList, removeHead(&(mon->readyList)));
    }
    //If we still have the monitor we don't need to do anything
}

/**
 * Event related kernel functions
 **/

// list of event descriptors
EventDescriptor events[MAX_EVENTS];
int nextEventID = 0;

int createEvent(){ 
    // We check if we haven't reached the max amount of events yet
    if(nextEventID == MAX_EVENTS)
    {
        fprintf(stderr, "Error: No more events available\n");
        exit(EXIT_FAILURE);
    }
    events[nextEventID].waitingList = -1; // no process are waiting yet
    events[nextEventID].happened = false; // event hasn't happened yet
    return nextEventID++; // return the ID of the newly created event
}

void attendre(int eventID){
    // we check if the given ID is valid
    if(eventID >= nextEventID)
    {
        fprintf(stderr, "Error: using invalid event!!\n");
        exit(EXIT_FAILURE);
    }

    EventDescriptor *event = &events[eventID];

    // if the event hasn't happened yet we queue the process in the event and transfer control
    if(!event->happened)
    {
        int temp = removeHead(&readyList);
        addLast(&(event->waitingList), temp);
        transfer(processes[head(&readyList)].p);
    }
}

void declencher(int eventID){
    // we check if the given ID is valid
    if(eventID >= nextEventID)
    {
        fprintf(stderr, "Error: using invalid event!!\n");
        exit(EXIT_FAILURE);
    }

    EventDescriptor *event = &events[eventID];

    // we move all the process waiting on the event in the readylist of our kernel
    while(head(&event->waitingList) != -1)
    {
        addLast(&readyList, removeHead(&event->waitingList));
    }
}

void reinitialiser(int eventID){
    // we check if the given ID is valid
    if(eventID >= nextEventID)
    {
        fprintf(stderr, "Error: using invalid event!!\n");
        exit(EXIT_FAILURE);
    }

    EventDescriptor *event = &events[eventID];
    // we reinitialize the state of the given event
    event->happened = false;
}
