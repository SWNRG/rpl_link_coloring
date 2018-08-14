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

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678
#define UDP_EXAMPLE_ID  190

//#define DEBUG DEBUG_FULL
//George No debug messages
#define DEBUG 0

#include "net/ip/uip-debug.h"

#ifndef PERIOD
#define PERIOD 60
#endif

#define IDOUBLE 8

//George Turn this to 1 for mobile nodes ONLY
//#define RPL_CONF_LEAF_ONLY 1 //Does not seem to work

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL	(PERIOD * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		30

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

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


static int seq_id;
static int reply;

static void
tcpip_handler(void)
{
  char *str;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    reply++;
    printf("DATA recv '%s' (s:%d, r:%d)\n", str, seq_id, reply);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  char buf[MAX_PAYLOAD_LEN];

#ifdef SERVER_REPLY
  uint8_t num_used = 0;
  uip_ds6_nbr_t *nbr;

  nbr = nbr_table_head(ds6_neighbors);
  while(nbr != NULL) {
    nbr = nbr_table_next(ds6_neighbors, nbr);
    num_used++;
  }

  if(seq_id > 0) {
    ANNOTATE("#A r=%d/%d,color=%s,n=%d %d\n", reply, seq_id,
             reply == seq_id ? "GREEN" : "RED", uip_ds6_route_num_routes(), num_used);
  }
#endif /* SERVER_REPLY */

  seq_id++;
  PRINTF("DATA send to %d 'Hello %d'\n",
         server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1], seq_id);
  sprintf(buf, "Hello %d from the client", seq_id);
  uip_udp_packet_sendto(client_conn, buf, strlen(buf),
                        &server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
}
/*---------------------------------------------------------------------------*/

static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
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

/* The choice of server address determines its 6LoWPAN header compression.
 * (Our address will be compressed Mode 3 since it is derived from our
 * link-local address)
 * Obviously the choice made here must also be selected in udp-server.c.
 *
 * For correct Wireshark decoding using a sniffer, add the /64 prefix to the
 * 6LowPAN protocol preferences,
 * e.g. set Context 0 to fd00::. At present Wireshark copies Context/128 and
 * then overwrites it.
 * (Setting Context 0 to fd00::1111:2222:3333:4444 will report a 16 bit
 * compressed address of fd00::1111:22ff:fe33:xxxx)
 *
 * Note the IPCMV6 checksum verification depends on the correct uncompressed
 * addresses.
 */
 
#if 0
/* Mode 1 - 64 bits inline */
   uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 1);
#elif 1
/* Mode 2 - 16 bits inline */
  uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
#else
/* Mode 3 - derived from server link-local (MAC) address */
  uip_ip6addr(&server_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
#endif
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic;
  static struct ctimer backoff_timer;
#if WITH_COMPOWER
  static int print = 0;
#endif

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  set_global_address();

  PRINTF("UDP client process started nbr:%d routes:%d\n",
         NBR_TABLE_CONF_MAX_NEIGHBORS, UIP_CONF_MAX_ROUTES);

  print_local_addresses();

  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL); 
  if(client_conn == NULL) {
    PRINTF("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT)); 

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  /* initialize serial line */
  uart0_set_input(serial_line_input_byte);
  serial_line_init();


#if WITH_COMPOWER
  powertrace_sniff(POWERTRACE_ON);
#endif

  etimer_set(&periodic, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
    
//George ADDED BEHAVIOUR COPIED FROM SINK TO DO LOCAL REPAIRS
    if (ev == sensors_event && data == &button_sensor) {
/*********** Trying to resent the instance for this node only *******/		   
			printf("RPL: Initializing LOCAL repair\n");
			rpl_local_repair(instance); // Dont forget to reset rpl
/**********************************************************************/
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
      ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);

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
//Tryfon's extra for statistics gathering at serial port
PROCESS_THREAD(print_metrics_process, ev, data){
  static struct etimer periodic_timer;
 
  //variable to be in the same printing round for each node
  static int counter=0;

/* NODE COLOR:
 * Remember: you have to use MRHOF2 in order to consider
 * node coloring. 
 * The idea is that, if any parent is RED, it is chosen,
 * Otherwise, normal etx value is considered
 */
  node_color = RPL_DAG_MC_LC_RED;

  // George current RPL instance
  instance = d->instance;

/********* Default mode: Imin remains unchanged **********/
  //d->instance->dio_intmin = 12;
  //d->instance->dio_intcurrent = 8;
  
  //George Idouble will be set from outside. Otherwise it is 8
  d->instance->dio_intdoubl = IDOUBLE;
  
  PROCESS_BEGIN();
  PRINTF("Printing Client Metrics...\n");

  SENSORS_ACTIVATE(button_sensor);

/******************** LEAF MODE ***************************/
	/* mode =2 means LEAF NODE i.e., node does not accept 
	 * forwarding packets. feather =1 
	*/
	//rpl_set_mode(2);
/******************** LEAF MODE ***************************/	
		

  // 60*CLOCKS_SECOND for rm090 should print every one (1) min
  etimer_set(&periodic_timer, 60*CLOCK_SECOND);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
	
	 printf("R:%d, Node COLOR: %d\n",counter,node_color);
	
    printf("R:%d, DAG-VERSION:%d\n",counter, d->version); 
   // printf("R:%d, DAG-DIO_COUNTER:%d\n",counter, d->instance->dio_counter); 
   // printf("R:%d, DAG-Imin:%d\n",counter, d->instance->dio_intmin); 
   // printf("R:%d, DAG-IDOUBLINGS:%d\n",counter, d->instance->dio_intdoubl); 
   // printf("R:%d, DAG-IminCURRENT:%d\n",counter, d->instance->dio_intcurrent); 
   // printf("R:%d, DAG-DefaultLIFETIME:%d\n",counter, d->instance->default_lifetime); 
   // printf("R:%d, DAG-LIFETIME:%d\n",counter, d->instance->lifetime_unit); 
	
    printf("R:%d, Imin:%d, Idoubling:%d\n",
   	counter, d->instance->dio_intmin, d->instance->dio_intdoubl);

    printf("R:%d, udp_sent:%d\n",counter,uip_stat.udp.sent);
    printf("R:%d, udp_recv:%d\n",counter,uip_stat.udp.recv);	
    
    printf("R:%d, icmp_sent:%d\n",counter,uip_stat.icmp.sent);
    printf("R:%d, icmp_recv:%d\n",counter,uip_stat.icmp.recv);

	 //rpl_mode rpl_set_mode(2) leaf mode =2, feather =1 
    printf("R:%d, Leaf MODE: %d\n",counter,rpl_get_mode());
    
    counter++; //new round of stats
  }
   PROCESS_END();
}

/*---------------------------------------------------------------------------*/

