#include <stdio.h>
#include "kernel.h"

// Global variables

int counter;
// semaphore
int sem;

void process1(){
    printf("Process 1...\n");
    yield();
    while(1){
        P(sem);
        counter++;
        yield();
        int temp = counter;
        V(sem);
        yield();
        printf("Process 1, counter = %d\n", temp);
    }
}

void process2(){
    printf("Process 2...\n");
    while(1){
        P(sem);
        counter++;
        yield();
        int temp = counter;
        V(sem);
        yield();
        printf("Process 2, counter = %d\n", temp);
    }
} 


int main() {
  
  createProcess(process1, 10000);
  createProcess(process2, 10000);
  
  sem = createSemaphore(1);
  counter = 0;
  start();
  return 0;
}



           