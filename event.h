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

void attendre(int eventID);
void declencher(int eventID);
void reinitialiser(int eventID);

#endif //_DEF_EVENT_H