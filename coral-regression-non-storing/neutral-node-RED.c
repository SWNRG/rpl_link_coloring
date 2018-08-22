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

#define UDP_PORT 1234

#define SEND_INTERVAL		(60 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
static uint32_t sent_time=0; // to me
static unsigned int message_number;

static struct simple_udp_connection unicast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(sender_node_process, "Sender node process");
AUTOSTART_PROCESSES(&sender_node_process);
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

static void sender(){

	char buf[20];
	uip_ipaddr_t addr;
    //uip_ip6addr(&addr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x0201, 0x001, 0x001, 0x001);
	
	 // Works correctly sending to sink!!!
	 uip_ip6addr(&addr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0xc30c, 0, 0, 1);

	 // Works correctly sending to No 2!!! Remember, the last one is active...
	 //uip_ip6addr(&addr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0xc30c, 0, 0, 2);

      printf("DATA: Sending unicast msg to ");
      uip_debug_ipaddr_print(&addr);
      printf("\n");
      sprintf(buf, "Message %d", message_number);
      message_number++;
	  
/********************** SENDING UDP UNICAST TO &addr ********************/
	   sent_time = RTIMER_NOW();
      simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, &addr);
/************************************************************************/
}
    
    
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  uint32_t rttime;
  
  rttime = RTIMER_NOW()-sent_time;	
  printf("DATA: Sender received msg back: '%s'. RTT:%d",data,rttime);
  printf(", last Msg Num: %d\n", message_number-1);
  
 /****************** EASY TO EXTRACT DATA **************************/
  printf("RTT#	%d\n", rttime); // ready for extraction
  
  char gg = *data;
  char *tt="Message";

  //printf("DATA tt: '%s'\n",tt);
}
/*---------------------------------------------------------------------------*/


PROCESS_THREAD(sender_node_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;


  static int counter=0;

/******************** NODE COLOR LC = RED ************************/
  node_color = RPL_DAG_MC_LC_RED; //Node color = 5
  
  
  PROCESS_BEGIN();

  set_global_address();
	 
  //ready but not used in NEUTRAL NODE
  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  // 60*CLOCK_SECOND should print for RM090 every one (1)  min
  etimer_set(&periodic_timer, 60*CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));

/*********** THIS IS A NEUTRAL NODE: DOES NOT SEND MESSAGES *****/
	 sender(); // send message....
/****************************************************************/    
    
    counter++;
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
