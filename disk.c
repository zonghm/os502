/*****************************************
 ***********Disk Queue functions**********
 *****************************************/

#include            "disk.h"
#include            "stdlib.h"
#include            "process.h"

/* eight disk queue structures defined,
each queue has a head and current pointer,
 it has the FCFS scheduling behavior */
extern struct diskQueue *head_d;
extern struct diskQueue *current_d;
extern struct diskQueue *head_d1;
extern struct diskQueue *current_d1;
extern struct diskQueue *head_d2;
extern struct diskQueue *current_d2;
extern struct diskQueue *head_d3;
extern struct diskQueue *current_d3;
extern struct diskQueue *head_d4;
extern struct diskQueue *current_d4;
extern struct diskQueue *head_d5;
extern struct diskQueue *current_d5;
extern struct diskQueue *head_d6;
extern struct diskQueue *current_d6;
extern struct diskQueue *head_d7;
extern struct diskQueue *current_d7;

/******************************************************
 put the disk queue element into the disk queue. Inputs 
 are diskID, sector, Is a disk read or writeoperation,
 buffer address, and process information associated with
 this disk operation. Output is a NON-NULL disk queue.
 ******************************************************/

void diskEnQueue(long diskID, long sector, int read_write, char* address, PCB *pcb){
    
    struct diskQueue *ptr = (struct diskQueue*)malloc(sizeof(struct diskQueue));
    
    if(ptr == NULL){
        printf("node creation failed.");
        return;
    }
    ptr->diskID = diskID;
    ptr->sector = sector;
    ptr->read_write = read_write;
    ptr->address = address;
    ptr->pcb = pcb;
    ptr->next = NULL;
    
    switch (diskID) {
        case 0:
            if(head_d == NULL){
                head_d = current_d =ptr;
            }
            else{
//                printf("*****Enqueue disk queue0 :**** %ld\n", diskID);
                current_d->next = ptr;
                current_d = ptr;
            }
            break;
            
        case 1:
            if(head_d1 == NULL){
                head_d1 = current_d1 =ptr;
            }
            else{
//                printf("*****Enqueue disk queue1 :**** %ld\n", diskID);
                current_d1->next = ptr;
                current_d1 = ptr;
            }
            break;
            
        case 2:
            if(head_d2 == NULL){
                head_d2 = current_d2 =ptr;
            }
            else{
//                printf("*****Enqueue disk queue2 :**** %ld\n", diskID);
                current_d2->next = ptr;
                current_d2 = ptr;
            }
            break;
            
        case 3:
            if(head_d3 == NULL){
                head_d3 = current_d3 =ptr;
            }
            else{
//                printf("*****Enqueue disk queue3 :**** %ld\n", diskID);
                current_d3->next = ptr;
                current_d3 = ptr;
            }
            break;
            
        case 4:
            if(head_d4 == NULL){
                head_d4 = current_d4 =ptr;
            }
            else
            {
//                printf("*****Enqueue disk queue4 :**** %ld\n", diskID);
                current_d4->next = ptr;
                current_d4 = ptr;
            }
            break;
            
        case 5:
            if(head_d5 == NULL){
                head_d5 = current_d5 =ptr;
            }
            else
            {
//                printf("*****Enqueue disk queue5 :**** %ld\n", diskID);
                current_d5->next = ptr;
                current_d5 = ptr;
            }
            break;
            
        case 6:
            if(head_d6 == NULL){
                head_d6 = current_d6 =ptr;
            }
            else{
//                printf("*****Enqueue disk queue6 :**** %ld\n", diskID);
                current_d6->next = ptr;
                current_d6 = ptr;
            }
            break;
            
        case 7:
            if(head_d7 == NULL){
                head_d7 = current_d7 =ptr;
            }
            else{
//                printf("*****Enqueue disk queue7 :**** %ld\n", diskID);
                current_d7->next = ptr;
                current_d7 = ptr;
            }
            break;
            
        default:
            break;
    }
 
}

/*************************************************
 Take away the tail element from the disk queue.
 Input is the diskID, output is the PCB structure
 of one process.
 *************************************************/
PCB* diskDeQueue(long diskID){
    
    PCB *pcb = NULL;
    struct diskQueue *temp = (struct diskQueue*)malloc(sizeof(struct diskQueue));
    
    if(temp == NULL){
        printf("node creation failed.");
        return pcb;
    }
    
    switch(diskID){
        case 0:
            if(head_d == NULL){
                return NULL;
            }
            else{
                temp = head_d;
                pcb = temp->pcb;
                if(temp->next == NULL){
                    head_d = NULL;
                    current_d = NULL;
                }
                else{
                    head_d = temp->next;
                }
            }
            break;
        case 1:
            if(head_d1 == NULL){
                return NULL;
            }
            else{
                temp = head_d1;
                pcb = temp->pcb;
                if(temp->next == NULL){
                    head_d1 = NULL;
                    current_d1 = NULL;
                }else{
                    head_d1 = temp->next;
                }
            }
            break;
        case 2:
            if(head_d2 == NULL){
                return NULL;
            }
            else{
                temp = head_d2;
                pcb = temp->pcb;
                if(temp->next == NULL){
                    head_d2 = NULL;
                    current_d2 = NULL;
                }else{
                    head_d2 = temp->next;
                }
            }
            break;
        case 3:
            if(head_d3 == NULL){
                return NULL;
            }
            else{
                temp = head_d3;
                pcb = temp->pcb;
                if(temp->next == NULL){
                    head_d3 = NULL;
                    current_d3 = NULL;
                }else{
                    head_d3 = temp->next;
                }
            }
            break;
        case 4:
            if(head_d4 == NULL){
                return NULL;
            }
            else{
                temp = head_d4;
                pcb = temp->pcb;
                if(temp->next == NULL){
                    head_d4 = NULL;
                    current_d4 = NULL;
                }else{
                    head_d4 = temp->next;
                }
            }
            break;
        case 5:
            if(head_d5 == NULL){
                return NULL;
            }
            else{
                temp = head_d5;
                pcb = temp->pcb;
                if(temp->next == NULL){
                    head_d5 = NULL;
                    current_d5 = NULL;
                }else{
                    head_d5 = temp->next;
                }
            }
            break;
        case 6:
            if(head_d6 == NULL){
                return NULL;
            }
            else{
                temp = head_d6;
                pcb = temp->pcb;
                if(temp->next == NULL){
                    head_d6 = NULL;
                    current_d6 = NULL;
                }else{
                    head_d6 = temp->next;
                }
            }
            break;
        case 7:
            if(head_d7 == NULL){
                return NULL;
            }
            else{
                temp = head_d7;
                pcb = temp->pcb;
                if(temp->next == NULL){
                    head_d7 = NULL;
                    current_d7 = NULL;
                }else{
                    head_d7 = temp->next;
                }
            }
            break;
        default:
            printf("Invalid diskID\n");
            break;
    }
    
    free(temp);
    return pcb;
}

/*
 get number of process in Disk queue
 */
int getNumofDiskQProcess( long DiskID){
    
    int count = 0;
    struct diskQueue *temp = NULL;
    
    switch(DiskID){
        case 0:
            temp = head_d;
            break;
        case 1:
            temp = head_d1;
            break;
        case 2:
            temp = head_d2;
            break;
        case 3:
            temp = head_d3;
            break;
        case 4:
            temp = head_d4;
            break;
        case 5:
            temp = head_d5;
            break;
        case 6:
            temp = head_d6;
            break;
        case 7:
            temp = head_d7;
            break;
        default:
            break;
    }
    
    while( temp != NULL){
        count++;
        temp = temp-> next;
    }
    return count;
}
