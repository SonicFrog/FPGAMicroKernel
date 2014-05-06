#include "kernel.h"
#include "system_m.h"
#include "interrupt.h"
#include <stdlib.h>
#include <stdio.h>

// Maximum number of semaphores.
#define MAXSEMAPHORES 10
// Maximum number of processes.
#define MAXPROCESS 10


typedef struct {
    int next;
    Process p;
} ProcessDescriptor;

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



        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
         
