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
static int top;
static int base;
static int seq;
static int winsize;

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
	seq = base;
	while((seq < (base+winsize))&&(seq<top)){
		tolayer3(A,Apkt[seq]);
		if(base == seq){
			starttimer(A,ertt);
		}
		seq++;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	int bflag = 0;
	if (!(check_chk(packet))){//Corrupted packet received
		return;
	}
	
	base = packet.acknum + 1;
	stoptimer(A);

	if(base != seq){
		starttimer(A,ertt);
		while((seq < (base+winsize))&&(seq<top)){
			tolayer3(A,Apkt[seq]);
			seq++;
		}		
	}	
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	for(int i = base; i < seq; i++){
		tolayer3(A,Apkt[i]);
		if(i == base){
			starttimer(A,ertt);
		}
	}		
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	ertt = 10.0;
	top = 1;
	base = 1;
	seq = 1;
	winsize = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	if(!(check_chk(packet))){ 
		tolayer3(B,Bpkt);
		return;
	}
	if(packet.seqnum == Bpkt.seqnum){
		tolayer5(B,packet.payload);
		Bpkt.seqnum++;
		Bpkt.acknum++;
		Bpkt.checksum = check_gen(Bpkt);
		tolayer3(B,Bpkt);
	}
	else{
		tolayer3(B,Bpkt);
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	Bpkt.seqnum = 1;
	Bpkt.acknum = 0;
	strcpy(Bpkt.payload,"11111111111111111111");
	Bpkt.checksum = check_gen(Bpkt);
;
}
