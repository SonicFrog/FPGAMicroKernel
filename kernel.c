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
    int m_sp;
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
    if(processId == -1)
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
    unsigned int* stack = malloc(stackSize);
    process = newProcess(f, stack, stackSize);
    processes[nextProcessId].next = -1;
    processes[nextProcessId].p = process;
    processes[nextProcessId].m_sp = 0;

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

void start(){

    printf("Starting kernel...\n");
    if (readyList == -1){
        printf("Error: No process in the ready list!\n");
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
 * Returns true if the pid has entered into the monitor with id mid at
 * least once
 **/
bool hasMonitor(int pid, int mid)
{
    int i;
    for(i = 0 ; i < MAX_MONITORS ; i++)
    {
        if(processes[pid].monitors[i] == mid)
        {
            return true;
        }
    }
    return false;
}

int peekMonitor(ProcessDescriptor *p)
{
	if(p->m_sp == 0) {
		return -1;
	}
    return p->monitors[p->m_sp - 1];
}

int popMonitor(ProcessDescriptor *p)
{
    if(p->m_sp == -1) //Nothing to pop
    {
        return -1;
    }
    return p->monitors[--(p->m_sp)];
}

void pushMonitor(ProcessDescriptor *p, int monitorId)
{
    if(p->m_sp == MAX_MONITORS)
    {
        
        exit(1);
    }
    p->monitors[p->m_sp++] = monitorId;
}

bool isInMonitor(ProcessDescriptor *p, int monitorId)
{
    int i;
    for(i = p->m_sp ; i >= 0 ; i--)
    {
        if(p->monitors[i] == monitorId)
        {
            return true;
        }
    }
    return false;
}


int createMonitor()
{
    if(nextMonitorID >= MAX_MONITORS)
    {
        exit(1);
    }

    monitors[nextMonitorID].waitingList = -1;
    monitors[nextMonitorID].readyList = -1;
    monitors[nextMonitorID].locked = false;

    return nextMonitorID++;
}

void enterMonitor(int monitorId)
{
    ProcessDescriptor *proc = &(processes[head(&readyList)]);

    bool alreadyLocked;

    if(monitorId < 0 || monitorId >= nextMonitorID)
    {
        exit(1);
    }

    alreadyLocked = isInMonitor(proc, monitorId);
      
    pushMonitor(proc, monitorId);

    if(!alreadyLocked) //Si on n'a pas déjà le lock sur ce moniteur
    {
        if(monitors[monitorId].locked) //Et qu'il est déjà lock ailleurs
        {
            
            addLast(&(monitors[monitorId].readyList), removeHead(&readyList));
            transfer(processes[head(&readyList)].p);
        }
        else
        {
            monitors[monitorId].locked = true;
        }
    }
}

void wait()
{
    ProcessDescriptor *proc = &(processes[head(&readyList)]);
    int monitorId = peekMonitor(proc);

    if(monitorId == -1)
    {
        exit(1);
    }

    if(head(&(monitors[monitorId].readyList)) == -1)
    {
        monitors[monitorId].locked = false;
    }

    addLast(&(monitors[monitorId].waitingList), removeHead(&readyList));
    addLast(&readyList, removeHead(&(monitors[monitorId].readyList)));

    transfer(processes[head(&readyList)].p);
}

void notify()
{
    ProcessDescriptor *proc = &(processes[head(&readyList)]);
    int monitorId = peekMonitor(proc);

    if(monitorId == -1)
    {
        exit(1);
    }

    addLast(&(monitors[monitorId].readyList),
            removeHead(&(monitors[monitorId].waitingList)));
}

void notifyAll()
{
    ProcessDescriptor *proc = &(processes[head(&readyList)]);
    int monitorId = peekMonitor(proc);

    if(monitorId == -1)
    {
        exit(1);
    }

    while(head(&(monitors[monitorId].waitingList)) != -1)
    {
        addLast(&(monitors[monitorId].readyList),
                removeHead(&(monitors[monitorId].waitingList)));
    }
}

void exitMonitor()
{
    ProcessDescriptor *proc = &(processes[head(&readyList)]);
    int monitorId = popMonitor(proc);

    if(monitorId == -1)
    {
        exit(1);
    }

    if(head(&(monitors[monitorId].readyList)) == -1)
    {
        if(!isInMonitor(proc, monitorId))
        {
            monitors[monitorId].locked = false;
        }
    }
    else
    {
        addLast(&readyList,
                removeHead(&(monitors[monitorId].readyList)));
    }
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
        printf("Error: No more events available\n");
        exit(EXIT_FAILURE);
    }
    events[nextEventID].waitingList = -1; // no process are waiting yet
    events[nextEventID].happened = false; // event hasn't happened yet

    return nextEventID++; // return the ID of the newly created event
}

void attendre(int eventID)
{
    fprintf(stderr, "Waiting on event %d\n", eventID);

    if(eventID < 0 || eventID >= nextEventID)
    {
        printf("Error: using invalid event!!\n");
        exit(EXIT_FAILURE);
    }

    if(!events[eventID].happened)
    {
        addLast(&(events[eventID].waitingList), removeHead(&readyList));
        transfer(processes[head(&readyList)].p);
    }
}

void declencher(int eventID)
{
    fprintf(stderr, "Triggering event %d\n", eventID);

    if(eventID < 0 || eventID >= nextEventID)
    {
        printf("Error: using invalid event!!\n");
        exit(EXIT_FAILURE);
    }

    events[eventID].happened = true;

    while(head(&(events[eventID].waitingList)) != -1)
    {
        addLast(&readyList, removeHead(&(events[eventID].waitingList)));
    }
}

void reinitialiser(int eventID)
{
    fprintf(stderr, "Reinit event %d\n", eventID);

    if(eventID < 0 || eventID >= nextEventID)
    {
        printf("Error: using invalid event!!\n");
        exit(EXIT_FAILURE);
    }

    events[eventID].happened = false;
}
