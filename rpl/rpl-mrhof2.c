/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         The Minimum Rank with Hysteresis Objective Function (MRHOF), RFC6719
 *
 *         This implementation uses the estimated number of
 *         transmissions (ETX) as the additive routing metric,
 *         and also provides stubs for the energy metric.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

/**
 * \addtogroup uip6
 * @{
 */

#include "net/rpl/rpl.h" // George Should be the same for all OFs

#include "net/rpl/rpl-private.h"
#include "net/nbr-table.h"
#include "net/link-stats.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

/* RFC6551 and RFC6719 do not mandate the use of a specific formula to
 * compute the ETX value. This MRHOF implementation relies on the value
 * computed by the link-stats module. It has an optional feature,
 * RPL_MRHOF_CONF_SQUARED_ETX, that consists in squaring this value.
 * This basically penalizes bad links while preserving the semantics of ETX
 * (1 = perfect link, more = worse link). As a result, MRHOF will favor
 * good links over short paths. Recommended when reliability is a priority.
 * Without this feature, a hop with 50% PRR (ETX=2) is equivalent to two
 * perfect hops with 100% PRR (ETX=1+1=2). With this feature, the former
 * path obtains ETX=2*2=4 and the former ETX=1*1+1*1=2. */
#ifdef RPL_MRHOF_CONF_SQUARED_ETX
#define RPL_MRHOF_SQUARED_ETX RPL_MRHOF_CONF_SQUARED_ETX
#else /* RPL_MRHOF_CONF_SQUARED_ETX */
#define RPL_MRHOF_SQUARED_ETX 0
#endif /* RPL_MRHOF_CONF_SQUARED_ETX */

#if !RPL_MRHOF_SQUARED_ETX
/* Configuration parameters of RFC6719. Reject parents that have a higher
 * link metric than the following. The default value is 512 but we use 1024. */
#define MAX_LINK_METRIC     1024 /* Eq ETX of 8 */
/* Hysteresis of MRHOF: the rank must differ more than PARENT_SWITCH_THRESHOLD_DIV
 * in order to switch preferred parent. Default in RFC6719: 192, eq ETX of 1.5.
 * We use a more aggressive setting: 96, eq ETX of 0.75.
 */
#define PARENT_SWITCH_THRESHOLD 96 /* Eq ETX of 0.75 */
#else /* !RPL_MRHOF_SQUARED_ETX */
#define MAX_LINK_METRIC     2048 /* Eq ETX of 4 */
#define PARENT_SWITCH_THRESHOLD 160 /* Eq ETX of 1.25 (results in a churn comparable to the threshold of 96 in the non-squared case) */
#endif /* !RPL_MRHOF_SQUARED_ETX */

/* Reject parents that have a higher path cost than the following. */
#define MAX_PATH_COST      32768   /* Eq path ETX of 256 */

/*---------------------------------------------------------------------------*/

static void
reset(rpl_dag_t *dag)
{
  printf("RPL: Reset MRHOF2 was initiated\n");
}
/*---------------------------------------------------------------------------*/


#if RPL_WITH_DAO_ACK
static void
dao_ack_callback(rpl_parent_t *p, int status)
{
  if(status == RPL_DAO_ACK_UNABLE_TO_ADD_ROUTE_AT_ROOT) {
    return;
  }
  /* here we need to handle failed DAO's and other stuff */
  PRINTF("RPL: MRHOF - DAO ACK received with status: %d\n", status);
  if(status >= RPL_DAO_ACK_UNABLE_TO_ACCEPT) {
    /* punish the ETX as if this was 10 packets lost */
    link_stats_packet_sent(rpl_get_parent_lladdr(p), MAC_TX_OK, 10);
  } else if(status == RPL_DAO_ACK_TIMEOUT) { /* timeout = no ack */
    /* punish the total lack of ACK with a similar punishment */
    link_stats_packet_sent(rpl_get_parent_lladdr(p), MAC_TX_OK, 10);
  }
}
#endif /* RPL_WITH_DAO_ACK */
/*---------------------------------------------------------------------------*/


