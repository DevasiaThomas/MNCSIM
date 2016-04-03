#include "../include/simulator.h"

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */

static int base, nextSeqNum, winsize, top;
static struct pkt Apkt[1001];
static float ertt = 13.0;
static int expectedSeqNum;
static int count;
static struct pkt ack_packet;

int check_gen(char data[20],int seq, int ack){
	int i=0;
	int checksum = 0;
	for(i=0;i<20;i++){
		checksum+=data[i];
	}
	checksum+=seq+ack;
	return checksum;
}

void A_output(message)
  struct msg message;
{
	strncpy(Apkt[top].payload,message.data,20);
	Apkt[top].seqnum = top;
	Apkt[top].checksum = check_gen(message.data,top,0);
	if(nextSeqNum<base+winsize){
		//printf("data sent : %.*s\n",20,Apkt[nextSeqNum].payload);
		tolayer3(0, Apkt[nextSeqNum]);
		if(base==nextSeqNum)
			starttimer(0,ertt);
		nextSeqNum++;
	}
	top++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{

	if(packet.checksum == packet.acknum){
		if(packet.acknum < base){
			int i=base;
			int j=0;
			stoptimer(0);
			int c=0;
			for(;j<count;j++){
			for(;i<nextSeqNum;i++){
				if(c>3){
					i=base;
					c++;
				}
				tolayer3(0,Apkt[i]);
			}
			}
			starttimer(0,ertt);
			return;
			count++;
		}
		if(packet.acknum > base){
			base = packet.acknum+1;
			count=2;
		}
		if(base==nextSeqNum){
			stoptimer(0);/*
			if(nextSeqNum < top){
				for(;nextSeqNum<top;nextSeqNum++){
					if(nextSeqNum >= base+winsize)
						break;
					tolayer3(0,Apkt[nextSeqNum]);
				}
				starttimer(0,ertt);
			}*/
		}
		else{
			stoptimer(0);
			starttimer(0,ertt);
		}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	////printf("Time out\n");
	int i=0;
	starttimer(0,ertt);
	for(i=base;i<nextSeqNum;i++){
		////printf("The data retransmission: %.*s\n",20,Apkt[i].payload);
		tolayer3(0, Apkt[i]);
	}
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	base=1;
	nextSeqNum=1;
	top=1;
	winsize=getwinsize();
	count =2;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	int check = check_gen(packet.payload,packet.seqnum,packet.acknum);
	if((packet.checksum == check) && (packet.seqnum==expectedSeqNum)){
		char data[20];		
		memcpy(data,packet.payload,20);				
		tolayer5(1,data);			
		////printf("The data received: %.*s\n",20,data);
		ack_packet.acknum = expectedSeqNum;
		ack_packet.checksum = expectedSeqNum;
		tolayer3(1,ack_packet);
		expectedSeqNum++;
		}
	else{
		////printf("Reciever problem\n");
		tolayer3(1,ack_packet);
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expectedSeqNum =1;
	ack_packet.acknum=0;
}


