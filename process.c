/********************************************************
 process.c contains functions for managing ready Queue and 
 processes related functions. Such as create, start, 
 terminate processes.
 ********************************************************/

#include        "process.h"
#include        "string.h"

extern PCB *currentPcb;
extern PCB *pcb_list[20];

extern struct readyQueue *head_r;
extern struct readyQueue *current_r;
extern INT32  processCount;

/* create a new process with process name and return pageTable, processID
 and ErrorReturned. This routine also create a new PCB for this process.
 add the pcb to pcb list and put the pcb to ready queue. process count
 plus one.
*/
void createProcess(char* name, void *test, INT32 *processID, INT32 *ErrorReturned){
    
    INT32 LockResult;
    MEMORY_MAPPED_IO mmio;
    void *PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);
    PCB *pcb = (PCB*)malloc(sizeof(PCB));
    
    mmio.Mode = Z502InitializeContext;
    mmio.Field1 = 0;
    mmio.Field2 = (long) test;
    mmio.Field3 = (long) PageTable;
    MEM_WRITE(Z502Context, &mmio);
    *ErrorReturned= (INT32)mmio.Field4;

    pcb->processID = processCount;
    processID = &pcb->processID;
    int size =(int)strlen(name);
    pcb->processName = malloc(sizeof(char)*(size+1));
    strcpy(pcb->processName,name);
    pcb->pageTable = PageTable;
    pcb->context = mmio.Field1;
    pcb->status = PROCESS_NEW;
    pcb->parent = currentPcb->processID;
    
    READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
    readyEnQueue(pcb);
    READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
    addToPCBList(pcb);
    processCount++;
    }

/*
 start process for the input pcb
*/
void startProcess(PCB *pcb){
    
    MEMORY_MAPPED_IO mmio;
    currentPcb = pcb;
    currentPcb->status = RROCESS_RUNNING;
    mmio.Mode = Z502StartContext;
    mmio.Field1 = pcb->context;
    mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
    MEM_WRITE(Z502Context, &mmio);
//    printf("start a process with ProcessID: %d\n", pcb->processID);
}

/*
 get processID by process name. Input, process name, return processID and
 ErrorReturned message. If process found, return ERR_SUCCESS otherwise
 return ERR_NO_SUCH_PROCESS.
 */
void getProcessID(char* processName, long *processID, long *ErrorReturned){
    
    if(strcmp(processName, "") == 0){
        *processID =currentPcb->processID;
        *ErrorReturned = ERR_SUCCESS;
    }
    else{
        //check if the process exist with processName
        int processIdex = searchProcessName(processName);
        if(processIdex >=0){
            *processID = pcb_list[processIdex]->processID;
            *ErrorReturned = ERR_SUCCESS;
        }
        else{
            *ErrorReturned = ERR_NO_SUCH_PROCESS;
        }
    }
}

/*
terminate process with passed in processID. Input: processID,
 return ErrorReturned message. It remove pcb of terminated 
 process from the pcb list and process count minus one if 
 found, else return error message ERR_NO_SUCH_PROCESS.
 */
void terminateProcess(long *processID, long *ErrorReturned){

    int i =0;
    while(i<20){
        if (pcb_list[i] == NULL){
            i++;
            continue;
        }
        if(pcb_list[i]!= NULL && (pcb_list[i]->processID == (INT32)processID)){
            
            readyQueueRemove(pcb_list[i]);
            pcb_list[i]=NULL;
            processCount--;
            *ErrorReturned = ERR_SUCCESS;
//            printf("process terminated with PID %d \n",(INT32)processID);
            return;
        }
        i++;
    }
    *ErrorReturned = ERR_NO_SUCH_PROCESS;
}

/*
 Add pcb to the pcb list. Input: pcb
 */
void addToPCBList(PCB *pcb){
    int i=0;
    while(i<20){
        if(pcb_list[i] == NULL){
            pcb_list[i] = pcb;
            return;
        }
        i++;
    }
}

/*
 remove pcb from pcb list. Input: pcb.
 */
void removePCB(PCB *pcb){
    
    int i=0;
    while(i<20){
        if(pcb_list[i] == NULL){
            i++;
            continue;
        }
        if(pcb_list[i]->processID == pcb->processID){
            pcb_list[i]=NULL;
//            printf("*********terminate process %d\n",pcb->processID);
            return;
        }
        i++;
    }
}

/*
 search process name from pcb list. Input: process name.
 output: the index number in the list if found, else 
 return -1.
 */
int searchProcessName(char* name){
    
    int i =0;
    while(i<20){
        
        if(pcb_list[i] != NULL){
            
            if(strcmp(pcb_list[i]->processName, name) == 0){
                return i;
            }
        }

        i++;
    }
    return -1;
}

/*
 when a process need to wait for the hardware to finish, dispatcher is
 called. if there are other processes in the ready queue waiting for CPU,
 then give the process in the head of the ready queue a chance to run.
 */
void dispatcher(){
    
    INT32 LockResult;
    
    //check readyQueue, if there are waitting processes.
    while(head_r == NULL){
        CALL();
    }
    
    //start next process if head_r not null
    READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
    PCB *readyToRun =readyDeQueue();
    READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
    startProcess(readyToRun);
}

/*
 readyEnqueue, put pcb in ready queue, enqueue from tail. Input: pcb
 */

void readyEnQueue(PCB *pcb){
    
    struct readyQueue *ptr = (struct readyQueue*)malloc(sizeof(struct readyQueue));
    if(ptr == NULL)
    {printf("node creation failed.");
        return;
    }
    
    pcb->status =RROCESS_READY;
    ptr->pcb = pcb;
    ptr->next =NULL;
    
    if(head_r == NULL){
        head_r = current_r = ptr;
        return;
    }
    else{
        current_r->next = ptr;
        current_r = ptr;
        current_r->next = NULL;
    }
}

/*
 ready queue dequeue, take away element from head.
 */
PCB* readyDeQueue(){
    struct readyQueue *temp = NULL;
    if(head_r == NULL){
        return NULL;
    }
    else{
        temp = head_r;
        PCB *pcb = temp->pcb;
        if(head_r->next == NULL){
            head_r = NULL;
            current_r = NULL;
        }
        else{
            head_r = head_r ->next;
        }
        return pcb;
    }
}

/*
 ready queue remove is to remove the pcb from the
 the ready queue. Input: pcb, is the one need to
 remove.
 */
int readyQueueRemove(PCB *pcb){
    struct readyQueue *temp = NULL;
    if(head_r==NULL){
        return -1;
    }
    else if(head_r->pcb ==pcb){
            head_r = head_r->next;
            return 0;
        }
    else{
            temp = head_r;
            while(temp->next->pcb != pcb && temp->next->next !=NULL){
                temp = temp->next;
            }
            if(temp->next->pcb == pcb){
                temp->next = temp->next->next;
                if(temp->next == NULL){
                    current_r = temp;
                }
                return 1;
            }
        }
    return -2;
}

/*
 get number of processes in ready queue
 */
int getNumofReadyProcess(){
    int count = 0;
    struct readyQueue *temp = NULL;
    temp = head_r;
    
    while( temp != NULL){
        count++;
        temp = temp-> next;
    }
    return count;
}

/*
 go idle is the z502 hardware idle, do nothing
 */
void goIdle(){
    MEMORY_MAPPED_IO mmio;
    mmio.Mode = Z502Action;
    mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;    
    MEM_WRITE(Z502Idle, &mmio);
    DoSleep(10);
    printf("went into idle()\n");
}