static uint16_t
parent_link_metric(rpl_parent_t *p)
{
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
  if(stats != NULL) {
#if RPL_MRHOF_SQUARED_ETX
    uint32_t squared_etx = ((uint32_t)stats->etx * stats->etx) / LINK_STATS_ETX_DIVISOR;
    return (uint16_t)MIN(squared_etx, 0xffff);
#else /* RPL_MRHOF_SQUARED_ETX */
  return stats->etx;
#endif /* RPL_MRHOF_SQUARED_ETX */
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/



static uint16_t
parent_path_cost(rpl_parent_t *p)
{
  uint16_t base;

  if(p == NULL || p->dag == NULL || p->dag->instance == NULL) {
    return 0xffff;
  }

#if RPL_WITH_MC
  /* Handle the different MC types */
  switch(p->dag->instance->mc.type) {
    case RPL_DAG_MC_ETX:
      base = p->mc.obj.etx;
      break;

	/* George : Altough coloring, the base is still etx. 
	 * if color==X do something else..   
	 * if you do base = p->mc.obj.lc; then the base will 
	 * be calculated on the color number. This way you 
	 * can poison certain roots
	 */   
    case RPL_DAG_MC_LC: 
	
		//printf("parent's lc: %d\n",p->mc.obj.lc);
		//printf("parent's etx: %d\n",p->mc.obj.etx);

	    base = p->mc.obj.etx;
		//Can make the red nodes more attractive with this
		//base = p->mc.obj.lc;
		break;
		
    case RPL_DAG_MC_ENERGY:
      base = p->mc.obj.energy.energy_est << 8;
      break;
    default:
      base = p->rank;
      break;
  }
#else /* RPL_WITH_MC */
  base = p->rank;
#endif /* RPL_WITH_MC */

  /* path cost upper bound: 0xffff */
  return MIN((uint32_t)base + parent_link_metric(p), 0xffff);
}
/*---------------------------------------------------------------------------*/




static rpl_rank_t
rank_via_parent(rpl_parent_t *p)
{
  uint16_t min_hoprankinc;
  uint16_t path_cost;

  if(p == NULL || p->dag == NULL || p->dag->instance == NULL) {
    return INFINITE_RANK;
  }

  min_hoprankinc = p->dag->instance->min_hoprankinc;
  path_cost = parent_path_cost(p);

  /* Rank lower-bound: parent rank + min_hoprankinc */
  return MAX(MIN((uint32_t)p->rank + min_hoprankinc, 0xffff), path_cost);
}
/*---------------------------------------------------------------------------*/




static int
parent_is_acceptable(rpl_parent_t *p)
{
  uint16_t link_metric = parent_link_metric(p);
  uint16_t path_cost = parent_path_cost(p);
  /* Exclude links with too high link metrics or path cost (RFC6719, 3.2.2) */
  return link_metric <= MAX_LINK_METRIC && path_cost <= MAX_PATH_COST;
}
/*---------------------------------------------------------------------------*/





static int
parent_has_usable_link(rpl_parent_t *p)
{
  uint16_t link_metric = parent_link_metric(p);
  /* Exclude links with too high link metrics  */
  return link_metric <= MAX_LINK_METRIC;
}
/*---------------------------------------------------------------------------*/




static rpl_parent_t *
best_parent(rpl_parent_t *p1, rpl_parent_t *p2)
{
  rpl_dag_t *dag;
  uint16_t p1_cost;
  uint16_t p2_cost;
  int p1_is_acceptable;
  int p2_is_acceptable;

  uint8_t my_color; //George use node's own color
  
  // George prints all neighbors !!!
  //rpl_print_neighbor_list();

  p1_is_acceptable = p1 != NULL && parent_is_acceptable(p1);
  p2_is_acceptable = p2 != NULL && parent_is_acceptable(p2);

  if(!p1_is_acceptable) {
    return p2_is_acceptable ? p2 : NULL;
  }
  if(!p2_is_acceptable) {
    return p1_is_acceptable ? p1 : NULL;
  }

  dag = p1->dag; /* Both parents are in the same DAG. */
  p1_cost = parent_path_cost(p1);
  p2_cost = parent_path_cost(p2);

	// Only when the parents are both or none RED
	if( (p1->mc.l_color.lc == RPL_DAG_MC_LC_RED && p2->mc.l_color.lc == RPL_DAG_MC_LC_RED) ||
		 (p1->mc.l_color.lc != RPL_DAG_MC_LC_RED && p2->mc.l_color.lc != RPL_DAG_MC_LC_RED)
	  )
	{
	  /* Maintain stability of the preferred parent in case of similar ranks. */
	  if(p1 == dag->preferred_parent || p2 == dag->preferred_parent) {
		 if(p1_cost < p2_cost + PARENT_SWITCH_THRESHOLD &&
		    p1_cost > p2_cost - PARENT_SWITCH_THRESHOLD) {
		   return dag->preferred_parent;
		 }
	  }
	}


// node's OWN color
  //printf("MRHOF2: My color: %u\n",dag->instance->mc.l_color.lc);
  my_color = dag->instance->mc.l_color.lc ;
  
  //rpl_rank_t e2e = RPL_OF.calculate_rank(p1, 0);
             //rpl_rank_t hbh = e2e - p1->rank;
             //printf("    [ip ");
             //uip_debug_ipaddr_print(&p->addr);
             //printf("] ");
             //printf(" rank: %5u + %5u = %5u", p1->rank, hbh, e2e);
            // p == dag->preferred_parent ? printf(" PREF\n") : printf("\n");



	if(p1->rank == ROOT_RANK(dag->instance)){
		//printf("MRHOF2: P1 chosen as Sink: %u\n",rpl_get_parent_ipaddr(p1)->u8[15]);
		return p1;
	}
	if(p2->rank == ROOT_RANK(dag->instance)){
		//printf("MRHOF2: P2 chosen as Sink: %u\n",rpl_get_parent_ipaddr(p2)->u8[15]);
		return p2;
	}



	// If I AM RED, and any parent is the receiver, return it
	if(p1->mc.l_color.lc == RPL_DAG_MC_LC_ORANGE){
		if(my_color == RPL_DAG_MC_LC_RED){
			//printf("MRHOF2: I am RED, found ORANGE father: %u\n",rpl_get_parent_ipaddr(p1)->u8[15]);
			return p1;
		}else{
			//printf("MRHOF2: I am not RED, found ORANGE (Don't care..), I choose etx\n");
			return p1_cost < p2_cost ? p1 : p2; //original return			
		}
	}
	if(p2->mc.l_color.lc == RPL_DAG_MC_LC_ORANGE){
		if(my_color == RPL_DAG_MC_LC_RED){
			//printf("MRHOF2: I am RED, found ORANGE father: %u\n",rpl_get_parent_ipaddr(p1)->u8[15]);
			return p2;
		}else{
			//printf("MRHOF2: I am not RED, found ORANGE (Don't care..), I choose etx\n");
			return p1_cost < p2_cost ? p1 : p2; //original return			
		}
	}


   switch(my_color) {
  
		case RPL_DAG_MC_LC_PURPLE: //sender
			if(p1->mc.l_color.lc == RPL_DAG_MC_LC_RED){ //p1 IS red
				 //printf("MRHOF2: I am sender. P1: %u was RED\n",rpl_get_parent_ipaddr(p1)->u8[15]);
				 return p1;	
			}else if(p2->mc.l_color.lc == RPL_DAG_MC_LC_RED){ //p2 IS red
				 //printf("MRHOF2: I am sender. P2: %u was RED\n",rpl_get_parent_ipaddr(p2)->u8[15]);
				 return p2;	
			}else{
				//printf("MHROF2: Sender didn't find RED father. Chose etx\n");
				return p1_cost < p2_cost ? p1 : p2; //original return	
			}		
			break;
			
		case RPL_DAG_MC_LC_ORANGE: //receiver
			/* You are last, don't bother with colors, normal choice */
			//printf("MHROF2: I am ORANGE (last one). Chose etx\n");
			return p1_cost < p2_cost ? p1 : p2; //original return	
			break;
			
		case RPL_DAG_MC_LC_RED: // I am RED, so I have at least ONE RED parent
			if(p1->mc.l_color.lc == RPL_DAG_MC_LC_RED){
		  		if(p2->mc.l_color.lc == RPL_DAG_MC_LC_RED){
		  			//printf("MRHOF2: both parents RED\n");
		  	   	if(p1_cost < p2_cost){ // p1_cost smaller
		  	   		 //printf("MRHOF2: P1 RED chosen: %u\n",rpl_get_parent_ipaddr(p1)->u8[15]);
		  	   		 return p1;
		  	   	}else{                 // p2_cost smaller
		  	   		 //printf("MRHOF2: P2 RED chosen: %u\n",rpl_get_parent_ipaddr(p2)->u8[15]);
		  	   		 return p2;
		  	   	}
		  		}else{ //p2 is NOT red, but p1 is red
		  			 //printf("MRHOF2: Parent p1: %u was RED\n",rpl_get_parent_ipaddr(p1)->u8[15]);
		  			 return p1;	
		  		}
		  	}else{ //p1 is NOT red
		  	   if(p2->mc.l_color.lc == RPL_DAG_MC_LC_RED){ //p2 IS red
		  	   	 //printf("MRHOF2: Parent p2: %u was RED\n",rpl_get_parent_ipaddr(p2)->u8[15]);
		  		    return p2;
		  		}
		  		else{ //no parent is RED
		  			//printf("MHROF2: A RED node didn't find RED father. Chose etx\n");
		  			return p1_cost < p2_cost ? p1 : p2; //original return	
		  		}
			}
			break;
			
		case RPL_DAG_MC_LC_WHITE: // I am a normal node, choose etx	
			//printf("MHROF2: I am WHITE (Don't care about anything). Chose etx\n");		 
	 		return p1_cost < p2_cost ? p1 : p2; //original return	
	 		break;
	 	default:
	 		printf("MRHOF2: Unsupported color found. Switch to etx...\n");
	 		return p1_cost < p2_cost ? p1 : p2; //original return	return NULL;
 
  }// end switch my_color
	
}
/*---------------------------------------------------------------------------*/




static rpl_dag_t *
best_dag(rpl_dag_t *d1, rpl_dag_t *d2)
{
  if(d1->grounded != d2->grounded) {
    return d1->grounded ? d1 : d2;
  }

  if(d1->preference != d2->preference) {
    return d1->preference > d2->preference ? d1 : d2;
  }

  return d1->rank < d2->rank ? d1 : d2;
}
/*---------------------------------------------------------------------------*/



#if !RPL_WITH_MC // This is IF NOT MC
static void
update_metric_container(rpl_instance_t *instance)
{
  instance->mc.type = RPL_DAG_MC_NONE;
  
  // George: it should never come here when MC exists...
  printf("RPL: RPL_DAG_MC_NONE\n");
  
}

#else /* RPL_WITH_MC */
static void
update_metric_container(rpl_instance_t *instance)
{
  rpl_dag_t *dag;
  uint16_t path_cost;
  uint8_t type;
  
  dag = instance->current_dag;
  if(dag == NULL || !dag->joined) {
    PRINTF("RPL: Cannot update the metric container when not joined\n");
    return;
  }

  if(dag->rank == ROOT_RANK(instance)) {
    
/* George: RPL_DAG_MC is FIRST SET HERE. 
 * It is ONLY SET TO ROOT!
 * Configure MC at root only, other nodes are 
 * auto-configured when joining 
 */
    instance->mc.type = RPL_DAG_MC;

    //George: Testing only
    //printf("RPL: instance->mc.type =%d\n",RPL_DAG_MC); 

    instance->mc.flags = 0;
    instance->mc.aggr = RPL_DAG_MC_AGGR_ADDITIVE;
    instance->mc.prec = 0;
    path_cost = dag->rank;
    
    //George
    instance->mc.l_color.lc = node_color; //sink needs to be red?

  } else {
    path_cost = parent_path_cost(dag->preferred_parent);
    
    //George
    instance->mc.l_color.lc = node_color;
  }

  /* Handle the different MC types */
  switch(instance->mc.type) {
    case RPL_DAG_MC_NONE:
    	
    	// George: It should NEVER come here with containers enabled
    	printf("RPL: MRHOF2 case RPL_DAG_MC_NONE\n");
    	
      break;
    case RPL_DAG_MC_ETX:
      instance->mc.length = sizeof(instance->mc.obj.etx);
      instance->mc.obj.etx = path_cost;     
      printf("It should NOT come here with LC enabled...\n");
      break;
      

/* George: Remember, in here it only sets the length
 * of the container and sets the object lc to
 * node_color for each node. 
 * Calculating the path, DOES NOT HAPPEN HERE,
 * it happens in best_parent()
 */ 
    case RPL_DAG_MC_LC: 
      
      instance->mc.obj.etx = path_cost;       

      break;
    case RPL_DAG_MC_ENERGY:
      instance->mc.length = sizeof(instance->mc.obj.energy);
      if(dag->rank == ROOT_RANK(instance)) {
        type = RPL_DAG_MC_ENERGY_TYPE_MAINS;
      } else {
        type = RPL_DAG_MC_ENERGY_TYPE_BATTERY;
      }
      instance->mc.obj.energy.flags = type << RPL_DAG_MC_ENERGY_TYPE;
      /* Energy_est is only one byte, use the least significant byte of the path metric. */
      instance->mc.obj.energy.energy_est = path_cost >> 8;
      break;
    default:
      // George: It should never come here if MC is valid
      printf("MRHOF2, non-supported MC %u\n", instance->mc.type);
      PRINTF("RPL: MRHOF2, non-supported MC %u\n", instance->mc.type);
      break;
  }
}
#endif /* RPL_WITH_MC */
/*---------------------------------------------------------------------------*/




rpl_of_t rpl_mrhof2 = {
  reset,
#if RPL_WITH_DAO_ACK
  dao_ack_callback,
#endif
  parent_link_metric,
  parent_has_usable_link,
  parent_path_cost,
  rank_via_parent,
  best_parent,
  best_dag,
  update_metric_container,
  RPL_OCP_MRHOF2 //George: name of the OF
};

/** @}*/  
