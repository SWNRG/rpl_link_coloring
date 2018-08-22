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
// RPL current instance to be used in local_repair()
rpl_instance_t *instance;

#define UDP_PORT 1234

#define SEND_INTERVAL		(60 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

// RPL current instance to be used in local_repair()
rpl_instance_t *instance;

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
  

uint8_t should_blink = 1;
static void
route_callback(int event, uip_ipaddr_t *route, uip_ipaddr_t *ipaddr, int num_routes)
{
  if(event == UIP_DS6_NOTIFICATION_DEFRT_ADD) {
    should_blink = 0;
  } else if(event == UIP_DS6_NOTIFICATION_DEFRT_RM) {
    should_blink = 1;
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
  printf("Sending DATA BACK to ");
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
  simple_udp_sendto(&unicast_connection, data, strlen(data) + 1, sender_addr);
/************************************************************************/

/***********************************************************************/  

}
/*---------------------------------------------------------------------------*/


PROCESS_THREAD(receiver_node_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct uip_ds6_notification n;

  static int counter=0;

/******************** NODE COLOR LC *****************************/
  node_color = RPL_DAG_MC_LC_RED; //Node color = 5
  
  PROCESS_BEGIN();

  set_global_address();

  uip_ds6_notification_add(&n, route_callback);

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  // 60*CLOCK_SECOND should print for RM090 every one (1)  min
  etimer_set(&periodic_timer, 60*CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
	
    if(should_blink) {
      leds_on(LEDS_ALL);
      printf("R: %u, NO ROUTES...\n",counter);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
      leds_off(LEDS_ALL);
    }

    if(ev){ // Any event)
       //printf("DATA event: '%u'\n",ev); // THIS WORKS!
    }

//George ADDED BEHAVIOUR COPIED FROM SINK TO DO LOCAL REPAIRS
    if (ev == sensors_event && data == &button_sensor) {
    
    // this is not working 
    
/**** Trying to reset the instance for this node only *******/		   
			printf("RPL: Initializing LOCAL repair\n");
			rpl_local_repair(instance); // Dont forget to reset rpl
/***************************************************************/
    }
    
    
    //printf("R:%d, Leaf MODE: %d\n",counter,rpl_get_mode());
	if(counter%11 == 0){
		printf("R:%d, Node COLOR: %d\n",counter,node_color);
	}	
	counter++;
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
