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

static int Abase, nextSeqNum, winsizeA, top, winsizeB, Bbase;
static struct pkt Apkt[1001];
static char Bmsg[1001][20];
static int Btrack[1001];
static float timer[1001];
static float ertt;
static int timer_state,timer_holder,maxack;
static struct pkt ack_packet;

int check_gen(char data[20],int seq, int ack)
{
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
	if(nextSeqNum<Abase+winsizeA){
		//printf("data sent : %s\n",Apkt[nextSeqNum].payload);
		tolayer3(0, Apkt[nextSeqNum]);
		timer[nextSeqNum] = get_sim_time();
		if(timer_state){
			starttimer(0,ertt);
			timer_state = 0;
			timer_holder=nextSeqNum;	
		}
		nextSeqNum++;
	}
	top++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	if(packet.checksum == packet.acknum)
	{
		timer[packet.acknum] = -278.0;
		if(packet.acknum==Abase){
			while(timer[Abase]==-278.0){
				Abase++;
			} 			
		}
		if(packet.acknum>maxack){
			maxack=packet.acknum;
		}
		if(packet.acknum<Abase){
			int i=Abase;
			for(;i<maxack;i++){
				if(timer[i]>0){
					tolayer3(0,Apkt[i]);
				}
			}
		}
		if(packet.acknum == timer_holder){
			stoptimer(0);
			float min = 0.0;
			int k=0;
			for(k=Abase;k<nextSeqNum;k++){
				if(min==(float)0 && timer[k]>0){
					min = timer[k];
					timer_holder=k;
				}
				if(timer[k]<min && timer[k]>0){
					min = timer[k];
					timer_holder=k;
				}
			}
			float tvalue=0;
			//if(min!=0) 
			tvalue = ertt-(get_sim_time()-min);
			//if(min!=0)
			//printf("tvalue : %f\n", tvalue);
			if(min!=0 && tvalue>0.0)
				starttimer(0,tvalue);
			else {
				timer_state =1;
				//starttimer(0,ertt);
				//timer_holder=nextSeqNum;
			}	
		}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	if(timer[timer_holder]>=0){
		tolayer3(0,Apkt[timer_holder]);
		timer[timer_holder] = get_sim_time();
	}
	float min = 0.0;
	int k=0;
	for(k=Abase;k<nextSeqNum;k++){
		if(min==(float)0 && timer[k]>0){
			min = timer[k];
			timer_holder=k;
		}
		if(timer[k]<min && timer[k]>0){
			min = timer[k];
			timer_holder=k;
		}
	}
	float tvalue=0;
	//if(min!=0) 
	tvalue = ertt-(get_sim_time()-min);
	//if(min!=0)
	//printf("tvalue : %f\n", tvalue);
	if(min!=0 && tvalue>0.0)
		starttimer(0,tvalue);
	else {
		timer_state =1;
		//starttimer(0,ertt);
		//timer_holder=nextSeqNum;
	}
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	Abase=1;
	nextSeqNum=1;
	top=1;
	timer_holder=1;
	ertt=20;
	maxack=0;
	winsizeA=getwinsize();
	int j=0;
	for(j=0;j<1001;j++){
		timer[j]=-23.0;
	}
	timer_state=1;
}
/* Note that with simplex transfer from a-to-B, there is no B_output() */



/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	int check = check_gen(packet.payload,packet.seqnum,packet.acknum);

	if((packet.checksum == check)){
		if(Btrack[packet.seqnum]==-1){
			Btrack[packet.seqnum]=1;
			strncpy(Bmsg[packet.seqnum],packet.payload,20); 
			//printf("data recieved : %.*s\n",20,Bmsg[packet.seqnum]);
		}
		ack_packet.acknum = packet.seqnum;
		ack_packet.checksum = packet.seqnum;
		if(Bbase+winsizeB>ack_packet.acknum)
			tolayer3(1,ack_packet);
		int i=0,j=0;
		for(i=Bbase;i<Bbase+winsizeB;i++){
			if(Btrack[i]==1){
				j++;
				tolayer5(1,Bmsg[i]);
				//printf("data recieved : %.*s\n",20,Bmsg[i]);
				Btrack[i]=0;
			}
			else{
				break;
			}
		}
		Bbase+=j;
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
Bbase=1;
ack_packet.acknum=0;
int j=0;
for(j=0;j<1001;j++){
	Btrack[j]=-1;
}
	winsizeB=getwinsize();
}
