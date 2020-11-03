/*
 * telnet_server.h
 *
 *  Created on: 19/06/2015
 *      Author: Gustavo
 */

#ifndef LWIP_1_4_1_APPS_TELNET_TELNET_SERVER_H_
#define LWIP_1_4_1_APPS_TELNET_TELNET_SERVER_H_


extern void SocketTelnetServer( void *pvParameters );
extern void TelnetServer( void *pvParameters );
extern void telnet_write(const void *dataptr, int size);

#define MAX_OUTPUT_LENGTH   128
#define MAX_INPUT_LENGTH    512


#endif /* LWIP_1_4_1_APPS_TELNET_TELNET_SERVER_H_ */
