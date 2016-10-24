/****************************************************
 header file for disk.c
 It define a diskQueue structure and prototype of 
 diskEnqueue and diskDequeue functions.
 ****************************************************/

#ifndef disk_h
#define disk_h

#include    "process.h"

#define      DISK_0_LOCK               0x7FE00003
#define      DISK_1_LOCK               0x7FE00004
#define      DISK_2_LOCK               0x7FE00005
#define      DISK_3_LOCK               0x7FE00006
#define      DISK_4_LOCK               0x7FE00007
#define      DISK_5_LOCK               0x7FE00008
#define      DISK_6_LOCK               0x7FE00009
#define      DISK_7_LOCK               0x7FE0000A

#define         DISK_INTERRUPT_DISK2            (short)7
#define         DISK_INTERRUPT_DISK3            (short)8
#define         DISK_INTERRUPT_DISK4            (short)9
#define         DISK_INTERRUPT_DISK5            (short)10
#define         DISK_INTERRUPT_DISK6            (short)11
#define         DISK_INTERRUPT_DISK7            (short)12


struct diskQueue {
    PCB *pcb;
    long diskID;
    long sector;
    int read_write;         //read: =0; write: =1
    char* address;
    struct diskQueue *next;
};

/*
 * Prototype of disk Queue functions
 */
void diskEnQueue(long diskID, long sector, int read_write, char* address,PCB *pcb);
PCB* diskDeQueue(long diskID);
int getNumofDiskQProcess( long DiskID);
#endif /* disk_h */
