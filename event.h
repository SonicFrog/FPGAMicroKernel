#ifndef _DEF_EVENT_H
#define _DEF_EVENT_H

#define MAX_EVENTS 10

typedef struct {
    int waitingList;
    bool happened;
} EventDescriptor;

/**
 * Creates a new event and returns an idenfitier
 **/
int createEvent();

/**
 * Puts the running process in waiting state on the given event if it hasn't happened yet
 **/
void attendre(int eventID);

/**
 * Unlock all threads currently waiting on the given event
 **/
void declencher(int eventID);

/**
 * We reinitialize the given event (revert the happened part)
 **/
void reinitialiser(int eventID);

#endif //_DEF_EVENT_H