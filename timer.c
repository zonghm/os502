
/*************************************************
            timer.c for timerQueue management
 *************************************************/

#include                 "timer.h"
#include                 "stdlib.h"
#include                 "process.h"

extern struct timerQueue *head;         //timer queue head
extern struct timerQueue *current;      //timer queue tail

/**************************************************
 timerEnQueue is to put a new timerQueue element 
 into the timer queue. It accepts a point of PCB and
 the wake up time of the timer.
 **************************************************/
bool timerEnQueue(PCB *pcb, long endTime){
    
    struct timerQueue *ptr = (struct timerQueue*)malloc(sizeof(struct timerQueue));
    struct timerQueue *temp = NULL;
    bool startTimer = FALSE;
    
    ptr->pcb = pcb;
    ptr->endTime = endTime;
    ptr->next = NULL;
    
// Timer Queue is empty, then head and current are both point to enqueue element.
    if(head == NULL){
        head = ptr;
        current = ptr;
        startTimer = TRUE;
//        printf("Create timer queue with endTime: %ld\n",ptr->endTime);
    }
    else{
        if(endTime < head->endTime){        // current enqueue element goes to the head
            ptr->next = head;
            head = ptr;
//            printf("TimerEnQueueHead with endTime: %ld\n",ptr->endTime);
            startTimer = TRUE;
        }
        else{                               // current enqueue element goes to the meddle or tail.
            temp = head;
            
            while(temp->next !=NULL && (temp->next->endTime <= ptr->endTime)){
                temp = temp->next;
            }
            if(temp->next !=NULL){
                ptr->next = temp->next;
                temp->next = ptr;
            }
            else{
                temp->next = ptr;
                current = ptr;
            }
            
        }
    }
    return startTimer;
}

// Take one time queue element from head
PCB* timerDeQueue(){
    PCB *pcb;

        pcb = head->pcb;
        if(head->next == NULL){
            head = NULL;
            current = NULL;
        }else
        {
            head = head->next;
        }
    return pcb;
}

/*
 get number of processes in timer queue
 */
int getNumofTimerQProcess(){
    int count = 0;
    struct timerQueue *temp = NULL;
    temp = head;
    
    while( temp != NULL){
        count++;
        temp = temp-> next;
    }
    return count;
}

//start hardware timer, input:sleep time
void startTimer(long sleepTime){
    MEMORY_MAPPED_IO mmio;
    
    mmio.Mode = Z502Start;
    mmio.Field1 = sleepTime;
    mmio.Field2 = mmio.Field3 = 0;
    MEM_WRITE(Z502Timer, &mmio);
    printf("start timer: sleep %ld\n", sleepTime);
}
