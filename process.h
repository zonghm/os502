/********************************************************
 Header file of process.h.
 It define the structure of process, the sturcture of ready
 Queue and the prototype of functions in process.c
 ********************************************************/

#ifndef process_h
#define process_h

#include        "syscalls.h"

#define     RROCESS_RUNNING             1
#define     PROCESS_WAITTING            2
#define     RROCESS_READY               3
#define     PROCESS_SUSPENDED           4
#define     RROCESS_TERMINATED          5
#define     PROCESS_NEW                 6

#define     ERR_NO_SUCH_PROCESS         -1
#define     ILLEGAL_PRIORITY            -3
#define     LEGAL_PRIORITY              10
#define     DUPLICATE_PROCESS_NAME      -2
#define     MAXMIUM_NUMBER_PROCESS      15
#define     EXCEED_MAXMIUM_NUMBER_PROCESS 16

#define     READY_Q_LOCK                0x7FE00002
#define     DO_LOCK                     1
#define     DO_UNLOCK                   0
#define     SUSPEND_UNTIL_LOCKED        TRUE
#define     DO_NOT_SUSPEND              FALSE

/*
 * Process structure
 */
typedef struct  {
    INT32 processID;
    char* processName;
    void *pageTable;
    INT32 errorReturned;
    INT32 status;
    INT32 parent;
    long context;
} PCB;

/*
 * readyQueue structure
 */
struct readyQueue {
    PCB *pcb;
    struct readyQueue *next;
};

/*
 * Prototype of ready Queue functions
 */
void readyEnQueue(PCB *pcb);
PCB* readyDeQueue(void);
int readyQueueRemove(PCB *pcb);
int getNumofReadyProcess(void);
/*
 * Prototype of process functions
 */
void createProcess(char* name, void* PageTable, INT32 *processID, INT32 *ErrorReturned);
void getProcessID(char* processName, long *processID, long *ErrorReturned);
void startProcess(PCB *pcb);
void terminateProcess(long processID, long *ErrorReturned);
void terminateSelf(PCB *pcb);
int searchProcessName(char* name);
void addToPCBList(PCB *pcb);
void removePCB(PCB *pcb);
void dispatcher(void);
void goIdle(void);
void diskFormat( long diskID);
#endif /* process_h */


