#include "../include/simulator.h"


#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<math.h>

#define A 0
#define B 1
#define ALPHA 0.125
#define BETA 0.25


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

static struct pkt Apkt[1500];//already processed packets
static struct pkt Bpkt[1500];
static struct pkt Bbpkt;
static float timer[1500];
static float ertt;  // estimated RTT
static float rtime;
static int top;
static int Abase;
static int Bbase;
static int seq;
static int tseq;
static int maxack;
static int winsize;
//static int Bstart;


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
int check_gen(struct pkt pack){
	pack.checksum = pack.seqnum + pack.acknum;
	for(int i=0; i<20; i++){
		pack.checksum += (int)pack.payload[i];
	}
	return pack.checksum;
}
int check_chk(struct pkt pack){
	int chk = check_gen(pack);
	if(chk == pack.checksum)
		return 1;
	else
		return 0;
}

void A_output(message)
  struct msg message;
{
	if(top >= 1500){
		return;
	}
	strcpy(Apkt[top].payload,message.data);
	Apkt[top].seqnum = seq;
	Apkt[top].acknum = -1;
	Apkt[top].checksum = check_gen(Apkt[top]);
	top++;
	seq=Abase;
	while((seq < (Abase+winsize))&&(seq<top)){
		if(Apkt[seq].seqnum != Apkt[seq].acknum){
			printf("Sending Seq %d\n", seq); 
			tolayer3(A,Apkt[seq]);
			if(Abase == seq){
				starttimer(A,ertt);
				tseq = seq;
			}	
			timer[seq] = get_sim_time();
		}
		seq++;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	int flg = 0;
	int ttseq = -1;
	if(!(check_chk(packet))){//Corrupted ack packet received
		return;
	}
	printf("Ack received %d\n", packet.acknum);
	printf("Current Base %d\n", Abase);
	Apkt[packet.acknum].acknum = packet.acknum;
	//printf("Ack recieved %d\n", packet.acknum);

	if(packet.acknum > maxack){
		maxack = packet.acknum;
		printf("Max Ack %d\n",maxack);
	}
	if(packet.acknum < Abase){
		printf("acknum less that base\n");
		flg = 1;
	}
	
	if(packet.acknum == tseq){
		printf("Stopping timer %d\n",tseq);
		stoptimer(A);
	}
	
	while(Apkt[Abase].acknum == Apkt[Abase].seqnum){  
		Abase++;
	}
	printf("New Base %d\n",Abase);

	if(!flg){
		float min;
		if(Abase != seq){
			min = timer[Abase];
			for(int i = Abase; ((i < (Abase+winsize))&&(i<seq)); i++){
				if(Apkt[i].acknum != Apkt[i].seqnum){
					printf("Seq num %d ",i);
					rtime =get_sim_time();
					if( (i < maxack) || ((rtime-timer[i]) >= ertt)){
						printf("resent\n");
						tolayer3(A,Apkt[i]);
						timer[i] = get_sim_time();
					}
					else{
						if(timer[i] < min){
							printf("used to set min\n");
							min = timer[i];
							ttseq = i;
						}
					}
				}
			}
		}
	}
	else{
		for(int i = Abase; i < maxack; i++){
			if(Apkt[i].acknum != Apkt[i].seqnum){
				//printf("Retransmitting from 2input %d\n",i);
				tolayer3(A,Apkt[i]);
				timer[i] = get_sim_time();
			}
		}
	}
	if((packet.acknum == tseq) && (ttseq != -1)){
		tseq = ttseq;
		printf("Seq num %d ",tseq);
		printf("Timer set in input %8.4f\n",(rtime-timer[tseq]));
		starttimer(A,(rtime - timer[tseq]));
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	//printf("TimerInterrupt %d\n",tseq);
	float min = timer[((Abase==tseq)?(Abase+1):Abase)];
	int ttseq = -1;
	tolayer3(A,Apkt[tseq]);
	timer[tseq] = get_sim_time();
	for(int i = Abase; ((i < (Abase+winsize))&&(i<seq)); i++){
		if(i == tseq){
			continue;
		}
		if(Apkt[i].acknum != Apkt[i].seqnum){
			printf("ISR Seq num %d ",i);
			rtime =get_sim_time();
			if( (i < maxack) || ((rtime-timer[i]) >= ertt) ){
				printf("resent\n");
				tolayer3(A,Apkt[i]);
				timer[i] = get_sim_time();
			}
			else{
				if(timer[i] < min){
					printf("used to set min\n");
					min = timer[i];
					ttseq = i;
				}
			}
		}
	}
	if(ttseq != -1){
		tseq = ttseq;
		printf("Seq num %d ",tseq);
		printf("Timer set in ISR %8.4f\n",(rtime-timer[tseq]));
		starttimer(A,(rtime - timer[tseq]));
	}
	else{
		printf("Seq num %d ",tseq);
		printf("Timer set in ISR %8.4f\n",(ertt));
		starttimer(A,ertt);
	}
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	ertt = 12.0;
	for(int i = 0; i < 1500; i++){
		Apkt[i].seqnum = i;
		Apkt[i].acknum = -1;
	}
	top = 1;
	Abase = 1;
	tseq = 1;
	seq = 1;
	maxack = -1;
	winsize = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	if(!(check_chk(packet))){//Corrupted ack packet received
		tolayer3(B,Bbpkt);
		return;
	}
	printf("Packet number received %d\n",packet.seqnum);
	if((packet.seqnum < Bbase+winsize)){
		packet.acknum = packet.seqnum;
		Bpkt[packet.acknum] = packet;
		Bpkt[packet.acknum].checksum = check_gen(Bpkt[packet.acknum]);
		tolayer3(B,Bpkt[packet.acknum]);
		while(Bpkt[Bbase].seqnum == Bpkt[Bbase].acknum){
			tolayer5(B,Bpkt[Bbase].payload);
			Bbase++;
		}
		Bbpkt = Bpkt[(Bbase-1)];
	}	
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	winsize = getwinsize();
	strcpy(Bbpkt.payload,"11111111111111111111");
	Bbpkt.seqnum = 0;
	Bbpkt.acknum = 0;
	Bbpkt.checksum = check_gen(Bbpkt);
	Bbase=1;
	for(int i = 0; i < 1500; i++){
		Bpkt[i].seqnum = i;
		Bpkt[i].acknum = -1;		
	}
}
