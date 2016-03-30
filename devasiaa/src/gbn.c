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
static struct pkt Bpkt;
static float ertt;  // estimated RTT
static float drtt;  // deviation in RTT
static float stime; // time when packet sent
static float rtime; // time when packet recieved
static int resend;
static int top;
static int base;
static int seq;
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
	while((seq < (base+winsize))&&(seq<top)){
		resend = 0;
		tolayer3(A,Apkt[seq]);
		if(base == seq){
			starttimer(A,(ertt + 4*drtt));
			stime = get_sim_time(); // for re-calculating rtt;
		}
		seq++;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	if (!(check_chk(packet))){//Corrupted packet received
		return;
	}

	if(packet.acknum < base){//1st packet in window was lost/corrupted
		stoptimer(A);
		resend = 1;
		for(int i = base; i < seq; i++){
			tolayer3(A,Apkt[i]);
			if(i == base){
				starttimer(A,(ertt + 4*drtt));
			}
		}	
		return;
	}

	base = packet.acknum + 1;
	stoptimer(A);

	if(base != seq){
		starttimer(A,(ertt + 4*drtt));	
	}
	else{
		if(!resend){
			ertt = (1-ALPHA)*ertt +	ALPHA*(fabs(rtime-stime)) ;
			drtt = (1-BETA)*drtt + BETA*(fabs(fabs(rtime-stime) - ertt));
		}
	}	
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	resend = 1;
	for(int i = base; i < seq; i++){
		tolayer3(A,Apkt[i]);
		if(i == base){
			starttimer(A,(ertt + 4*drtt));
		}
	}		
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	ertt = 13.0;
	drtt = 0;
	resend = 0;
	top = 0;
	base = 0;
	seq = 0;
	winsize = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	Bpkt.checksum = check_gen(Bpkt);
	packet.payload[19] = '\0';

	if(!(check_chk(packet))){ // corrupted packet received
		printf(" Bad stuff at B1\n");
		tolayer3(B,Bpkt);
		return;
	}

	if(packet.seqnum == Bpkt.seqnum){
		tolayer5(B,packet.payload);
		Bpkt.seqnum++;
		Bpkt.acknum++;
		//if(packet.sequm == (Bstart + winsize)){
		Bpkt.checksum = check_gen(Bpkt);
		tolayer3(B,Bpkt);
		//Bstart = Bpkt.seqnum;
		//}
	}
	else{
		tolayer3(B,Bpkt);
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	Bpkt.seqnum = 0;
	Bpkt.acknum = -1;
	Bpkt.payload[0] = '\0';
	//Bstart = 0;
}
