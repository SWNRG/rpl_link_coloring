# rpl_link_coloring
Alteration of contiki/rpl to support link color:
Add the directory rpl into contiki/core/net/rpl and substitute the original. You now have:
Three (3) objective functions instead of two. 
The new one is MRHOF2. It is the same like the original MRHOF, but, the basic function best_parent() is altered:
The basic idea is that if any parent is red color, it is to be chosen, regardless the rank. If both or none parent is red, then etx is calculated as a metric.
To choose this new OF, you should declare it inside the project-conf.h
#ifndef RPL_CONF_OF_OCP
#define RPL_CONF_OF_OCP RPL_OCP_MRHOF2 // original is RPL_OCP_MRHOF
#endif

Other minor alterations exist in many files. They all have the comment //George at the beggining.
Remember that Link Color (LC) is mentioned in IANA, but not implemented by default.

Combine this new RPL with the code in the folder coral-rpl-udp-of which has three (3) different types of nodes inside:
a server sink, a mobile node, and a static node. If you alter the color of a family of nodes (node_color = RPL_DAG_MC_LC_RED) those nodes should be prefered as parents. There are more colors in rpl.h, alter them as needed.
#define RPL_DAG_MC_LC_WHITE  0
#define RPL_DAG_MC_LC_BROWN  1
#define RPL_DAG_MC_LC_GREEN  2
#define RPL_DAG_MC_LC_YELLOW 3
#define RPL_DAG_MC_LC_BLACK  4
#define RPL_DAG_MC_LC_RED    5

uint16_t node_color;  is defined also in rpl.h

Remember that enabling the Metric Container MC, and definining as Link color (LC) is more elegant to do it inside the specific project settings, i.e. in project-conf.h, as 
#ifndef RPL_CONF_WITH_MC
#define RPL_CONF_WITH_MC 1 // Enable MC
#endif 

#ifndef RPL_CONF_DAG_MC
#define RPL_CONF_DAG_MC RPL_DAG_MC_LC // Enable MC
#endif 


