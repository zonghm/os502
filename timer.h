/*************************************************
 Header file of timer.c.
 It define a timerQueue structure and some functions
 of timerQueue operations.
 *************************************************/

#ifndef timer_h
#define timer_h

#include        "process.h"
#include        <stdbool.h>
#define         TIMER_Q_LOCK              0x7FE00001

struct timerQueue{
    PCB *pcb;
    long endTime;
    struct timerQueue *next;
};

/*
 * Prototype of timer queue functions
 */
bool timerEnQueue(PCB *pcb, long endTime);
PCB* timerDeQueue(void);
int getNumofTimerQProcess(void);
void startTimer(long sleepTime);

#endif /* timer_h */
