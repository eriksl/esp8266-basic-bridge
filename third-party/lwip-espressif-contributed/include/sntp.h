#ifndef LWIP_SNTP_H
#define LWIP_SNTP_H

#include "lwip/opt.h"
#include "lwip/ip_addr.h"

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __tzrule_struct
{
  char ch;
  int m;
  int n;
  int d;
  int s;
  time_t change;
  long offset; /* Match type of _timezone. */
} __tzrule_type;

/** The maximum number of SNTP servers that can be set */
#ifndef SNTP_MAX_SERVERS
#define SNTP_MAX_SERVERS           3
#endif

/** Set this to 1 to implement the callback function called by dhcp when
 * NTP servers are received. */
#ifndef SNTP_GET_SERVERS_FROM_DHCP
#define SNTP_GET_SERVERS_FROM_DHCP 0//LWIP_DHCP_GET_NTP_SRV
#endif

/* Set this to 1 to support DNS names (or IP address strings) to set sntp servers */
#ifndef SNTP_SERVER_DNS
#define SNTP_SERVER_DNS            1
#endif

bool sntp_get_timetype(void);
void sntp_set_receive_time_size(void);
/** One server address/name can be defined as default if SNTP_SERVER_DNS == 1:
 * #define SNTP_SERVER_ADDRESS "pool.ntp.org"
 */

#if SNTP_SERVER_DNS
void sntp_setservername(u8_t idx, char *server);
char *sntp_getservername(u8_t idx);
#endif /* SNTP_SERVER_DNS */

#if SNTP_GET_SERVERS_FROM_DHCP
void sntp_servermode_dhcp(int set_servers_from_dhcp);
#else /* SNTP_GET_SERVERS_FROM_DHCP */
#define sntp_servermode_dhcp(x)
#endif /* SNTP_GET_SERVERS_FROM_DHCP */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_SNTP_H */
