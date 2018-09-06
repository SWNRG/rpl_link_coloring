#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "simple-udp.h"
#include "net/rpl/rpl.h"
#include "dev/leds.h"
#include <stdio.h>
#include <string.h>
#include "dev/button-sensor.h"

#define UDP_PORT 1234


#ifndef DAG_RESET_START_CONF
// remember: has to do with SEND_INTERVAL_CONF
#define DAG_RESET_START 10
#endif 

#ifndef DAG_RESET_STOP_CONF
// remember: has to do with SEND_INTERVAL_CONF
#define DAG_RESET_STOP 40
#endif 

#ifdef SEND_INTERVAL_CONF // In project-conf.h
#define SEND_INTERVAL		SEND_INTERVAL_CONF
#else //Only in case you want to localy define it
#define
#define SEND_INTERVAL		60*CLOCK_SECONDS
#endif

// this has to be defined in EVERY station for randomness
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static uint32_t sent_time=0; // to me
static unsigned int message_number;
static int counter=0;

rpl_dag_t *cur_dag; // to use in local_repair()

static struct simple_udp_connection unicast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(receiver_node_process, "Receiver node");
AUTOSTART_PROCESSES(&receiver_node_process);
/*---------------------------------------------------------------------------*/

static uip_ipaddr_t *
set_global_address(void)
{
  static uip_ipaddr_t ipaddr;
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

  return &ipaddr;
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

  //printf("DATA: In p '%s' received from ",data);
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
  
/*********** Sending back the received message *********************/

  printf("In p sending '%s' BACK to ",data);
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
  simple_udp_sendto(&unicast_connection, data, strlen(data) + 1, sender_addr);

/********************************************************************/
}
/*---------------------------------------------------------------------------*/


static void reset_dag(unsigned int start, unsigned int end){

	//printf("RTT# local repair scheduled:%d. %d\n",start,end);

	if(counter == start){ //One round after global repair
		printf("RTT# In p Node Calling local repair...\n");
		cur_dag = rpl_get_any_dag(); //get the current dag
		rpl_local_repair(cur_dag->instance);
		rpl_recalculate_ranks(); // IT DOES NOT SEEM TO WORK
	}

	if(counter == end){ //One round after global repair
		printf("RTT# In p Node Calling local repair...\n");
		cur_dag = rpl_get_any_dag(); //get the current dag
		rpl_local_repair(cur_dag->instance);
		rpl_recalculate_ranks(); // IT DOES NOT SEEM TO WORK
	}
}
/*------------------------------------------------------------------*/



PROCESS_THREAD(receiver_node_process, ev, data)
{
	static struct etimer periodic_timer;
	static struct etimer send_timer;


/******************** NODE COLOR LC *****************************/
	node_color = RPL_DAG_MC_LC_RED; //Node color = 5
/****************************************************************/

	PROCESS_BEGIN();

	set_global_address();

   printf("RTT# local repair scheduled:%d. %d\n",DAG_RESET_START+1,DAG_RESET_STOP+1);
  		
   simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  // 60*CLOCK_SECOND should print for RM090 every one (1)  min
  // SEND_INTERVAL shoulb be defined in project-cnof.h
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

/*** SENDING MESSAGES: USUALLY RECEIVER DOES NOT SEND MSGS ***/
	 //sender(1); // send message: NUM IS THE NODE ID
/*************************************************************/
	
	// local repairs DO NOT SEEM TO WORK without a global repair...  	

   //local repair: Once at the 1st param, Once again at the 2nd
	reset_dag(DAG_RESET_START+1,DAG_RESET_STOP+1);      
    
    
    //printf("R:%d, Leaf MODE: %d\n",counter,rpl_get_mode());
	if(counter%11 == 0){
		//printf("R:%d, Node COLOR: %d\n",counter,node_color);
	}	
	

	
	
/********************** Nothing beyond this point *************/    
	counter++;
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

