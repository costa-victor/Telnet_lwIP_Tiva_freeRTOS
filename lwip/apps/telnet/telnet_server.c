/* ------------------------ FreeRTOS includes ----------------------------- */
#include "FreeRTOS.h"

/* ------------------------ lwIP includes --------------------------------- */
#include "inc/hw_memmap.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "lwip/tcp.h"
#include "lwip/ip.h"
#include "lwip/memp.h"
#include "lwip/stats.h"

#include "lwip/mem.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/sys.h"

/* ------------------------ Project includes ------------------------------ */
#include <string.h>
#include <stdio.h>

#ifndef SOCK_SERVER_PORT
#define SOCK_SERVER_PORT  23
#endif


volatile int clientfd=0;
void NewClient( void *pvParameters );
void SocketTelnetServer( void *pvParameters )
{
	//int on = 1;
	int sockfd, newsockfd, clilen;
	struct sockaddr_in serv_addr, cli_addr;

    /* Parameters are not used - suppress compiler error. */
    LWIP_UNUSED_ARG(pvParameters);

	/* First call to socket() function */
	sockfd = lwip_socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		//erro
		while(1){
			vTaskDelay(1000);
		}
	}

	/* Initialize socket structure */
	/* set up address to connect to */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_len = sizeof(serv_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = PP_HTONS(SOCK_SERVER_PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	/* Now bind the host address using bind() call.*/
	if (lwip_bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		//erro
		lwip_close(sockfd);
		while(1){
			vTaskDelay(1000);
		}
	}

	/* Now start listening for the clients, here
	 * process will go in sleep mode and will wait
	 * for the incoming connection
	 */
	if ( lwip_listen(sockfd, 5) != 0 ){
		lwip_close(sockfd);
		while(1){
			vTaskDelay(1000);
		}
	}

	clilen = sizeof(cli_addr);

	while(1){
		// Accept all requests
		newsockfd = lwip_accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)&clilen);

	    if (newsockfd>0) {
	        if (sys_thread_new( "NewClient", NewClient, ( void * ) &newsockfd, 2048, 5) == NULL){
	        	lwip_close(newsockfd);
	        }
	    }
	}
}

#define MAX_OUTPUT_LENGTH   128
#define MAX_INPUT_LENGTH    512

void NewClient( void *pvParameters ) {
	char buffer[256];
	int i, nbytes;
	int clientfd = *((int*) pvParameters);

    // Empty initial trash
    nbytes=lwip_read(clientfd, buffer, sizeof(buffer));

    // Welcome message
    lwip_send(clientfd, "Welcome to the FreeRTOS Telnet Server\n\r", 39, 0);

#if 0
	do {
		nbytes=lwip_recv(clientfd, buffer, sizeof(buffer),0);
		if (nbytes>0) { //no error

            for(i=0;i<nbytes;i++)
            {
                //lwip_send(clientfd, buffer[i], sizeof(buffer),0);
                //UARTPutChar(UART0_BASE, buffer[i]);
                // Aqui vai o c�digo do terminal
                // (void)UARTGetChar(&buffer[i], portMAX_DELAY);
            }



		}
	}  while (nbytes>0);



#else


    char cInputIndex = 0;
    BaseType_t xMoreDataToFollow;
    /* The input and output buffers are declared static to keep them off the stack. */
    static char pcOutputString[ MAX_OUTPUT_LENGTH ], pcInputString[ MAX_INPUT_LENGTH ];


    for( ;; ){

        //(void)UARTGetChar(&buffer, portMAX_DELAY);
        nbytes = lwip_recv(clientfd, buffer, sizeof(buffer), 0);

        if (nbytes>0) { //no error
            for(i=0;i<nbytes;i++){

                if( buffer[i] == '\r' )     // Enter pressed
                {

                    // Break line
                    //UARTPutString(UART0_BASE, "\r\n");
                    lwip_send(clientfd, "\r\n", sizeof("\r\n"),0);
                    do
                    {

                        xMoreDataToFollow = FreeRTOS_CLIProcessCommand
                                      (
                                          pcInputString,   /* The command string.*/
                                          pcOutputString,  /* The output buffer. */
                                          MAX_OUTPUT_LENGTH/* The size of the output buffer. */
                                      );


                        //UARTPutString(UART0_BASE, pcOutputString);
                        lwip_send(clientfd, pcOutputString, sizeof(pcOutputString),0);

                    } while( xMoreDataToFollow != pdFALSE );


                    cInputIndex = 0;
                    memset( pcInputString, 0x00, MAX_INPUT_LENGTH );
                }
                else
                {


                    if( buffer[i] == '\n' )
                    {
                        /* Ignore . */
                    }
                    else if( buffer[i] == '\b' )
                    {

                        if( cInputIndex > 0 )
                        {
                            cInputIndex--;
                            pcInputString[ cInputIndex ] = '\0';
                        }

                        //UARTPutString(UART0_BASE, &buffer[i]);
                        lwip_send(clientfd, &buffer[i], sizeof(char),0);
                    }
                    else
                    {

                        if( cInputIndex < MAX_INPUT_LENGTH )
                        {
                            pcInputString[ cInputIndex ] = buffer[i];
                            cInputIndex++;
                        }
                        //UARTPutString(UART0_BASE, &buffer[i]);
                        //lwip_send(clientfd, &buffer[i], sizeof(char),0);
                    }
                }

            }
        }
    }

#endif








	lwip_close(clientfd);
	// N�o precisa apagar a tarefa, pq o pr�prio port
	// do LwIP para o FreeRTOS faz isso
}
