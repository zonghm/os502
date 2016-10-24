/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Portability attempted.
 1.3 July     1992: More Portability enhancements.
 Add call to SampleCode.
 1.4 December 1992: Limit (temporarily) printout in
 interrupt handler.  More portability.
 2.0 January  2000: A number of small changes.
 2.1 May      2001: Bug fixes and clear STAT_VECTOR
 2.2 July     2002: Make code appropriate for undergrads.
 Default program start is in test0.
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 4.0  July    2013: Major portions rewritten to support multiple threads
 4.20 Jan     2015: Thread safe code - prepare for multiprocessors
 ************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             "timer.h"
#include             "disk.h"
#include             "process.h"
#include             <stdlib.h>
#include             <ctype.h>

#define              DO_LOCK                     1
#define              DO_UNLOCK                   0
#define              SUSPEND_UNTIL_LOCKED        TRUE
#define              DO_NOT_SUSPEND              FALSE

//  Allows the OS and the hardware to agree on where faults occur
extern void *TO_VECTOR[];

INT32  processCount=0;
PCB *currentPcb= NULL;
PCB *pcb_list[20]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
SP_INPUT_DATA SPData;
int printCount = 0;

struct timerQueue *head = NULL;             //timer queue head
struct timerQueue *current = NULL;          //timer queue tail
struct readyQueue *head_r = NULL;           //ready queue head
struct readyQueue *current_r = NULL;        //ready queue tail
struct diskQueue *head_d = NULL;            //disk queue head of disk0
struct diskQueue *current_d = NULL;         //disk queue tail of disk0
struct diskQueue *head_d1 = NULL;           //disk queue head of disk1
struct diskQueue *current_d1 = NULL;        //disk queue tail of disk1
struct diskQueue *head_d2 = NULL;           //disk queue head of disk2
struct diskQueue *current_d2 = NULL;        //disk queue tail of disk2
struct diskQueue *head_d3 = NULL;           //disk queue head of disk3
struct diskQueue *current_d3 = NULL;        //disk queue tail of disk3
struct diskQueue *head_d4 = NULL;           //disk queue head of disk4
struct diskQueue *current_d4 = NULL;        //disk queue tail of disk4
struct diskQueue *head_d5 = NULL;           //disk queue head of disk5
struct diskQueue *current_d5 = NULL;        //disk queue tail of disk5
struct diskQueue *head_d6 = NULL;           //disk queue head of disk6
struct diskQueue *current_d6 = NULL;        //disk queue tail of disk6
struct diskQueue *head_d7 = NULL;           //disk queue head of disk7
struct diskQueue *current_d7 = NULL;        //disk queue tail of disk7

char *call_names[]={ "mem_read ", "mem_write", "read_mod ", "get_time ",
		"sleep    ", "get_pid  ", "create   ", "term_proc", "suspend  ",
		"resume   ", "ch_prior ", "send     ", "receive  ", "PhyDskRd ",
		"PhyDskWrt", "def_sh_ar", "Format   ", "CheckDisk", "Open_Dir ",
        "OpenFile ", "Crea_Dir ", "Crea_File", "ReadFile ", "WriteFile",
		"CloseFile", "DirContnt", "Del_Dir  ", "Del_File "};

/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the OS.
 ************************************************************************/
