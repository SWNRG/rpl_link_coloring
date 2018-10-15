#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "net/rpl/rpl.h"
#include "dev/leds.h"
#include "simple-udp.h"
#include <stdio.h>
#include <string.h>
#include "dev/button-sensor.h"

#include <core/sys/rtimer.h>

#define UDP_PORT 1234



/**** BOTH centrally controlled in project-conf.h *******/
#ifndef DAG_RESET_START_CONF
// remember: has to do with SEND_INTERVAL_CONF
#define DAG_RESET_START 10
#endif 

#ifndef DAG_RESET_STOP_CONF
// remember: has to do with SEND_INTERVAL_CONF
#define DAG_RESET_STOP 40
#endif 
/******************************************************/



#ifdef SEND_INTERVAL_CONF // In project-conf.h
#define SEND_INTERVAL		SEND_INTERVAL_CONF
#else //Only in case you want to localy define it
#define
#define SEND_INTERVAL		60*CLOCK_SECONDS
#endif

// this has to be defined in EVERY station for randomness
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))


extern uint8_t node_color;
extern uint8_t aggregator;



rtimer_clock_t sent_time; // to me
rtimer_clock_t time_now; //rtimer time
 
static unsigned int message_number;

static int counter=0;
  
rpl_dag_t *cur_dag; // to use in local_repair()

static struct simple_udp_connection unicast_connection;


// new way, looks more stable
rtimer_clock_t *rtime_new_sent;
rtimer_clock_t *rtime_new_recv;
	
/*---------------------------------------------------------------------------*/
PROCESS(sender_node_process, "Sender node process");
PROCESS(sender_button_press_process, "Sender node button press process");
AUTOSTART_PROCESSES(&sender_node_process, &sender_button_press_process);
/*---------------------------------------------------------------------------*/



static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/


static void reset_dag(unsigned int start, unsigned int end){

	//printf("RTT# local repair scheduled:%d. %d\n",start,end);

	if(counter == start){ //One round after global repair
		//printf("RTT# In p Node Calling local repair...\n"); // Msg in rpl.dag.c
		cur_dag = rpl_get_any_dag(); //get the current dag
		rpl_local_repair(cur_dag->instance);
		rpl_recalculate_ranks(); // IT DOES NOT SEEM TO WORK
	}

	if(counter == end){ //One round after global repair
		//printf("RTT# In p Node Calling local repair...\n"); // Msg in rpl.dag.c
		cur_dag = rpl_get_any_dag(); //get the current dag
		rpl_local_repair(cur_dag->instance);
		rpl_recalculate_ranks(); // IT DOES NOT SEEM TO WORK	
	}
}
/*---------------------------------------------------------------------------*/


static void sender(unsigned int nodeID){

	char buf[20];
	uip_ipaddr_t addr;

	// Works correctly sending to any node: Change only nodeID
	uip_ip6addr(&addr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0xc30c, 0, 0, nodeID);

	printf("DATA: In p Sending unicast MSG no %d to ",message_number);
	uip_debug_ipaddr_print(&addr);
	printf("\n");
	sprintf(buf, "Message %d", message_number);
	message_number++;

	//new way
   rtime_new_sent = rtimer_arch_now();

/**************** SENDING UDP UNICAST TO &addr *******************/
	rtimer_init();
	sent_time = RTIMER_NOW();
	simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, &addr);
/****************************************************************/
}
/*----------------------------------------------------------------------*/ 
 
    
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

/****************** Testing NEW rtimer() **********************/
   rtime_new_recv = rtimer_arch_now();
   //printf("rt_new_sent: %u, rt_new_recv: %u\n", rtime_new_sent, rtime_new_recv);
   rtime_new_recv = rtime_new_recv - rtime_new_sent;
	printf("RTT rt_new_diff: %u\n", rtime_new_recv);
  	timer_restart(rtime_new_sent); //restart the timer every time
/***************************************************************?	
	
/********** CHECK THE TIMES AGAIN, BUT IT SEEMS TO WORK ********/
   time_now = RTIMER_NOW(); //maybe it was restarted

  	if(RTIMER_CLOCK_LT(time_now,sent_time)){// if arg1<arg2
		printf("RTIMER restarted...\n");
		printf("time_now: %d\n",time_now);
		printf("RTIMER_ARCH_SECOND:%u\n",RTIMER_ARCH_SECOND);

// the variable prints as negative ???
		time_now-=RTIMER_ARCH_SECOND; 
	}
   time_now -= sent_time;


  printf("DATA: In p Sender received back: '%s'. RTT:%d",data,time_now);
  printf(", last local Msg Num: %d\n", message_number-1);
  
/****************** EASY TO EXTRACT DATA ************************/
  printf("RTT# %d\n", time_now); // ready for extraction
/****************************************************************/  
  
  // Trying to extract the message number out of data
  char gg = *data;
  char *tt="Message";

  //printf("DATA tt: '%s'\n",tt);
}
/*--------------------------------------------------------------------*/


PROCESS_THREAD(sender_button_press_process, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_PAUSE();

  SENSORS_ACTIVATE(button_sensor);

  while(1){
    	PROCESS_YIELD();
		if (ev == sensors_event && data == &button_sensor) {
			printf("DATA: Node button pressed: Calling local repair...\n");
			rpl_dag_t *d = rpl_get_any_dag(); //get the current dag
			rpl_local_repair(d->instance);
		}
  }
  PROCESS_END();
}




PROCESS_THREAD(sender_node_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;

  PROCESS_BEGIN();
  
/******************** NODE COLOR LC **********************/
  node_color = RPL_DAG_MC_LC_RED; //Node color = 5
/*********************************************************/
  
  set_global_address();

  printf("RTT# local repair scheduled:%d. %d\n",
  		DAG_RESET_START+1, DAG_RESET_STOP+1);
  
  
  
  //test to turn of receiver
  
  //simple_udp_register(&unicast_connection, UDP_PORT,
     //                 NULL, UDP_PORT, receiver);

 /* 60*CLOCK_SECOND should print for RM090 every one (1) min
  * etimer_set(&periodic_timer, 60*CLOCK_SECOND);
  * By using this, you use randomness, and sends many msgs
  * etimer_set(&periodic_timer, SEND_TIME / 2);
  * Be careful: it affects the counter
  * SEND_INTERVAL is cetrally defined in project-conf.h
  */
  etimer_set(&periodic_timer, SEND_INTERVAL);
  
  while(1) {
	 
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

/******************* SENDING MESSAGES ************************/
	 //sender(1); // send message: NUM IS THE NODE ID
/*************************************************************/
	
	// local repairs DO NOT SEEM TO WORK without a global repair...

	/*local repair: 
	 * Once at the 1st param, Once again at the 2nd
	 * (+1) so it follows the global repair
	 */
   reset_dag(DAG_RESET_START+1,DAG_RESET_STOP+1);		
	
	
    //printf("R:%d, Leaf MODE: %d\n",counter,rpl_get_mode());
	if(counter%11 == 0){
		//printf("R:%d, Node COLOR: %d\n",counter,node_color);
	}	

    //printf("R:%d, udp_sent:%d\n",counter,uip_stat.udp.sent);
    //printf("R:%d, udp_recv:%d\n",counter,uip_stat.udp.recv);	
    
    //printf("R:%d, icmp_sent:%d\n",counter,uip_stat.icmp.sent);
    //printf("R:%d, icmp_recv:%d\n",counter,uip_stat.icmp.recv);
	
	
/********************** Nothing beyond this point *************/    
    counter++;
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

