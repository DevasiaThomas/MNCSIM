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
static struct pkt Apkt;//already processed packets
static struct pkt Bpkt;
static float ertt;  // estimated RTT
static float drtt;  // deviation in RTT
static float stime; // time when packet sent
static float rtime; // time when packet recieved
static int resend;


/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/
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
/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
	if(Apkt.seqnum != Apkt.acknum){
		printf("Ack not here yet \n");
		return;
	}
	else{
		//prepare packet to send
		struct pkt spkt;
		message.data[19] = '\0';
		strcpy(spkt.payload,message.data);
		spkt.seqnum = abs(Apkt.seqnum -1);
		spkt.acknum = Apkt.acknum;
		spkt.checksum = check_gen(spkt);
		Apkt = spkt; // Changing state
		//now send and start timer
		resend = 0;
		tolayer3(A,spkt);
		starttimer(A,(ertt + 4*drtt));
		stime = get_sim_time(); // for re-calculating rtt;
	}		
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	if((packet.acknum == Apkt.acknum) || (!(check_chk(packet)))){ //packet that was sent got corrupted || Ack corrupted
		printf(" Bad stuff at A\n");
		stoptimer(A);
		resend = 1;
		tolayer3(A,Apkt);
		starttimer(A,(ertt + 4*drtt));
		return;
	}
	//Correct Case
	stoptimer(A);
	rtime = get_sim_time();
	Apkt.acknum = packet.acknum;
	Apkt.checksum = check_gen(Apkt);
	//RTT estimation
	if(!resend){
		ertt = (1-ALPHA)*ertt +	ALPHA*(fabs(rtime-stime)) ;
		drtt = (1-BETA)*drtt + BETA*(fabs(fabs(rtime-stime) - ertt));
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf(" Timer gone wrong\n");
	resend = 1;
	//ertt = 1.5 * ertt;
	tolayer3(A,Apkt);
	starttimer(A,(ertt + 4*drtt));
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	Apkt.seqnum = 1;
	Apkt.acknum = 1;
	ertt = 15.0;
	drtt = 0;
	resend = 0;
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
	if(packet.seqnum == Bpkt.seqnum){ // Corrupted Packet recived but somehow checksum works out or timer expired and resent
		printf(" Bad stuff at B2\n");
		tolayer3(B,Bpkt);
		return;
	}
	//Expected Sequence number received
	tolayer5(B,packet.payload);
	Bpkt = packet;//Changed state
	Bpkt.acknum = Bpkt.seqnum;
	Bpkt.checksum = check_gen(Bpkt);
	tolayer3(B,Bpkt);
	
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	Bpkt.seqnum = 1;
	Bpkt.acknum = 1;
	Bpkt.payload[0] = '\0';
}
