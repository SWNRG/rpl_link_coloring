#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "dev/button-sensor.h"
#include "simple-udp.h"
#include "net/rpl/rpl.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 1234
#define SERVICE_ID 190

#define SEND_INTERVAL		(10 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection unicast_connection;
rpl_dag_t *dag; // moved here to be treated as global

static int counter=0;
  
/*---------------------------------------------------------------------------*/
PROCESS(button_server_process, "Button press server process");
PROCESS(unicast_receiver_process, "Unicast server receiver process");
AUTOSTART_PROCESSES(&unicast_receiver_process, &button_server_process);
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


static void reset_dag(unsigned int start, unsigned int end){
 
	if(counter == start){ //Change OF in real time
		printf("RTT# R:%d, OF_C Changing OF=3\n",counter);
		//dag->instance->of->ocp = RPL_OCP_MRHOF2; // 3
		
		dag->instance->of->ocp = RPL_OCP_MRHOF10; // 4
		
		
		
		while( rpl_repair_root(RPL_DEFAULT_INSTANCE) != 1 ){
			printf("RTT# RPL repair NOT succesful\n");
			rpl_repair_root(RPL_DEFAULT_INSTANCE);
		}
		printf("RTT# In p RPL repair succesful\n");
	}
	if(counter == end){ //Change OF back to oginal
		printf("RTT# R:%d, OF_C Changing OF=RPL_OCP_MRHOF\n",counter);
		dag->instance->of->ocp = RPL_OCP_MRHOF; // 1
		while( rpl_repair_root(RPL_DEFAULT_INSTANCE) != 1 ){
			printf("RTT# RPL repair NOT succesful\n");
			rpl_repair_root(RPL_DEFAULT_INSTANCE);
		}
		printf("RTT# In p RPL repair succesful\n");
	}
}
/*---------------------------------------------------------------------------*/


static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("DATA: '%s' received from ",data);
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
  
/**************** Sending back the received message **********************/
  printf("DATA: Sending msg back to ");
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
  
  
  
  
  
  
  //simple_udp_sendto(&unicast_connection, data, strlen(data) + 1, sender_addr);
/************************************************************************/ 
}
/*---------------------------------------------------------------------------*/


static void
create_rpl_dag(uip_ipaddr_t *ipaddr)
{
  struct uip_ds6_addr *root_if;

  root_if = uip_ds6_addr_lookup(ipaddr);
  if(root_if != NULL) {
    //rpl_dag_t *dag; // moved to the top for global usage
    uip_ipaddr_t prefix;
    
    rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
    dag = rpl_get_any_dag(); 
    uip_ip6addr(&prefix, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &prefix, 64);
    PRINTF("created a new RPL dag\n");
  } else {
    PRINTF("failed to create a new RPL DAG\n");
  }
}
/*---------------------------------------------------------------------------*/


PROCESS_THREAD(button_server_process, ev, data)
{

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  SENSORS_ACTIVATE(button_sensor);

  while(1){
    PROCESS_YIELD();
   
   
   	// maybe again here ???? dag = rpl_get_any_dag(); 
    	if (ev == sensors_event && data == &button_sensor) {
    		PRINTF("DATA Sender Sink button pressed...\n");
/******* YOU CAN CHANGE THE OF BY PRESSING THE SINK'S BUTTON *******/		   
			/*
			if(dag->instance->of->ocp == RPL_OCP_MRHOF2){
				PRINTF("DATA: Changing OF to RPL_OCP_MRHOF\n");
				dag->instance->of->ocp = RPL_OCP_MRHOF; // Original MRHOF
				printf("RPL: New ocp=%u\n",dag->instance->of->ocp);
			}
			else if(dag->instance->of->ocp != RPL_OCP_MRHOF2){
				PRINTF("DATA: Changing OF to RPL_OCP_MRHOF2\n");
				dag->instance->of->ocp = RPL_OCP_MRHOF2; // link color MRHOF
				printf("DATA: New ocp=%u\n",dag->instance->of->ocp);
			}
			*/
			PRINTF("DATA: Initiaing global repair\n");
			// Dont forget to reset rpl
			if( rpl_repair_root(RPL_DEFAULT_INSTANCE) == 1 ){
				printf("DATA: Sender Sink RPL repair succesful\n");
			}
			
/*******************************************************************/
    }
  }

  PROCESS_END();
}


PROCESS_THREAD(unicast_receiver_process, ev, data)
{
  uip_ipaddr_t *ipaddr;
  static struct etimer periodic_timer;

/* As root, the color has to be RED, correct?
 * So, if a child with two RED parents, will choose 
 * less etx, i.e. the sink.
 * If the sink is the only red, it will be chosen, but
 * yet if a child sees the sink, it has to choose it 
 * anyway, right? Even without color, result is the same?
 * There is no case with no RED color when sink is involved...
 */
	node_color = RPL_DAG_MC_LC_RED; //5    
  
  PROCESS_BEGIN();

  ipaddr = set_global_address();

  create_rpl_dag(ipaddr);

/******************* Creating a UDP UNICAST CONNECTION **********/
  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);
/***************************************************************/



  // 60*CLOCK_SECOND should print for RM090 every one (1)  min
  etimer_set(&periodic_timer, 60*CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);


/******* Objective Function (OF) **************************/	
/* NOTES ON OBJECTIVE FUNCTION:
 * Don't forger to repair the DAG after changing OF.
 * RPL does not check the OF after repair. 
 * We added the checking OF inside the code...	
 * Dont forget there is also LOCAL REPAIR. Can be used
 * as an argument that we dont destroy the whole graph,
 * just the neighborhood!
 */
/********************************************************/	

   //global repair: Once at the 1st param, Once again at the 2nd
	reset_dag(10,50);

	

/********************* PRINTOUTS ************************/	
	
	if(RPL_OF_OCP != dag->instance->of->ocp && counter%10 == 0){ //in case OF changes...
		printf("RTT# R:%d, In p Current Obj.Func= %u, while original RPL_OF_OCP=%d \n", 
			counter, dag->instance->of->ocp,RPL_OF_OCP);
	}else if(counter%10 == 0){ // print the OF every ten rounds....
		 //printf("RPL: variable value RPL_OF_OCP %d\n", RPL_OF_OCP);
		 printf("RTT# R:%d, Current Obj.Func: %u\n", 
			 counter, dag->instance->of->ocp);
	}
	
	if(counter%11 == 0){
		printf("R:%d, Node COLOR: %d\n",counter,node_color);
		//printf("RPL_MRHOF_SQUARED_ETX: %d\n", RPL_MRHOF_SQUARED_ETX); 
		// I should set this to zero ???
		if(RPL_CONF_WITH_NON_STORING !=1){
			printf("RTT# In p RPL IN STORING MODE\n"); 
		}
		else{
			printf("RTT# In p RPL IN NO STORING MODE\n"); 
		}
	}

	counter++;
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
