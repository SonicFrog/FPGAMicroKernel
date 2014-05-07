#include "kernel.h"
#include "system_m.h"
#include "interrupt.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Maximum number of semaphores.
#define MAXSEMAPHORES 10
// Maximum number of processes.
#define MAXPROCESS 10


typedef struct {
    int next;
    Process p;
    int monitors[MAX_MONITORS];
} ProcessDescriptor;

typedef struct {
    int n;
    int waitingList;
    bool locked;
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

void createProcess (void (*f), int stackSize) {
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

size_t nextMonitorID = 0;

int getLastMonitor(ProcessDescriptor p)
{
    int i = -1;
    while(p.monitors[i + 1] != -1 && i + 1 < MAX_MONITORS)
    {
        i++;
    }
    return i;
}

int createMonitor()
{
    if(nextMonitorID == MAX_MONITORS)
    {
        fprintf(stderr, "Error: No more monitors available\n");
        exit(EXIT_FAILURE);
    }
    monitors[nextMonitorID].waitingList = -1;
    monitors[nextMonitorID].readyList = -1;
    monitors[nextMonitorID].locked = false;
    return nextMonitorID++;
}

void enterMonitor(int monitorID)
{
    if(monitorID >= nextMonitorID)
    {
        fprintf(stderr, "Error: using invalid monitor!!\n");
        exit(EXIT_FAILURE);
    }

    MonitorDescriptor desc = monitors[monitorID];

    if(desc.locked)
    {
        int p = removeHead(&readyList);
        addLast(&(desc.readyList), p);
        transfer(processes[head(&readyList)].p);
    }
    else
    {
        desc.locked = true;
    }
}

void wait() {
    int mid = getLastMonitor(head(&readyList));
    int pid = head(&(desc.readyList));
    MonitorDescriptor desc = monitors[mid];
    int p = removeHead(&readyList);

    addLast(&desc.waitingList, p);

    if(pid == -1) //No one can execute on this monitor
    {
        desc.locked = false;
    }
    else //Wake the first ready process for this monitor
    {
        addLast(&readyList, removeHead(&desc.readyList));
    }
    transfer(processes[head(&readyList)].p);
}

void notify() {
    int mid = getLastMonitor(processes[head(&readyList)].p);
    if(mid == -1)
    {
        //TODO handler errors
        exit(1);
    }
    //Put the first process in the waiting list of this monitor
    //in the ready list of the monitor
    addLast(&monitors[mid].readyList, removeHead(&monitors[mid].waitingList));
}

void notifyAll() {
    int mid = getLastMonitor(processes[head(&readyList)].p);
    if(mid == -1) 
    {
        //TODO handle errors 
        exit(1);
    }

    MonitorDescriptor desc = monitors[mid];

    while(head(&desc.waitingList) != -1)
    {
        addLast(&desc.readyList, removeHead(&desc.waitingList));
    }
}

void exitMonitor() {
    ProcessDescriptor desc = processes[head(&readyList)];
    int mid = getLastMonitor(desc);

    if(mid == -1)
    {
        fprintf(stderr, "A process that was in no monitors exited a monitor\n");
        exit(1);
    }
}
