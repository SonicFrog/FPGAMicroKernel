#ifndef KERNEL_H_
#define KERNEL_H_

void createProcess (void (*f), int stackSize);

void start();

int createSemaphore(int n);

void P(int s);

void V(int s);

void yield();


#endif /*KERNEL_H_*/
