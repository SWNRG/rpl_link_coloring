#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_



// Enable Link Color: Remember, it still estimates etx 
#ifndef RPL_CONF_DAG_MC
#define RPL_CONF_DAG_MC RPL_DAG_MC_LC
#endif

/* Centrally control msg sending time
 * For RM90 60 * CLOCK_SECONDS is one (1) min
 * Play with it here to send more messages per min
 * BUT, remember the counter is affected
 */
#define SEND_INTERVAL_CONF  (60 * CLOCK_SECOND) 



/* Centrally control the repairs(Global & local) for all nodes */
#define DAG_RESET_START_CONF 10 //First global repair
#define DAG_RESET_STOP_CONF  40 //Second global repair


#ifdef DAG_RESET_START_CONF
// remember: has to do with SEND_INTERVAL_CONF
#define DAG_RESET_START DAG_RESET_START_CONF
#endif 

#ifdef DAG_RESET_STOP_CONF
// remember: has to do with SEND_INTERVAL_CONF
#define DAG_RESET_STOP DAG_RESET_STOP_CONF
#endif 



//George "poison" the cost if parent is red !!!!!!!!!!!!!!
#ifndef RED_NODE_BONUS 
#define RED_NODE_BONUS PARENT_SWITCH_THRESHOLD; //play with it
#endif
 
// Server Relpy to enable Receiver to reply
#ifndef SERVER_REPLY
#define SERVER_REPLY 1 // Enable responces
#endif 

/* George: NON-STORING MODE */
#ifndef RPL_CONF_WITH_NON_STORING
#define RPL_CONF_WITH_NON_STORING 0
#endif

//George: Change Objective Function HERE
#ifndef RPL_CONF_OF_OCP
//#define RPL_CONF_OF_OCP RPL_OCP_MRHOF2 // original is RPL_OCP_MRHOF




//testing 

#define RPL_CONF_OF_OCP RPL_OCP_MRHOF




#endif 


/* George No need to change it. Just RPL_CONF_WITH_NON_STORING...*/
#ifndef WITH_NON_STORING
#define WITH_NON_STORING RPL_CONF_WITH_NON_STORING /* Set this to run with non-storing mode */
#endif /* WITH_NON_STORING */

//Stolen from regression tests non-storing
#undef RPL_CONF_PROBING_INTERVAL
#define RPL_CONF_PROBING_INTERVAL (60 * CLOCK_SECOND)
//#define TCPIP_CONF_ANNOTATE_TRANSMISSIONS 1


//George: Enable METRIC CONTAINER (mc) FOR THIS PROJECT ONLY
#ifndef RPL_CONF_WITH_MC
#define RPL_CONF_WITH_MC 1 // Enable MC
#endif 



#if WITH_NON_STORING
#undef RPL_CONF_MOP
#define RPL_CONF_MOP RPL_MOP_NON_STORING /* Mode of operation*/
#endif /* WITH_NON_STORING */


#endif 
