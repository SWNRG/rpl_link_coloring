rpl-conf.h
/*
 * The objective function (OF) used by a RPL root is configurable through
 * the RPL_CONF_OF_OCP parameter. This is defined as the objective code
 * point (OCP) of the OF, RPL_OCP_OF0 or RPL_OCP_MRHOF. This flag is of
 * no relevance to non-root nodes, which run the OF advertised in the
 * instance they join.
 * Make sure the selected of is in RPL_SUPPORTED_OFS.
 */
#ifdef RPL_CONF_OF_OCP
#define RPL_OF_OCP RPL_CONF_OF_OCP
#else /* RPL_CONF_OF_OCP */
#define RPL_OF_OCP RPL_OCP_MRHOF
//#define RPL_OF_OCP RPL_OCP_OF0
#endif /* RPL_CONF_OF_OCP */


// * Maximum of concurent RPL instances.
#ifdef RPL_CONF_MAX_INSTANCES
#define RPL_MAX_INSTANCES     RPL_CONF_MAX_INSTANCES
#else
#define RPL_MAX_INSTANCES     1
#endif /* RPL_CONF_MAX_INSTANCES */


rpl.h
/* IANA Objective Code Point as defined in RFC6550 */
#define RPL_OCP_OF0     0
#define RPL_OCP_MRHOF   1

rpl-private.h
#if RPL_OF_OCP == RPL_OCP_MRHOF... (adiaforo)

rpl-dag.c
PRINTF("RPL: OF with OCP %u not supported\n", RPL_OF_OCP);
    
power parameters inside rpl-mrhof.c
