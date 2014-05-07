#ifndef _DEF_MONITOR_H
#define _DEF_MONITOR_H

#define MAX_MONITORS 10

typedef struct {
    int waitingList;
    int readyList;
} MonitorDescriptor;

/**
 * Creates a new monitor and returns an idenfitier
 **/
int createMonitor();

/**
 * The running process enter the given monitor
 **/
void enterMonitor(int monitorID);

/**
 * Puts the running process in waiting state on the given monitor
 **/
void wait();

/**
 * Notifies a thread currently waiting on the monitor the running
 * process is in
 **/
void notify();

/**
 * Notifies all threads currently waiting on the monitor the runnnig
 * process is in
 **/
void notifyAll();

/**
 * The running process exists the monitor it is in
 **/
void exitMonitor();

#endif //_DEF_MONITOR_H