void InterruptHandler(void) {
	INT32 DeviceID;
	INT32 Status;
    INT32 LockResult;
	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = (INT32)mmio.Field1;
	Status = (INT32)mmio.Field2;
    
    if(DeviceID == TIMER_INTERRUPT){
        
        if(printCount < 11){
            printf("went into interrupt handler by Timer_interrupt\n");
            printCount++;
        }

        READ_MODIFY(TIMER_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        PCB *pcb = timerDeQueue();
        READ_MODIFY(TIMER_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);

        READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        readyEnQueue(pcb);
        READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        //if timer queue has waitting process
        if(head != NULL){
            
            //get current time
            MEMORY_MAPPED_IO mmio;
            mmio.Mode = Z502ReturnValue;
            mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
            MEM_READ(Z502Clock, &mmio);
            
            //while the wake up time of head process in queue is early than current time,
            //then the process dequeque from timer queue and en queue to ready queue.
            while(head !=NULL && mmio.Field1 > head->endTime){
                READ_MODIFY(TIMER_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
                PCB *pcb = timerDeQueue();
                READ_MODIFY(TIMER_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
                
                READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
                readyEnQueue(pcb);
                READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
            }
            
            // start the next timer task
            if(head !=NULL){
                long sleepTime =head->endTime - mmio.Field1;
                startTimer(sleepTime);
//                printf("reset timer with sleeptime:%ld\n",sleepTime);
            }
            
        }
    }
    else if(DeviceID == DISK_INTERRUPT_DISK0){
        if(printCount < 11){
            printf("went into interrupt handler by Disk_interrupt_Disk0\n");
            printCount++;
        }

        //de queue the head process in disk0 queue
        READ_MODIFY(DISK_0_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        PCB *pcb = diskDeQueue(0);
        READ_MODIFY(DISK_0_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // put this process into readyQueue
        READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        readyEnQueue(pcb);
        READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // check if diskQueue0 has waitting process
        if(head_d !=NULL){
            
            MEMORY_MAPPED_IO mmio;
            
            if(head_d->read_write == 0){                //disk read
                mmio.Mode = Z502DiskRead;
                mmio.Field1 = head_d->diskID;
                mmio.Field2 = head_d->sector;
                mmio.Field3 = (long)head_d->address;
                mmio.Field4 = 0;
                MEM_READ(Z502Disk, &mmio);
            }else{                                      //disk write
                mmio.Mode = Z502DiskWrite;
                mmio.Field1 = head_d->diskID;
                mmio.Field2 = head_d->sector;
                mmio.Field3 = (long)head_d->address;
                mmio.Field4 = 0;
                MEM_WRITE(Z502Disk, &mmio);
            }
        }
    }
    else if(DeviceID == DISK_INTERRUPT_DISK1){
        
        if(printCount < 11){
            printf("went into interrupt handler by Disk_interrupt_Disk1\n");
            printCount++;
        }
        READ_MODIFY(DISK_1_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        PCB *pcb = diskDeQueue(1);
        READ_MODIFY(DISK_1_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        

        READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        readyEnQueue(pcb);
        READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // check if diskQueue1 is empty
        if(head_d1 !=NULL){
            
            MEMORY_MAPPED_IO mmio;

            if(head_d1->read_write == 0){                       //disk read
                mmio.Mode = Z502DiskRead;
                mmio.Field1 = head_d1->diskID;
                mmio.Field2 = head_d1->sector;
                mmio.Field3 = (long)head_d1->address;
                mmio.Field4 = 0;
                MEM_READ(Z502Disk, &mmio);
            }else{                                              //disk write
                mmio.Mode = Z502DiskWrite;
                mmio.Field1 = head_d1->diskID;
                mmio.Field2 = head_d1->sector;
                mmio.Field3 = (long)head_d1->address;
                mmio.Field4 = 0;
                MEM_WRITE(Z502Disk, &mmio);
            }
        }
    }
    else if(DeviceID == DISK_INTERRUPT_DISK2){
        
        if(printCount < 11){
            printf("went into interrupt handler by Disk_interrupt_Disk2\n");
            printCount++;
        }
        READ_MODIFY(DISK_2_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        PCB *pcb = diskDeQueue(2);
        READ_MODIFY(DISK_2_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // put this process into readyQueue
        READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        readyEnQueue(pcb);
        READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // check if diskQueue2 is empty
        if(head_d2 !=NULL){
            
            MEMORY_MAPPED_IO mmio;

            if(head_d2->read_write == 0){
                mmio.Mode = Z502DiskRead;
                mmio.Field1 = head_d2->diskID;
                mmio.Field2 = head_d2->sector;
                mmio.Field3 = (long)head_d2->address;
                mmio.Field4 = 0;
                MEM_READ(Z502Disk, &mmio);
            }else{
                mmio.Mode = Z502DiskWrite;
                mmio.Field1 = head_d2->diskID;
                mmio.Field2 = head_d2->sector;
                mmio.Field3 = (long)head_d2->address;
                mmio.Field4 = 0;
                MEM_WRITE(Z502Disk, &mmio);
            }
        }
    }
    else if(DeviceID == DISK_INTERRUPT_DISK3){
        
        if(printCount < 11){
            printf("went into interrupt handler by Disk_interrupt_Disk3\n");
            printCount++;
        }
        READ_MODIFY(DISK_3_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        PCB *pcb = diskDeQueue(3);
        READ_MODIFY(DISK_3_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // put this process into readyQueue
        READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        readyEnQueue(pcb);
        READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // check if diskQueue3 is empty
        if(head_d3 !=NULL){
            
            MEMORY_MAPPED_IO mmio;

            if(head_d3->read_write == 0){
                mmio.Mode = Z502DiskRead;
                mmio.Field1 = head_d3->diskID;
                mmio.Field2 = head_d3->sector;
                mmio.Field3 = (long)head_d3->address;
                mmio.Field4 = 0;
                MEM_READ(Z502Disk, &mmio);
            }else{
                mmio.Mode = Z502DiskWrite;
                mmio.Field1 = head_d3->diskID;
                mmio.Field2 = head_d3->sector;
                mmio.Field3 = (long)head_d3->address;
                mmio.Field4 = 0;
                MEM_WRITE(Z502Disk, &mmio);
            }
        }
    }
    else if(DeviceID == DISK_INTERRUPT_DISK4){
        if(printCount < 11){
            printf("went into interrupt handler by Disk_interrupt_Disk4\n");
            printCount++;
        }
        READ_MODIFY(DISK_4_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        PCB *pcb = diskDeQueue(4);
        READ_MODIFY(DISK_4_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // put this process into readyQueue
        READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        readyEnQueue(pcb);
        READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // check if diskQueue4 is empty
        if(head_d4 !=NULL){
            MEMORY_MAPPED_IO mmio;

            if(head_d4->read_write == 0){
                mmio.Mode = Z502DiskRead;
                mmio.Field1 = head_d4->diskID;
                mmio.Field2 = head_d4->sector;
                mmio.Field3 = (long)head_d4->address;
                mmio.Field4 = 0;
                MEM_READ(Z502Disk, &mmio);
            }else{
                mmio.Mode = Z502DiskWrite;
                mmio.Field1 = head_d4->diskID;
                mmio.Field2 = head_d4->sector;
                mmio.Field3 = (long)head_d4->address;
                mmio.Field4 = 0;
                MEM_WRITE(Z502Disk, &mmio);
            }
        }
    }
    else if(DeviceID == DISK_INTERRUPT_DISK5){
        if(printCount < 11){
            printf("went into interrupt handler by Disk_interrupt_Disk5\n");
            printCount++;
        }
        READ_MODIFY(DISK_5_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        PCB *pcb = diskDeQueue(5);
        READ_MODIFY(DISK_5_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // put this process into readyQueue
        READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        readyEnQueue(pcb);
        READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // check if diskQueue5 is empty
        if(head_d5 !=NULL){
            
            MEMORY_MAPPED_IO mmio;

            if(head_d5->read_write == 0){
                mmio.Mode = Z502DiskRead;
                mmio.Field1 = head_d5->diskID;
                mmio.Field2 = head_d5->sector;
                mmio.Field3 = (long)head_d5->address;
                mmio.Field4 = 0;
                MEM_READ(Z502Disk, &mmio);
            }else{
                mmio.Mode = Z502DiskWrite;
                mmio.Field1 = head_d5->diskID;
                mmio.Field2 = head_d5->sector;
                mmio.Field3 = (long)head_d5->address;
                mmio.Field4 = 0;
                MEM_WRITE(Z502Disk, &mmio);
            }
        }
    }
    else if(DeviceID == DISK_INTERRUPT_DISK6){
        if(printCount < 11){
            printf("went into interrupt handler by Disk_interrupt_Disk6\n");
            printCount++;
        }
        READ_MODIFY(DISK_6_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        PCB *pcb = diskDeQueue(6);
        READ_MODIFY(DISK_6_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // put this process into readyQueue
        READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        readyEnQueue(pcb);
        READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // check if diskQueue6 is empty
        if(head_d6 !=NULL){
            MEMORY_MAPPED_IO mmio;

            if(head_d6->read_write == 0){
                mmio.Mode = Z502DiskRead;
                mmio.Field1 = head_d6->diskID;
                mmio.Field2 = head_d6->sector;
                mmio.Field3 = (long)head_d6->address;
                mmio.Field4 = 0;
                MEM_READ(Z502Disk, &mmio);
            }else{
                mmio.Mode = Z502DiskWrite;
                mmio.Field1 = head_d6->diskID;
                mmio.Field2 = head_d6->sector;
                mmio.Field3 = (long)head_d6->address;
                mmio.Field4 = 0;
                MEM_WRITE(Z502Disk, &mmio);
            }
        }
    }
    else if(DeviceID == DISK_INTERRUPT_DISK7){
        if(printCount < 11){
            printf("went into interrupt handler by Disk_interrupt_Disk7\n");
            printCount++;
        }
        READ_MODIFY(DISK_7_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        PCB *pcb = diskDeQueue(7);
        READ_MODIFY(DISK_7_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // put this process into readyQueue
        READ_MODIFY(READY_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        readyEnQueue(pcb);
        READ_MODIFY(READY_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
        
        // check if diskQueue7 is empty
        if(head_d7 !=NULL){
            MEMORY_MAPPED_IO mmio;

            if(head_d7->read_write == 0){
                mmio.Mode = Z502DiskRead;
                mmio.Field1 = head_d7->diskID;
                mmio.Field2 = head_d7->sector;
                mmio.Field3 = (long)head_d7->address;
                mmio.Field4 = 0;
                MEM_READ(Z502Disk, &mmio);
            }else{
                mmio.Mode = Z502DiskWrite;
                mmio.Field1 = head_d7->diskID;
                mmio.Field2 = head_d7->sector;
                mmio.Field3 = (long)head_d7->address;
                mmio.Field4 = 0;
                MEM_WRITE(Z502Disk, &mmio);
            }
        }
    }
    else{
        printf("Hardware Interupt can not be identified!\n");
    }

	// Clear out this device - we're done with it
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	mmio.Field2 = mmio.Field3 = 0;
	MEM_WRITE(Z502InterruptDevice, &mmio);
}           // End of InterruptHandler

/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.
 ************************************************************************/

void FaultHandler(void) {
	INT32 DeviceID;
	INT32 Status;

	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	// Get cause of interrupt
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	mmio.Mode = Z502GetInterruptInfo;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = (INT32)mmio.Field1;
	Status =(INT32) mmio.Field2;

	printf("Fault_handler: Found vector type %d with value %d\n", DeviceID,
			Status);
	// Clear out this device - we're done with it
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	MEM_WRITE(Z502InterruptDevice, &mmio);
} // End of FaultHandler

/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 The variable do_print is designed to print out the data for the
 incoming calls, but does so only for the first ten calls.  This
 allows the user to see what's happening, but doesn't overwhelm
 with the amount of data.
 ************************************************************************/

void svc(SYSTEM_CALL_DATA *SystemCallData) {
	
    short call_type;
    
    long sleepTime;
    long diskID =0;
    long currentTime;
    
    INT32 LockResult;
    INT32 diskLock =0;
    
    bool start;

    MEMORY_MAPPED_IO mmio;
    struct readyQueue *temp_r = NULL;
    struct timerQueue *temp_t = NULL;
    struct diskQueue  *temp_d = NULL;
    
	call_type = (short) SystemCallData->SystemCallNumber;
    printf("SVC handler: %s\n", call_names[call_type]);

    switch (call_type) {

        case SYSNUM_GET_TIME_OF_DAY:
            
            mmio.Mode = Z502ReturnValue;
            mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
            MEM_READ(Z502Clock, &mmio);
             *(long*)SystemCallData->Argument[0] = mmio.Field1;
            break;
            
        case SYSNUM_TERMINATE_PROCESS:
                
            if((INT32)SystemCallData->Argument[0] == -2){           //terminate the current process and its child
                
                mmio.Mode = Z502Action;
                mmio.Field1 = mmio.Field2 = mmio.Field3 =0;
                MEM_WRITE(Z502Halt, &mmio);
                *SystemCallData->Argument[1] =ERR_SUCCESS;
            }
            else if ((INT32)SystemCallData->Argument[0] == -1){     //terminate self
                
                terminateSelf(currentPcb);
                
                mmio.Mode = Z502Action;
                mmio.Field1 = mmio.Field2 = mmio.Field3 =0;
                MEM_WRITE(Z502Halt, &mmio);
                *SystemCallData->Argument[1] =ERR_SUCCESS;
            }
            else                                                    //terminate process by process name
            {
                terminateProcess(SystemCallData->Argument[0], SystemCallData->Argument[1]);
            }
            break;

        case SYSNUM_CREATE_PROCESS:
            
            // if try to create an illegal priority process, print warning, no process created.
            if((INT32)SystemCallData->Argument[2] == ILLEGAL_PRIORITY){
                *SystemCallData->Argument[4] = ILLEGAL_PRIORITY;
                printf("try to create an ILLEGAL_PRIORITY process!\n");
                break;
            }
            // if try to create an process with process name has been created, print warning.
            if(searchProcessName((char *)SystemCallData->Argument[0])!= -1){
                *SystemCallData->Argument[4] = DUPLICATE_PROCESS_NAME;
                printf("try to create an process with duplicate process name!\n");
                break;
            }
            // if the total process number is not reach the maximun, then create a new process
            if(processCount< (MAXMIUM_NUMBER_PROCESS+1)){

/***********   print   ***********/
                memset(&SPData, 0, sizeof(SP_INPUT_DATA));
                strcpy(SPData.TargetAction, "Create");
                SPData.CurrentlyRunningPID = currentPcb->processID;
                SPData.TargetPID = processCount;
                SPData.NumberOfRunningProcesses = 0;
                
                SPData.NumberOfReadyProcesses = getNumofReadyProcess();   // Processes ready to run
                if(SPData.NumberOfReadyProcesses > 0){
                    int i = 0;
                    temp_r = head_r;
                    
                    while( temp_r !=NULL){
                        SPData.ReadyProcessPIDs[i] = temp_r->pcb->processID;
                        temp_r = temp_r->next;
                        i++;
                    }
                }
                
                SPData.NumberOfTimerSuspendedProcesses = getNumofTimerQProcess();
                if(SPData.NumberOfTerminatedProcesses > 0){
                    int i = 0;
                    temp_t = head;
                    
                    while(temp_t != NULL){
                        SPData.TimerSuspendedProcessPIDs[i] = temp_t->pcb->processID;
                        temp_t = temp_t->next;
                        i++;
                    }
                }

                SPData.NumberOfDiskSuspendedProcesses = getNumofDiskQProcess(0);
                if(SPData.NumberOfDiskSuspendedProcesses >0){
                    int i =0;
                    temp_d = head_d;
                    
                    while(temp_d != NULL){
                        SPData.DiskSuspendedProcessPIDs[i] = temp_d->pcb->processID;
                        temp_d = temp_d->next;
                        i++;
                    }
                }
                
                CALL(SPPrintLine(&SPData));
/*********  end print ************/
                
                createProcess((char*)SystemCallData->Argument[0],SystemCallData->Argument[1],
                              (INT32*)SystemCallData->Argument[3],(INT32*)SystemCallData->Argument[4]);
                }
            else{
                printf("Exceed the allowed maximium number of processes\n");
                *SystemCallData->Argument[4] = EXCEED_MAXMIUM_NUMBER_PROCESS;
            }
            break;

        case SYSNUM_SLEEP:
            
            sleepTime = (INT32)SystemCallData->Argument[0];
    
            // get current time
            mmio.Mode = Z502ReturnValue;
            mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
            MEM_READ(Z502Clock, &mmio);
            
            currentTime = mmio.Field1;
            currentPcb->status = PROCESS_WAITTING;
            
            
            READ_MODIFY(TIMER_Q_LOCK, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
            start = timerEnQueue(currentPcb, (sleepTime + currentTime));
            READ_MODIFY(TIMER_Q_LOCK, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
            
            if(start){                              //start = TRUE : timer need to be reset.
                startTimer(sleepTime);
            }
            
            /***********   print   ***********/
            memset(&SPData, 0, sizeof(SP_INPUT_DATA));
            strcpy(SPData.TargetAction, "Sleep");
            SPData.CurrentlyRunningPID = currentPcb->processID;
            
            if(head_r != NULL){
                SPData.TargetPID = head_r->pcb->processID;
            }else{
                SPData.TargetPID = currentPcb->processID;
            }
    
            SPData.NumberOfRunningProcesses = 0;
            
            SPData.NumberOfReadyProcesses = getNumofReadyProcess();   // Processes ready to run
            if(SPData.NumberOfReadyProcesses > 0){
                int i = 0;
                temp_r = head_r;
                
                while( temp_r !=NULL){
                    SPData.ReadyProcessPIDs[i] = temp_r->pcb->processID;
                    temp_r = temp_r->next;
                    i++;
                }
            }
            
            SPData.NumberOfTimerSuspendedProcesses = getNumofTimerQProcess();
            if(SPData.NumberOfTimerSuspendedProcesses > 0){
                int i = 0;
                temp_t = head;
                
                while(temp_t != NULL){
                    SPData.TimerSuspendedProcessPIDs[i] = temp_t->pcb->processID;
                    temp_t = temp_t->next;
                    i++;
                }
            }
            
            SPData.NumberOfDiskSuspendedProcesses = getNumofDiskQProcess(0);
            if(SPData.NumberOfDiskSuspendedProcesses >0){
                int i =0;
                temp_d = head_d;
                
                while(temp_d != NULL){
                    SPData.DiskSuspendedProcessPIDs[i] = temp_d->pcb->processID;
                    temp_d = temp_d->next;
                    i++;
                }
            }
            
            CALL(SPPrintLine(&SPData));
            /*********  end print ************/
            
            dispatcher();
            break;
            
        case SYSNUM_GET_PROCESS_ID:
            
            getProcessID((char*)SystemCallData->Argument[0],SystemCallData->Argument[1],SystemCallData->Argument[2]);
            break;
            
        case SYSNUM_PHYSICAL_DISK_WRITE:
            
            diskID = (long)SystemCallData->Argument[0];
            
            switch(diskID){
                case(0):
                    diskLock = DISK_0_LOCK;
                    break;
                case(1):
                    diskLock = DISK_1_LOCK;
                    break;
                case(2):
                    diskLock = DISK_2_LOCK;
                    break;
                case(3):
                    diskLock = DISK_3_LOCK;
                    break;
                case(4):
                    diskLock = DISK_4_LOCK;
                    break;
                case(5):
                    diskLock = DISK_5_LOCK;
                    break;
                case(6):
                    diskLock = DISK_6_LOCK;
                    break;
                case(7):
                    diskLock = DISK_7_LOCK;
                    break;
                default:
                    break;
            }
            
            //check disk status
            mmio.Mode = Z502Status;
            mmio.Field1 = (long)SystemCallData->Argument[0];
            mmio.Field2 = mmio.Field3 = 0;
            MEM_READ(Z502Disk, &mmio);
            
            currentPcb->status = PROCESS_WAITTING;
            
            READ_MODIFY(diskLock, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
            diskEnQueue((long)SystemCallData->Argument[0],(long)SystemCallData->Argument[1],1,(char *)SystemCallData->Argument[2], currentPcb);
            READ_MODIFY(diskLock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
            
            if (mmio.Field2 == DEVICE_FREE){
//                printf("Disk free with diskID: %ld\n",mmio.Field1);
                mmio.Mode = Z502DiskWrite;
                mmio.Field1 = (long)SystemCallData->Argument[0];
                mmio.Field2 = (long)SystemCallData->Argument[1];
                mmio.Field3 = (long)SystemCallData->Argument[2];
                mmio.Field4 = 0;
               
                MEM_WRITE(Z502Disk, &mmio);
            }

            /***********   print   ***********/
            memset(&SPData, 0, sizeof(SP_INPUT_DATA));
            strcpy(SPData.TargetAction, "Disk_W");
            SPData.CurrentlyRunningPID = currentPcb->processID;
            
            if(head_r != NULL){
                SPData.TargetPID = head_r->pcb->processID;
            }else{
                SPData.TargetPID = currentPcb->processID;
            }
            
            SPData.NumberOfRunningProcesses = 0;
            
            SPData.NumberOfReadyProcesses = getNumofReadyProcess();   // Processes ready to run
            if(SPData.NumberOfReadyProcesses > 0){
                int i = 0;
                temp_r = head_r;
                
                while( temp_r !=NULL){
                    SPData.ReadyProcessPIDs[i] = temp_r->pcb->processID;
                    temp_r = temp_r->next;
                    i++;
                }
            }
            
            SPData.NumberOfTimerSuspendedProcesses = getNumofTimerQProcess();
            if(SPData.NumberOfTimerSuspendedProcesses > 0){
                int i = 0;
                temp_t = head;
                
                while(temp_t != NULL){
                    SPData.TimerSuspendedProcessPIDs[i] = temp_t->pcb->processID;
                    temp_t = temp_t->next;
                    i++;
                }
            }
            
            SPData.NumberOfDiskSuspendedProcesses = getNumofDiskQProcess(diskID);
            if(SPData.NumberOfDiskSuspendedProcesses >0){
                int i =0;
                switch(diskID){
                    case 0:
                        temp_d = head_d;
                        break;
                    case 1:
                        temp_d = head_d1;
                        break;
                    case 2:
                        temp_d = head_d2;
                        break;
                    case 3:
                        temp_d = head_d3;
                        break;
                    case 4:
                        temp_d = head_d4;
                        break;
                    case 5:
                        temp_d = head_d5;
                        break;
                    case 6:
                        temp_d = head_d6;
                        break;
                    case 7:
                        temp_d = head_d7;
                        break;
                    default:
                        break;
                }
                
                while(temp_d != NULL){
                    SPData.DiskSuspendedProcessPIDs[i] = temp_d->pcb->processID;
                    temp_d = temp_d->next;
                    i++;
                }
            }
            
            CALL(SPPrintLine(&SPData));
            /*********  end print ************/
            
            dispatcher();
            break;
            
        case SYSNUM_PHYSICAL_DISK_READ:
            
            diskID = (long)SystemCallData->Argument[0];
            
            switch(diskID){
                case(0):
                    diskLock = DISK_0_LOCK;
                    break;
                case(1):
                    diskLock = DISK_1_LOCK;
                    break;
                case(2):
                    diskLock = DISK_2_LOCK;
                    break;
                case(3):
                    diskLock = DISK_3_LOCK;
                    break;
                case(4):
                    diskLock = DISK_4_LOCK;
                    break;
                case(5):
                    diskLock = DISK_5_LOCK;
                    break;
                case(6):
                    diskLock = DISK_6_LOCK;
                    break;
                case(7):
                    diskLock = DISK_7_LOCK;
                    break;
                default:
                    break;
            }
            //check disk status
            mmio.Mode = Z502Status;
            mmio.Field1 = (long)SystemCallData->Argument[0];
            mmio.Field2 = mmio.Field3 = 0;
            MEM_READ(Z502Disk, &mmio);
            
            currentPcb->status = PROCESS_WAITTING;
            
            READ_MODIFY(diskLock, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
            diskEnQueue((long)SystemCallData->Argument[0],(long)SystemCallData->Argument[1],0,(char *)SystemCallData->Argument[2], currentPcb);
            READ_MODIFY(diskLock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
            
            if (mmio.Field2 == DEVICE_FREE){
    //            printf("Disk free with diskID: %ld\n",mmio.Field1);
                mmio.Mode = Z502DiskRead;
                mmio.Field1 = (long)SystemCallData->Argument[0];
                mmio.Field2 = (long)SystemCallData->Argument[1];
                mmio.Field3 = (long)SystemCallData->Argument[2];
                mmio.Field4 = 0;

                MEM_READ(Z502Disk, &mmio);
            }

            /***********   print   ***********/
            memset(&SPData, 0, sizeof(SP_INPUT_DATA));
            strcpy(SPData.TargetAction, "Disk_R");
            SPData.CurrentlyRunningPID = currentPcb->processID;
            
            if(head_r != NULL){
                SPData.TargetPID = head_r->pcb->processID;
            }else{
                SPData.TargetPID = currentPcb->processID;
            }
            
            SPData.NumberOfRunningProcesses = 0;
            
            SPData.NumberOfReadyProcesses = getNumofReadyProcess();   // Processes ready to run
            if(SPData.NumberOfReadyProcesses > 0){
                int i = 0;
                temp_r = head_r;
                
                while( temp_r !=NULL){
                    SPData.ReadyProcessPIDs[i] = temp_r->pcb->processID;
                    temp_r = temp_r->next;
                    i++;
                }
            }
            
            SPData.NumberOfTimerSuspendedProcesses = getNumofTimerQProcess();
            if(SPData.NumberOfTimerSuspendedProcesses > 0){
                int i = 0;
                temp_t = head;
                
                while(temp_t != NULL){
                    SPData.TimerSuspendedProcessPIDs[i] = temp_t->pcb->processID;
                    temp_t = temp_t->next;
                    i++;
                }
            }
            
            SPData.NumberOfDiskSuspendedProcesses = getNumofDiskQProcess(diskID);
            if(SPData.NumberOfDiskSuspendedProcesses >0){
                int i =0;
                switch(diskID){
                    case 0:
                        temp_d = head_d;
                        break;
                    case 1:
                        temp_d = head_d1;
                        break;
                    case 2:
                        temp_d = head_d2;
                        break;
                    case 3:
                        temp_d = head_d3;
                        break;
                    case 4:
                        temp_d = head_d4;
                        break;
                    case 5:
                        temp_d = head_d5;
                        break;
                    case 6:
                        temp_d = head_d6;
                        break;
                    case 7:
                        temp_d = head_d7;
                        break;
                    default:
                        break;
                }
                
                while(temp_d != NULL){
                    SPData.DiskSuspendedProcessPIDs[i] = temp_d->pcb->processID;
                    temp_d = temp_d->next;
                    i++;
                }
            }
            
            CALL(SPPrintLine(&SPData));
            /*********  end print ************/
            
            dispatcher();
            break;
            
        case SYSNUM_FORMAT:
            
            diskID = (long)SystemCallData->Argument[0];
            diskFormat(diskID);
            
            break;
            
        case SYSNUM_CHECK_DISK:
            
            break;
            
        default:
            printf("ERROR! call_type not recognized!\n");
            break;
    }
}                                               // End of svc

/************************************************************************
 osInit
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/

void osInit(int argc, char *argv[]) {
	void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
	INT32 i;
	MEMORY_MAPPED_IO mmio;
    void *test;
	// Demonstrates how calling arguments are passed thru to here

	printf("Program called with %d arguments:", argc);
	for (i = 0; i < argc; i++)
		printf(" %s", argv[i]);
	printf("\n");
	printf("Calling with argument 'sample' executes the sample program.\n");

	// Here we check if a second argument is present on the command line.
	// If so, run in multiprocessor mode
	if (argc > 2) {
		if ( strcmp( argv[2], "M") || strcmp( argv[2], "m")) {
		printf("Simulation is running as a MultProcessor\n\n");
		mmio.Mode = Z502SetProcessorNumber;
		mmio.Field1 = MAX_NUMBER_OF_PROCESSORS;
		mmio.Field2 = (long) 0;
		mmio.Field3 = (long) 0;
		mmio.Field4 = (long) 0;
		MEM_WRITE(Z502Processor, &mmio);   // Set the number of processors
		}
	} else {
		printf("Simulation is running as a UniProcessor\n");
		printf(
				"Add an 'M' to the command line to invoke multiprocessor operation.\n\n");
	}

	//          Setup so handlers will come to code in base.c

	TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR ] = (void *) InterruptHandler;
	TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR ] = (void *) FaultHandler;
	TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR ] = (void *) svc;

	//  Determine if the switch was set, and if so go to demo routine.

	PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
    
	if ((argc > 1) && (strcmp(argv[1], "sample") == 0)) {
		mmio.Mode = Z502InitializeContext;
		mmio.Field1 = 0;
		mmio.Field2 = (long) SampleCode;
		mmio.Field3 = (long) PageTable;

		MEM_WRITE(Z502Context, &mmio);   // Start of Make Context Sequence
		mmio.Mode = Z502StartContext;
		// Field1 contains the value of the context returned in the last call
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context

	} // End of handler for sample code - This routine should never return here

	//  By default test0 runs if no arguments are given on the command line
	//  Creation and Switching of contexts should be done in a separate routine.
	//  This should be done by a "OsMakeProcess" routine, so that
	//  test0 runs on a process recognized by the operating system.
    
    if (argc >1){
        if(strcmp( argv[1], "test1") == 0 ) {
            test = test1;
        }
        else if(strcmp( argv[1], "test2") == 0){
            test = test2;
        }
        else if(strcmp( argv[1], "test3") == 0){
            test = test3;
        }
        else if(strcmp( argv[1], "test4") == 0){
            test = test4;
        }
        else if(strcmp( argv[1], "test5") == 0){
            test = test5;
        }
        else if(strcmp( argv[1], "test6") == 0){
            test = test6;
        }
        else if(strcmp( argv[1], "test7") == 0){
            test = test7;
        }
        else if(strcmp( argv[1], "test8") == 0){
            test = test8;
        }
        else if(strcmp( argv[1], "test9") == 0){
            test = test9;
        }
        else if(strcmp( argv[1], "test10") == 0){
            test = test10;
        }
        else if(strcmp( argv[1], "test11") == 0){
            test = test11;
        }
        else if(strcmp( argv[1], "test12") == 0){
            test = test12;
        }
        else if(strcmp( argv[1], "test13") == 0){
            test = test13;
        }
        else if(strcmp( argv[1], "test14") == 0){
            test = test14;
        }
        else if(strcmp( argv[1], "test15") == 0){
            test = test15;
        }
        else if(strcmp( argv[1], "test16") == 0){
            test = test16;
        }
        else{
            test = test0;
        }
        
    }
    
	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = processCount;
	mmio.Field2 = (long) test;
	mmio.Field3 = (long) PageTable;
    processCount++;
	MEM_WRITE(Z502Context, &mmio);                      // Start this new Context Sequence
    currentPcb = (PCB*)malloc(sizeof(PCB));
    
    // construct pcb
    currentPcb->processID = 0;
    
    int size = strlen("Process0");
    currentPcb->processName = malloc(sizeof(char)*(size+1));
    strcpy(currentPcb->processName,"Process#0");
    
    currentPcb->status = RROCESS_RUNNING;
    currentPcb->context = mmio.Field1;
    currentPcb->parent = 0;
    currentPcb->pageTable = PageTable;
    addToPCBList(currentPcb);

/********** state print ***********/
    memset(&SPData, 0, sizeof(SP_INPUT_DATA));
    strcpy(SPData.TargetAction, "Create");
    SPData.CurrentlyRunningPID = 0;
    SPData.TargetPID = 0;
    SPData.NumberOfRunningProcesses = 0;
    
    SPData.NumberOfReadyProcesses = 0;
    
    CALL(SPPrintLine(&SPData));
/************ end print ***********/
    
    mmio.Mode = Z502StartContext;
	// Field1 contains the value of the context returned in the last call
	// Suspends this current thread
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);                      // Start up the context

}                                                       // End of osInit

/*
 terminate current process
 */
void terminateSelf( PCB *pcb){
    
    removePCB(pcb);
    processCount--;
    
    // if there is process in the queque, give a chance to finish.
    if(head_r!= NULL || head !=NULL || head_d!= NULL || head_d1!= NULL ||
       head_d2!= NULL || head_d3!= NULL || head_d4 != NULL || head_d5!=NULL
       || head_d6 != NULL || head_d7 != NULL){
        
        dispatcher();
    }
        }

void diskFormat(long diskID){
    
    MEMORY_MAPPED_IO mmio;
    char char_data[PGSIZE];
    
    char_data[0] = diskID;
    char_data[1] = 1;
    char_data[2] = 2;
    char_data[3] = 1;
    char_data[4] = 0;
    char_data[5] = 2;
    char_data[6] = 7;
    char_data[7] = 0;
    char_data[8] = 1;
    char_data[9] = 0;
    char_data[10] = 3;
    char_data[11] = 0;
    char_data[12] = '\0';
    char_data[13] = '\0';
    char_data[14] = '\0';
    char_data[15] = '\0';
    
    mmio.Mode = Z502DiskWrite;
    mmio.Field1 = diskID;
    mmio.Field2 = 0;
    mmio.Field3 = char_data;
    mmio.Field4 = 0;
    
    MEM_WRITE(Z502Disk, &mmio);
    
}

