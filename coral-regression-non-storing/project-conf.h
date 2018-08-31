#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

// Enable Link Color: Remember, it still estimates etx 
#ifndef RPL_CONF_DAG_MC
#define RPL_CONF_DAG_MC RPL_DAG_MC_LC
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
