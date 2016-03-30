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
static float timer[1500];
static float ertt;  // estimated RTT
static float drtt;  // deviation in RTT
static float stime; // time when packet sent
static float rtime; // time when packet recieved
static int resend[1500];
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
	for(int i=0; pack.payload[i]!='\0'; i++){
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
	message.data[19]= '\0';
	strcpy(Apkt[top].payload,message.data);
	Apkt[top].seqnum = seq;
	Apkt[top].acknum = -1;
	Apkt[top].checksum = check_gen(Apkt[top]);
	top++;
	while((seq < (Abase+winsize))&&(seq<top)){
		resend[seq] = 0;
		tolayer3(A,Apkt[seq]);
		//printf("Sent seq: %d\n",seq);
		if(Abase == seq){
			starttimer(A,(ertt + 4*drtt));
			tseq = seq;
		}	
		timer[seq] = get_sim_time();
		seq++;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	rtime = get_sim_time();
	int flg = 0;
	
	if(!(check_chk(packet))){//Corrupted ack packet received
		return;
	}

	if(!resend[packet.acknum]){
		ertt = (1-ALPHA)*ertt +	ALPHA*(fabs(rtime-timer[packet.acknum])) ;
		drtt = (1-BETA)*drtt + BETA*(fabs(fabs(rtime-timer[packet.acknum]) - ertt));
		//printf("ertt :%8.4f drtt:%8.4f\n",ertt,drtt);
	}

	Apkt[packet.acknum].acknum = packet.acknum;
	//printf("Ack recieved %d\n", packet.acknum);

	if(packet.acknum > maxack){
		maxack = packet.acknum;
	}
	if(packet.acknum > Abase){
		flg =1;
	}
	
	if(packet.acknum == Abase){
		stoptimer(A);
	}
	while(Apkt[Abase].acknum == Apkt[Abase].seqnum){  
		//printf("Advancing base\n"); 
		Abase++;
	}
	if(packet.acknum < Abase){
		float min;
		if(Abase != seq){
			min = timer[Abase];
			for(int i = Abase; ((i < (Abase+winsize))&&(i<seq)); i++){
				if(Apkt[i].acknum != Apkt[i].seqnum){
					rtime =get_sim_time();
					if( (i < maxack) || ((rtime-timer[i]) >= (ertt + 4*drtt))){
						//printf("Retransmitting from 1input %d\n",i);
						tolayer3(A,Apkt[i]);
						//starttimer(A,(ertt + 4*drtt));
						timer[i] = get_sim_time();
						resend[i] = 1;
					}
					else{
						if(timer[i] < min){
							min = timer[i];
							tseq = i;
						}
					}
				}
			}
			//printf("Input Min:%8.8f tseq:%d\n", min,tseq);
			rtime = get_sim_time();
			starttimer(A,(fabs((ertt + 4*drtt) - fabs(rtime - min))));
		}
	}
	else if(flg == 1){
		for(int i = Abase; i < maxack; i++){
			if(Apkt[i].acknum != Apkt[i].seqnum){
				//printf("Retransmitting from 2input %d\n",i);
				tolayer3(A,Apkt[i]);
				timer[i] = get_sim_time();
				resend[i] = 1;
			}
		}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	//printf("TimerInterrupt %d\n",tseq);
	float min = timer[((Abase==tseq)?(Abase+1):Abase)];
	int count = 0;
	tolayer3(A,Apkt[tseq]);
	timer[tseq] = get_sim_time();
	resend[tseq] = 1;
	for(int i = Abase; ((i < (Abase+winsize))&&(i<seq)); i++){
		if(i == tseq){
			continue;
		}
		count++;
		if(Apkt[i].acknum != Apkt[i].seqnum){
			rtime =get_sim_time();
			if( (i < maxack) || ((rtime-timer[i]) >= (ertt + 4*drtt)) ){
				//printf("Retransmitting from timer %d\n",i);
				tolayer3(A,Apkt[i]);
				//starttimer(A,(ertt + 4*drtt));
				timer[i] = get_sim_time();
				resend[i] = 1;
			}
			else{
				if(timer[i] < min){
					min = timer[i];
					tseq = i;
				}
			}
		}
	}
	//printf("Timer Min:%8.8f tseq:%d\n", min,tseq);
	rtime = get_sim_time();
	//if(count > 0){
		//printf("Newtimer1:%8.8f\n",(fabs((ertt + 4*drtt) - fabs(rtime - min))>2)?(fabs((ertt + 4*drtt) - fabs(rtime - min))):2.0);
		//starttimer(A,(fabs((ertt + 4*drtt) - fabs(rtime - min))));
	//}
	//else{
		//printf("Newtimer2:%8.8f\n",(fabs(ertt + 4*drtt)));
		starttimer(A,(fabs(ertt + 4*drtt))/2.0);
	//}
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	ertt = 14.0;
	drtt = 0;
	for(int i = 0; i < 1500; i++){
		resend[i] = 0;
		Apkt[i].seqnum = i;
		Apkt[i].acknum = -1;
	}
	top = 0;
	Abase = 0;
	seq = 0;
	maxack = -1;
	winsize = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	packet.payload[19] ='\0';
	if(!(check_chk(packet))){//Corrupted ack packet received
		return;
	}
	if((packet.seqnum >= Bbase) && (packet.seqnum < Bbase+winsize)){
		//printf("Packet received %d\n",packet.seqnum);
		packet.acknum = packet.seqnum;
		Bpkt[packet.acknum] = packet;
		//tolayer5(B,packet.payload);
		Bpkt[packet.acknum].checksum = check_gen(Bpkt[packet.acknum]);
		//printf("Ack %d\n",packet.acknum);
		tolayer3(B,Bpkt[packet.acknum]);
		while(Bpkt[Bbase].seqnum == Bpkt[Bbase].acknum){
			//printf("Sent %d to layer5  \n",Bbase);
			tolayer5(B,Bpkt[Bbase].payload);
			Bbase++;
		}
	}	
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	winsize = getwinsize();
	Bbase=0;
	for(int i = 0; i < 1500; i++){
		Bpkt[i].seqnum = i;
		Bpkt[i].acknum = -1;		
	}
}
