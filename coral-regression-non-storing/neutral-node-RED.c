
#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "net/rpl/rpl.h" // for node_color
#include "simple-udp.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 1234

//#define SEND_INTERVAL		(60 * CLOCK_SECOND)

#define SEND_INTERVAL		(80 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))


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


PROCESS_THREAD(sender_node_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t addr;
  //variable to be in the same printing round for each node
  static int counter=0;
  
/******************** RED NODE ******************************/  
  node_color = RPL_DAG_MC_LC_RED; //5
    
    
  PROCESS_BEGIN();

  set_global_address();
  
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    etimer_set(&send_timer, SEND_TIME);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));


    //printf("R:%d, Leaf MODE: %d\n",counter,rpl_get_mode());
	if(counter%11 == 0){
		printf("R:%d, Node COLOR: %d\n",counter,node_color);
	}
   counter++; //new round of stats
    
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
