/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "sys/timer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uiplib.h"


#include "simple-udp.h"
#include "udp_server.h"

#include <stdio.h>
#include <string.h>
#include "dev/leds.h"

uint8_t httpd_sprint_ip6(uip_ipaddr_t addr, char * result);

#define UDP_PORT 3000

#define SEND_INTERVAL		(10 * CLOCK_SECOND)

static struct simple_udp_connection broadcast_connection;
int16_t send_counter = 0;

/*---------------------------------------------------------------------------*/
int16_t neighborCount(neighborList neighbors)
{
	if (neighbors)
	{
		return 1 + neighborCount(neighbors->next);
	}
	else
	{
		return 0;
	}
}

int16_t neighborMember(neighborList neighbors, uip_ipaddr_t *sender_addr)
{
	neighborList tmp;

	for(tmp = neighbors; tmp != NULL; tmp = tmp->next)
	{
		if (uip_ipaddr_cmp(&tmp->ipaddr, sender_addr))
		{
			return 1;
		}
	}

	return 0;
}

neighborList updateLastNeighborActivity(neighborList neighbors, uip_ipaddr_t *sender_addr)
{
	if (neighbors == NULL)
	{
		return NULL;
	}
	else if (uip_ipaddr_cmp(&neighbors->ipaddr, sender_addr))
	{
		neighbors->lastActivity = clock_time();
		return neighbors;
	}
	else
	{
		neighbors->next = updateLastNeighborActivity(neighbors->next, sender_addr);
		return neighbors;
	}
}

neighborList insertNeighbor(neighborList neighbors, uip_ipaddr_t *sender_addr)
{
	if (neighborMember(neighbors, sender_addr) == 1)
	{
		return updateLastNeighborActivity(neighbors, sender_addr);
	}
	else
	{
		PRINTF("[INSERT] Neighbor:");
		PRINT6ADDR(sender_addr);
		PRINTF("\n");

		neighborList tmp = malloc(sizeof(neighborElement_t));
		tmp->lastActivity = clock_time();
		uip_ipaddr_copy(&tmp->ipaddr, sender_addr);
		tmp->next = neighbors;

		if (neighbors == NULL)
		{
			tmp->id = 1;
		}
		else
		{
			tmp->id = neighbors->id + 1;
		}

		return tmp;
	}
}

neighborList removeInactivNeighbors(neighborList neighbors)
{
	if (neighbors == NULL)
	{
		return NULL;
	}
	else if (neighbors->lastActivity <= clock_time() - 3000)
	{
		PRINTF("[TIMEOUT] Neighbor: ");
		PRINT6ADDR(&neighbors->ipaddr);
		PRINTF("\n");

		neighborList tmp = neighbors;
		neighbors = neighbors->next;
		free(tmp);
		return removeInactivNeighbors(neighbors);
	}
	else
	{
		neighbors->next = removeInactivNeighbors(neighbors->next);
		return neighbors;
	}
}

void showAllNeighbors(neighborList neighbors)
{
	if (neighbors == NULL)
	{
	}
	else
	{
		showAllNeighbors(neighbors->next);

		PRINTF("%d: ", neighbors->id);
		PRINT6ADDR(&neighbors->ipaddr);
		PRINTF("\n");
	}
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
PROCESS(udp_server_process, "UDP broadcast Server process");
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
		uip_ipaddr_t *sender_addr,
        uint16_t sender_port,
        uip_ipaddr_t *receiver_addr,
        uint16_t receiver_port,
        uint8_t *data,
        uint16_t datalen)
{

	uint8_t *broadcast_message = "I am here";
	uint8_t *response_message = "Hey there";

	PRINTF("Receiving \"%.*s\" from ", datalen, data);
	PRINT6ADDR(sender_addr);
	PRINTF("\n");

	if (strstr(data, broadcast_message) != NULL)
	{
		neighbor_list = insertNeighbor(neighbor_list, sender_addr);

		messageToAll(response_message);
	}
	else if (strstr(data, response_message) != NULL)
	{

	}
	else
	{
		uip_ds6_addr_t *lladdr;
		uip_ipaddr_t tmp;

		uiplib_ipaddrconv(data, &tmp);

        lladdr = uip_ds6_get_link_local(-1);

		if (uip_ipaddr_cmp(&tmp, &lladdr->ipaddr)) {
			leds_toggle(LEDS_GREEN);
		}
	}

}
/*---------------------------------------------------------------------------*/
void
messageToAll(const uint8_t *message)
{
	uip_ipaddr_t addr;

	uip_create_linklocal_allnodes_mcast(&addr);

	PRINTF("Sending \"%s\" \n", message);

    simple_udp_sendto(&broadcast_connection, message, strlen(message), &addr);

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  static struct etimer send_timer;
  static struct etimer timeout_timer;

  uint8_t *broadcast_message = "I am here";
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

  neighbor_list = NULL;

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  etimer_set(&send_timer, SEND_INTERVAL);
  while(1) {
	send_counter = send_counter + 1;

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&send_timer));
    etimer_reset(&send_timer);

    if ( send_counter % 3 == 0)
    {
    	neighbor_list = removeInactivNeighbors(neighbor_list);
    }

    PRINTF("Sending broadcast message: \"%s\"\n", broadcast_message);
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&broadcast_connection, broadcast_message, strlen(broadcast_message), &addr);

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
