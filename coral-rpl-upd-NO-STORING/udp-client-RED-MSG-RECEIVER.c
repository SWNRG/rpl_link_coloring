#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include "net/rpl/rpl-conf.h"
#include "net/rpl/rpl.h"

#include "sys/ctimer.h"
#ifdef WITH_COMPOWER
#include "powertrace.h"
#endif
#include <stdio.h>
#include <string.h>

/* Only for TMOTE Sky? */
#include "dev/serial-line.h"
#include "dev/uart0.h"
#include "net/ipv6/uip-ds6-route.h"
#include "node-id.h"
#include "dev/button-sensor.h"

// George, redefined to go away from the original
#define UDP_CLIENT_PORT 6000 //8765
#define UDP_SERVER_PORT 7000 //5678
#define UDP_EXAMPLE_ID  190

//#define DEBUG DEBUG_FULL
//George No debug messages
#define DEBUG 0

#include "net/ip/uip-debug.h"

#ifndef PERIOD
#define PERIOD 60
#endif

#define IDOUBLE 8

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL	(PERIOD * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		30

static struct uip_udp_conn *client_conn;

static uip_ipaddr_t server_ipaddr; //this shoudl be the localhost???

//to emulate here the "server" reply
static struct uip_udp_conn *server_conn;

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

/* George: Moved here to be treated as global.
 * Remember, in sink it already existed as a variable
 */
rpl_dag_t *d; 

// RPL current instance to be used in local_repair()
rpl_instance_t *instance;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
PROCESS(print_metrics_process, "Printing metrics process");
AUTOSTART_PROCESSES(&udp_client_process,&print_metrics_process);
/*---------------------------------------------------------------------------*/

//from file rpl-conf.h
extern uint8_t rpl_dio_interval_min;
extern uint8_t rpl_dio_interval_doublings;


static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  printf("Server node local IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      print6addr(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/

static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 2);  // Changed last number from 1 to 2 

  // added behaviour for both ipaddr, and server_ipaddr
  uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 2);  // Changed last number from 1 to 2 
  
  printf("Server node ipaddr: ");
  print6addr(&ipaddr);
  printf("\n");
  
  printf("Server node server_ipaddr: ");
  print6addr(&ipaddr);
  printf("\n");
  
}
/*---------------------------------------------------------------------------*/


static void
tcpip_handler(void)
{
  char *appdata;

  if(uip_newdata()) {

  	 printf("Server: Custom Node DATA received....\n");
  	 
    appdata = (char *)uip_appdata;
    appdata[uip_datalen()] = 0;
    
// Although, this is NOT the server, it should nevertheless, reply  
#if SERVER_REPLY
    printf("Server Node DATA sending reply\n");
    
    // what does UIP_IP_BUF do ???
    
    uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
    uip_udp_packet_send(server_conn, "Reply", sizeof("Reply"));
    uip_create_unspecified(&server_conn->ripaddr);
#endif
  }
}    
    

// Threads start here........................

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic;
  static struct ctimer backoff_timer;
#if WITH_COMPOWER
  static int print = 0;
#endif


   // Is this needed ???
  //uip_ipaddr_t ipaddr;
  //uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 2);


  PROCESS_BEGIN();

  PROCESS_PAUSE();

  set_global_address();

  PRINTF("UDP client process started nbr:%d routes:%d\n",
         NBR_TABLE_CONF_MAX_NEIGHBORS, UIP_CONF_MAX_ROUTES);

  print_local_addresses();

  /* initialize serial line */
  uart0_set_input(serial_line_input_byte);
  serial_line_init();


#if WITH_COMPOWER
  powertrace_sniff(POWERTRACE_ON);
#endif

  /* The data sink runs with a 100% duty cycle in order to ensure high 
     packet reception rates. */
  NETSTACK_MAC.off(1); // George I dont know if this is REALLY needed

/********** Emulate here, the "server" reply ***************/
/********** Getting ready to receive messages **************/
  server_conn = udp_new(NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
  if(server_conn == NULL) {
    printf("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(server_conn, UIP_HTONS(UDP_SERVER_PORT));
  
/******************** Only printouts. Safely disable **********/
  printf("Created a Server connection with remote node ");
  print6addr(&server_conn->ripaddr);
  printf(" local/remote port %u/%u\n", UIP_HTONS(server_conn->lport),
         UIP_HTONS(server_conn->rport));
/*************************************************************/         


  etimer_set(&periodic, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
		
		
		// it doesn't come in here yet !!!!!!!!!
		
		printf("Server in custom Node: tcpip_event...\n");
		
    	tcpip_handler();
    }

    
//George ADDED BEHAVIOUR COPIED FROM SINK TO DO LOCAL REPAIRS
    if (ev == sensors_event && data == &button_sensor) {
/**** Trying to reset the instance for this node only *******/		   
			printf("RPL: Initializing LOCAL repair\n");
			rpl_local_repair(instance); // Dont forget to reset rpl
/***************************************************************/
    }

    if(ev == serial_line_event_message && data != NULL) {
      char *str;
      str = data;
      if(str[0] == 'r') {
        uip_ds6_route_t *r;
        uip_ipaddr_t *nexthop;
        uip_ds6_defrt_t *defrt;
        uip_ipaddr_t *ipaddr;
        defrt = NULL;
        if((ipaddr = uip_ds6_defrt_choose()) != NULL) {
          defrt = uip_ds6_defrt_lookup(ipaddr);
        }
        if(defrt != NULL) {
          PRINTF("DefRT: :: -> %02d", defrt->ipaddr.u8[15]);
          PRINTF(" lt:%lu inf:%d\n", stimer_remaining(&defrt->lifetime),
                 defrt->isinfinite);
        } else {
          PRINTF("DefRT: :: -> NULL\n");
        }

        for(r = uip_ds6_route_head();
            r != NULL;
            r = uip_ds6_route_next(r)) {
          nexthop = uip_ds6_route_nexthop(r);
          PRINTF("Route: %02d -> %02d", 
          		r->ipaddr.u8[15], nexthop->u8[15]);
          /* PRINT6ADDR(&r->ipaddr); */
          /* PRINTF(" -> "); */
          /* PRINT6ADDR(nexthop); */
          PRINTF(" lt:%lu\n", r->state.lifetime);

        }
      }
    }

    if(etimer_expired(&periodic)) {
      	etimer_reset(&periodic);
      
      // George we dont want this node to send messages. Just respond
      //ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);

#if WITH_COMPOWER
      if (print == 0) {
	      powertrace_print("#P");
      }
      if (++print == 3) {
	      print = 0;
      }
#endif

    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

// +++++++++++++ BELOW HERE, JUST PRINTING MESSAGES +++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
PROCESS_THREAD(print_metrics_process, ev, data){
  static struct etimer periodic_timer;

  static int counter=0;

  node_color = RPL_DAG_MC_LC_RED; //5

  instance = d->instance;

  d->instance->dio_intdoubl = IDOUBLE;
  
  PROCESS_BEGIN();
  PRINTF("Printing Client Metrics...\n");

  SENSORS_ACTIVATE(button_sensor);

  etimer_set(&periodic_timer, 60*CLOCK_SECOND);

  // Endless loop
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

// print whatever needed here.....
         
         
    counter++; //new round of stats
  }
  
   PROCESS_END();
}

/*---------------------------------------------------------------------------*/

