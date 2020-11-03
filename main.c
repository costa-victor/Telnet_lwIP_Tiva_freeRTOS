#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "FreeRTOS.h"
#include "task.h"
                                // Adições necessárias para desenvolver o driver de UART
#include "queue.h"
#include "semphr.h"

                                // Informações de registradores
#include "inc/hw_ints.h"        // Hardware interrupts
#include "inc/hw_uart.h"        // Hardware UART

                                // Informações do driver
#include "driverlib/uart.h"     // UART.h
#include "driverlib/pin_map.h"  // PIN_MAP.h para configurar os pinos

#include "FreeRTOS_CLI.h"       // CLI

#include "lwiplib.h"            // lwIP
#include "pinout.h"             // Pinout para config ethernet
#include "telnet_server.h"


/*
 * Otimização ao usar UART_QUEUE - Evita ficar voltando na função toda vez - Comentado abaixo:
 *
 * Ao inves de jogar os dados no registrador e controlar pelo registrador. Eu adiciono os
 * dados em uma fila, e quem controla a transmissão é a própria fila.
 *
 * CUSTO:
 *  Uma fila a mais adicionada.
 *
 * BENEFICIO:
 *  - A tarefa só volta a executar quando terminar a frase inteira. Na implementação anterior, a
 *  tarefa era chamada uma vez para cada caractere, até que terminasse a transmissão.
 *  - Tenho menos troca de contexto, já que to aproveitando a interrupção de TX para transmitir tudo.
 */

// ===========  Defines ===========

#define UART_CARACTERE      1
#define UART_QUEUE          2
#define UART_STRING         UART_QUEUE

// #define MAX_OUTPUT_LENGTH   128
// #define MAX_INPUT_LENGTH    512


// ===========  Global Variables  ===========

#if UART_STRING == UART_QUEUE
    QueueHandle_t qUART0Tx;             // Declares a queue structure for the UART
    volatile uint32_t isstring = 0;     // Sempre que usar a QUEUE para transmitir, este recebe valor 1 em UARTPutString
#endif
QueueHandle_t qUART0;                   // Declares a queue structure for the UART
SemaphoreHandle_t sUART0;               // Declares a semahpore strcuture for the UART
SemaphoreHandle_t mutexTx0;             // Declares a mutex structure for the UART
uint32_t g_ui32SysClock;                // System clock rate in Hz.
BaseType_t exitFlag = 0;

struct netif sNetIF;

volatile BaseType_t lwip_link_up = pdFALSE;     // Required by lwIP library to support any host-related timer functions.
uint32_t g_ui32IPAddress;                       // Required by lwIP library to support any host-related timer functions.


// Handler's
TaskHandle_t task1_handle;
TaskHandle_t task2_handle;
TaskHandle_t terminal_handle;
TaskHandle_t print_task_handle;
TaskHandle_t lwIP_handler;



// ===========  Headers  ===========

portBASE_TYPE UARTGetChar(char *data, TickType_t timeout);
void UARTPutChar(uint32_t ui32Base, char ucData);
void UARTPutString(uint32_t ui32Base, char *string);
static BaseType_t prvEchoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t prvLED_OnOff_Command(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t prvClearCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t prvExitCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t prvTextColorCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
void Terminal(void *param);
void print_task(void *arg);
void task1(void* param);
void task2(void* param);
extern void SocketTCPClient( void *pvParameters );
void UpLwIP(void *param);
void lwIPHostTimerHandler(void);
void DisplayIPAddress(uint32_t ui32Addr);



// ===========  Functions  ===========

void configurePIN0(void){
    // Enable the GPIO port that is used for the on-board LED.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    // Check if the peripheral access is enabled.
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
        // Caso o clock não esteja ativo, vou desistir do processador
        vTaskDelay(100);
    }

    // Enable the GPIO pin for the LED (PN0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

}

void setPIN0(BaseType_t param){
    if(param == 0){
        // Turn off the LED.
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
    }
    else{
        // Turn on the LED.
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
    }
}

void task1(void* param){

    // Enable the GPIO port that is used for the on-board LED.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    // Check if the peripheral access is enabled.
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
        // Caso o clock não esteja ativo, vou desistir do processador
        vTaskDelay(100);
    }

    // Enable the GPIO pin for the LED (PN0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    while(1){
        // Turn on the LED.
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
        vTaskDelay(200);   // 200ticks = 200ms ---> Ou seja, ticktime = 1ms

        // Turn off the LED.
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
        vTaskDelay(200);   // 200ticks = 200ms
    }
}

void task2(void* param){

    // Enable the GPIO port that is used for the on-board LED.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    // Check if the peripheral access is enabled.
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
        // Caso o clock não esteja ativo, vou desistir do processador
        vTaskDelay(100);
    }

    // Enable the GPIO pin for the LED (PN1).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);

    while(1){
        // Turn on the LED.
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
        vTaskDelay(1000);   // 1000ticks = 1s

        // Turn off the LED.
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
        vTaskDelay(1000);   // 1000ticks = 1s
    }
}


// This function will compete for UART resource
void print_task(void *arg){
    vTaskDelay(200);    // Tempo para esperar a inicializar da UART
    while(1){
        UARTPutString(UART0_BASE, "Teste!\n\r");
        vTaskDelay(1000);
    }
}


void UARTIntHandler(void){
    uint32_t ui32Status;        // Necessária para o driver da UART da Texas
    // Variaveis que testam se vai haver preempção ou não, se eu acordei uma tarefa mais prioritária
    signed portBASE_TYPE pxHigherPriorityTaskWokenRX = pdFALSE;
    signed portBASE_TYPE pxHigherPriorityTaskWokenTX = pdFALSE;
    char data;

    #if UART_STRING == UART_QUEUE
        BaseType_t ret;
    #endif

        // Get the interrupt stauts
        ui32Status = MAP_UARTIntStatus(UART0_BASE, true);       // É RX ou TX???

        UARTIntClear(UART0_BASE, ui32Status);                   // Depois de saber o que, eu limpo as flags

        // Se for a interrupção de RX
        if((ui32Status&UART_INT_RX) == UART_INT_RX){
            // Loop while there are characteres in the receive FIFO
            while(MAP_UARTCharsAvail(UART0_BASE)){
                // Read the next character from the UART and write it back to the UART
                data = (char)MAP_UARTCharGetNonBlocking(UART0_BASE);
                xQueueSendToBackFromISR(qUART0, &data, &pxHigherPriorityTaskWokenRX);
            }
        }

        if((ui32Status&UART_INT_TX) == UART_INT_TX){
            #if UART_STRING == UART_QUEUE
                if(isstring){
                    // read the char to be sent
                    // Se for string, recebo o caractere da fila Tx
                    ret = xQueueReceiveFromISR(qUART0Tx, &data, NULL);

                    if(ret){
                        // Send the char
                        HWREG(UART0_BASE + UART_O_DR) = data;
                    }
                    else{
                        // There is no more char to be send in this string
                        isstring = 0;
                        MAP_UARTIntDisable(UART0_BASE, UART_INT_TX);

                        // Wake up the task that is sending the string
                        xSemaphoreGiveFromISR(sUART0, &pxHigherPriorityTaskWokenTX);
                    }
                }
                else{
                    MAP_UARTIntDisable(UART0_BASE, UART_INT_TX);

                    // Call the task that is sending the char
                    xSemaphoreGiveFromISR(sUART0, &pxHigherPriorityTaskWokenTX);
                }
            #else
            MAP_UARTIntDisable(UART0_BASE, UART_INT_TX);

            // Call the keyboard analysis task
            xSemaphoreGiveFromISR(sUART0, &pxHigherPriorityTaskWokenTX);
            #endif
        }
        // Se houver uma task com prioridade maior em TX ou RX, significa que precisa ter uma troca de contexto
        // Portanto, eu chamo o YIELD.
        if((pxHigherPriorityTaskWokenRX == pdTRUE) || (pxHigherPriorityTaskWokenTX == pdTRUE)){
            portYIELD();
        }
}


// Lê uma fila do sistema
// Não tem mutex para receber os dados, por opção de projeto. Já que não tem nada concorrendo com a leitura
portBASE_TYPE UARTGetChar(char *data, TickType_t timeout){
    return xQueueReceive(qUART0, data, timeout);
}

void UARTPutChar(uint32_t ui32Base, char ucData){
    /*
     * 2 Diferenças entre semaphore e mutex
     *      1º - Semaphore não tem o controle de herança de prioridade, e o mutex tem
     *      2º - Semaphore binario do mutex sai aberto de inicio, e o que não for mutex sai fechado
     */

    if(mutexTx0 != NULL){
        if(xSemaphoreTake(mutexTx0, portMAX_DELAY) == pdTRUE){
            // Send the char
            // Copia o dado ucData que eu quero transmitir, para o registrador de dados da UART0
            HWREG(ui32Base + UART_O_DR) = ucData;

            // Wait until space is available.
            // Esta interrupt vai me avisar quando o buffer tiver vazio (Por isso chamei ela depois de copiar,
            //  pois senão ela já iria disparar)
            MAP_UARTIntEnable(UART0_BASE, UART_INT_TX);

            // Wait indefinitely for a UART interrupt
            // Poderia tambem usar um timeout de 2ms, e se demorasse mais que este timeout aconteceu um erro
            xSemaphoreTake(sUART0, portMAX_DELAY);  // Sai daqui significa que a transmissão terminou

            xSemaphoreGive(mutexTx0);               // Portanto, eu posso liberar o recurso
        }
    }
}

#if UART_STRING == UART_CARACTERE
void UARTPutString(uint32_t ui32Base, char *string){
    if(mutexTx0 != NULL){
        if(xSemaphoreTake(mutexTx0, portMAX_DELAY) == pdTRUE){
            while(*string){
                // Send the char
                // Copia o dado ucData que eu quero transmitir, para o registrador de dados da UART0
                HWREG(ui32Base + UART_O_DR) = *string;

                // Wait until space is available.
                // Esta interrupt vai me avisar quando o buffer tiver vazio (Por isso chamei ela depois de copiar,
                //  pois senão ela já iria disparar)
                MAP_UARTIntEnable(UART0_BASE, UART_INT_TX);

                // Wait indefinitely for a UART interrupt
                // Poderia tambem usar um timeout de 2ms, e se demorasse mais que este timeout aconteceu um erro
                xSemaphoreTake(sUART0, portMAX_DELAY);  // Sai daqui significa que a transmissão terminou

                string++;       // Avança para o proximo caractere da string

            }

            // Terminado a string, libero o recurso
            xSemaphoreGive(mutexTx0);
        }
    }
}
#else

void UARTPutString(uint32_t ui32Base, char *string){
    char data;
    if(mutexTx0 != NULL){
        if(xSemaphoreTake(mutexTx0, portMAX_DELAY) == pdTRUE){
            isstring = 1;
            // Vare a string e joga tudo para uma fila
            while(*string){
                // Joga para o final da fila
                xQueueSendToBack(qUART0Tx, string, portMAX_DELAY); // DELAY => Se a fila tiver cheia, eu fico esperando ter espaço na fila

                string++;
            }

            // Send the char
            xQueueReceive(qUART0Tx, &data, portMAX_DELAY);
            HWREG(ui32Base + UART_O_DR) = data;

            MAP_UARTIntEnable(UART0_BASE, UART_INT_TX);

            // Wait indefinitely the finish of the string transmission by the UART port
            xSemaphoreTake(sUART0, portMAX_DELAY);

            xSemaphoreGive(mutexTx0);
        }
    }
}

#endif

// =============== FreeRTOS+CLI ======================
/*
 * Para utilizar, é preciso cumprir 4 passos:
 *  1º - Criar uma função que implemente o compartamento do comando
 *  2º - Mapear o comando para a função que irá implementar seu comportamento
 *  3º - Registrar este comando com o FreeRTOS+CLI
 *  4º - Por fim, executar o interpretador de comandos
 */

// 1º Passo - Criar uma função que implemente o compartamento do comando
/* */
static BaseType_t prvLED_OnOff_Command(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString){
    char *pcParameter1;
    BaseType_t xParameterStringLength;

    // Get the first parameter
    pcParameter1 = FreeRTOS_CLIGetParameter
            (
                    // typed command
                    pcCommandString,
                    // Return the first parameter
                    1,
                    // Store the string size
                    &xParameterStringLength
            );


    if(!strcmp(pcParameter1, "ON") || !strcmp(pcParameter1, "on")){
        strcpy(pcWriteBuffer, "LED 0 turned ON\r\n");
        setPIN0(1);
    }
    else if(!strcmp(pcParameter1, "OFF") || !strcmp(pcParameter1, "off")){
        strcpy(pcWriteBuffer, "LED 0 turned OFF\r\n");
        setPIN0(0);
    }
    else{
        strcpy(pcWriteBuffer, "Invalid parameter - Type ON or OFF\r\n");
    }

    return pdFALSE;     // Processo terminou e não há mais nada pra fazer

}

static BaseType_t prvEchoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString){
    char *pcParameter1;
    BaseType_t xParameter1StringLength;

    pcParameter1 = FreeRTOS_CLIGetParameter
                        (
                          pcCommandString,
                          // Return the first parameter.
                          1,
                          // Store the parameter string length. */
                          &xParameter1StringLength
                        );
    strcpy(pcWriteBuffer, strcat(pcParameter1, "\r\n"));

    return pdFALSE;
}

static BaseType_t prvClearCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString){
    // Clears the screen and moves cursor to home position
    strcpy(pcWriteBuffer, "\033[2J\033[H");

    return pdFALSE;
}

static BaseType_t prvExitCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString){
    // Trying to write '^]' to close the connection - Not working yet
    // https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
    //strcpy(pcWriteBuffer, "\x1b[\x1d");

    strcpy(pcWriteBuffer, "^]");
    return pdFALSE;
}

static BaseType_t prvTextColorCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString){
    // References:
    // https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
    char *pcParameter1;
    BaseType_t xParameterStringLength;

    pcParameter1 = FreeRTOS_CLIGetParameter
            (
                    pcCommandString,
                    1,
                    &xParameterStringLength
            );


    if(!strcmp(pcParameter1, "red")){
        strcpy(pcWriteBuffer, "\x1b[38;5;196m\n\r\tColor switched to red\n\r");
    }
    else if(!strcmp(pcParameter1, "green")){
        strcpy(pcWriteBuffer, "\x1b[38;5;46m\n\r\tColor switched to green\n\r");
    }
    else if(!strcmp(pcParameter1, "blue")){
        strcpy(pcWriteBuffer, "\x1b[38;5;39m\n\r\tColor switched to blue\n\r");
    }
    else if(!strcmp(pcParameter1, "yellow")){
        strcpy(pcWriteBuffer, "\x1b[38;5;220m\n\r\tColor switched to yellow\n\r");
    }
    else if(!strcmp(pcParameter1, "black")){
        strcpy(pcWriteBuffer, "\x1b[38;5;232m\n\r\tColor switched to black\n\r");
    }
    else if(!strcmp(pcParameter1, "white")){
        strcpy(pcWriteBuffer, "\x1b[38;5;231m\n\r\tColor switched to white\n\r");
    }
    else{
        strcpy(pcWriteBuffer, "Invalid color - Type 'help'\r\n");
    }

    return pdFALSE;
}

// 2º Passo - Mapear o comando para a função que irá implementar seu comportamento
/* */
static const CLI_Command_Definition_t xLEDCommand =
{
     "led", // Command name
     "\r\nled: Turn the led ON or OFF (Parameter: ON or OFF)\r\n\r\n", // Hint --help
     prvLED_OnOff_Command,    // Called function
     1                      // This command has 1 parameter
};


static const CLI_Command_Definition_t xEchoCommand =
{
     "echo", // Command name
     "\r\necho:\r\n echo [string]\r\n\r\n", // Hint --help
     prvEchoCommand,    // Called function
     1                      // This command has 1 parameter
};

static const CLI_Command_Definition_t xClearCommand =
{
     "clear", // Command name
     "\r\nclear:\r\n Clean the console\r\n\r\n", // Hint --help
     prvClearCommand,    // Called function
     0
};

static const CLI_Command_Definition_t xExitCommand =
{
     "exit", // Command name
     "\r\nexit:\r\n Type <exit> to close connection\r\n\r\n", // Hint --help
     prvExitCommand,    // Called function
     0
};

static const CLI_Command_Definition_t xTextColorCommand =
{
     "color", // Command name
     "\r\ncolor:\r\nChoose your text color [red, green, blue, yellow, black, white]\r\n\tExample: color red\r\n\r\n", // Hint --help
     prvTextColorCommand,    // Called function
     1
};



// 4º - Executar o interpretador de comandos, mais info aqui:
// https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_Command_Line_Interface.html
void Terminal(void *param){
    char data;
    (void)param;
    int cInputIndex = 0;
    BaseType_t xMoreDataToFollow;
    static char pcOutputString[ MAX_OUTPUT_LENGTH ], pcInputString[ MAX_INPUT_LENGTH ];     // The input and output buffers are declared static to keep them off the stack

    // Criação de um semaphore binário
    // Vai alocar dinamicamente na memória do sistema que é o HEAP, a estrutura de dados semaphoro e vai devolver
    // um ponteiro de semaphore
    sUART0 = xSemaphoreCreateBinary();

    // Como é um ponteiro, posso testar se ele é nulo
    if (sUART0 == NULL){
        // There was insufficient FreeRTOS heap available for the semaphore to be created successfully
        vTaskSuspend(NULL);
    }
    else{
        mutexTx0 =  xSemaphoreCreateMutex();
        if (mutexTx0 == NULL){
            // There was insufficient FreeRTOS heap available for the semaphore to be created successfully
            vSemaphoreDelete(sUART0);   // Se não tive memória suficinete para criar este semaphore, exclua o anterior
            vTaskSuspend(NULL);
        }
        else{
            qUART0 = xQueueCreate(128, sizeof(char));    // Fila de 128 caracteres do tipo char
            if(qUART0 == NULL){
                // There was insufficient FreeRTOS heap available for the semaphore to be created successfully
                vSemaphoreDelete(sUART0);
                vSemaphoreDelete(mutexTx0);
                vTaskSuspend(NULL);
            }
            else{       // Deu tudo certo, agora vou inicializar o periférico
                #if UART_STRING == UART_QUEUE
                    qUART0Tx =  xQueueCreate(128, sizeof(char));
                #endif

                    // Enable the peripherals used by this example.
                    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);    // CLock da UART inicializado
                    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);    // CLock dos pinos conectados a UART

                    // Set GPIO A0 and A1 as UART pins
                    // Para usar o driver da UART, precisa definir este symbol TARGET_IS_TM4C129_RA1
                    // Operações para o processador ARM
                    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
                    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
                    // Operações para o periférico da Texas
                    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

                    // Configure the UART for 115.200, 8-N-1 operation
                    MAP_UARTConfigSetExpClk(UART0_BASE, configCPU_CLOCK_HZ, 115200,
                                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE)
                                            );

                    // Desabilitando a FIFO - Pq a FIFO tem uma forma diferente de usar e atrapalha
                    // um pouco o entendimento de como configura uma UART
                    MAP_UARTFIFODisable(UART0_BASE);

                    // Enable the UART interrupt
                    // 1º - Defino a prioridade de nível 5 das interrupções
                    // 2º - Para o processador ARM, quero habilitar a interrupção
                    // 3º - Para o periférico, quero habilitar a interrupção de recepção
                    MAP_IntPrioritySet(INT_UART0, 0xC0);
                    MAP_IntEnable(INT_UART0);
                    MAP_UARTIntEnable(UART0_BASE, UART_INT_RX);
            }
        }
    }

    // Limpa a tela
    UARTPutString(UART0_BASE, "\033[2J\033[H");

    /* This code assumes the peripheral being used as the console has already
    been opened and configured, and is passed into the task as the task
    parameter.  Cast the task parameter to the correct type. */

    /* Send a welcome message to the user knows they are connected. */
    UARTPutString(UART0_BASE , "\r\n\t\tFreeRTOS Terminal:\r\n\r\n~$ ");

    for( ;; )
    {
        /* This implementation reads a single character at a time.  Wait in the
        Blocked state until a character is received. */

        // Desiste do processador até que tenha uma interrupção, por isso o portMAX_DELAY
        (void)UARTGetChar(&data, portMAX_DELAY);
        if( data == '\r' )     // Enter pressed
        {
            /* A newline character was received, so the input command string is
            complete and can be processed.  Transmit a line separator, just to
            make the output easier to read. */

            /* The command interpreter is called repeatedly until it returns
            pdFALSE.  See the "Implementing a command" documentation for an
            exaplanation of why this is. */

            // Welcome home
            UARTPutString(UART0_BASE, "\r\n");
            do
            {
                /* Send the command string to the command interpreter.  Any
                output generated by the command interpreter will be placed in the
                pcOutputString buffer. */
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand
                              (
                                  pcInputString,   /* The command string.*/
                                  pcOutputString,  /* The output buffer. */
                                  MAX_OUTPUT_LENGTH/* The size of the output buffer. */
                              );

                /* Write the output generated by the command interpreter to the
                console. */
                UARTPutString(UART0_BASE, pcOutputString);

                if( xMoreDataToFollow == pdFALSE  )
                {
                    UARTPutString(UART0_BASE, "~$ ");
                }

            } while( xMoreDataToFollow != pdFALSE );

            /* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
            cInputIndex = 0;
            memset( pcInputString, 0x00, MAX_INPUT_LENGTH );
        }
        else
        {
            /* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */

            if( data == '\n' )
            {
                /* Ignore . */
            }
            else if( data == '\b' )
            {
                /* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
                if( cInputIndex > 0 )
                {
                    cInputIndex--;
                    pcInputString[ cInputIndex ] = '\0';
                }

                UARTPutChar(UART0_BASE, data);
            }
            else
            {
                /* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a n is entered the complete
                string will be passed to the command interpreter. */
                if( cInputIndex < MAX_INPUT_LENGTH )
                {
                    pcInputString[ cInputIndex ] = data;
                    cInputIndex++;
                }

                UARTPutChar(UART0_BASE, data);
            }
        }
    }
}


//*****************************************************************************
//
// Display an lwIP type IP Address.
//
//*****************************************************************************
void
DisplayIPAddress(uint32_t ui32Addr)
{
    // Para poder ver o conteúdo em modo Debug
    //static volatile char pcBuf[16];
    char pcBuf[16];

    //
    // Convert the IP Address into a string.
    //
    sprintf(pcBuf, "%d.%d.%d.%d", (int)(ui32Addr & 0xff), (int)((ui32Addr >> 8) & 0xff),
            (int)((ui32Addr >> 16) & 0xff), (int)((ui32Addr >> 24) & 0xff));

    //
    // Display the string.
    UARTPutString(UART0_BASE, pcBuf);
}


void
lwIPHostTimerHandler(void)
{
    uint32_t ui32Idx, ui32NewIPAddress;

    //
    // Get the current IP address.
    //
    ui32NewIPAddress = lwIPLocalIPAddrGet();

    //
    // See if the IP address has changed.
    //
    if(ui32NewIPAddress != g_ui32IPAddress)
    {
        //
        // See if there is an IP address assigned.
        //
        if(ui32NewIPAddress == 0xffffffff)
        {
            //
            // Indicate that there is no link.
            //
            //UARTPutString(UART0_BASE, "Waiting for link.\n\r");
        }
        else if(ui32NewIPAddress == 0)
        {
            //
            // There is no IP address, so indicate that the DHCP process is
            // running.
            //
            //UARTPutString(UART0_BASE, "Waiting for IP address.\n\r");
        }
        else
        {
            //
            // Display the new IP address.
            //
            lwip_link_up = pdTRUE;
            UARTPutString(UART0_BASE, "\r\nIP Address: ");
            DisplayIPAddress(ui32NewIPAddress);
        }

        //
        // Save the new IP address.
        //
        g_ui32IPAddress = ui32NewIPAddress;

        //
        // Turn GPIO off.
        //
       // MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, ~GPIO_PIN_1);
    }

    //
    // If there is not an IP address.
    //
    if((ui32NewIPAddress == 0) || (ui32NewIPAddress == 0xffffffff))
    {
        //
        // Loop through the LED animation.
        //

        for(ui32Idx = 1; ui32Idx < 17; ui32Idx++)
        {

            //
            // Toggle the GPIO
            //
#if 0
            MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1,
                    (MAP_GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_1) ^
                     GPIO_PIN_1));

           // DelayTask(1000/ui32Idx);
#endif
        }
    }
}



void UpLwIP(void *param)
{
    uint32_t ui32User0, ui32User1;
    //uint32_t ui32Loop;
    uint8_t pui8MACArray[6];
    (void)param;
    //
    // Configure the device pins.
    //
    PinoutSet(true, false);

    vTaskDelay(1500);
    //UARTPutString(UART0_BASE, "Ethernet lwIP example\n\r");

    // Configure the hardware MAC address for Ethernet Controller filtering of
    // incoming packets.  The MAC address will be stored in the non-volatile
    // USER0 and USER1 registers.
    ui32User0 = 0x001AB6;
    ui32User1 = 0x0318CC;

    MAP_FlashUserSet(ui32User0, ui32User1);
    MAP_FlashUserGet(&ui32User0, &ui32User1);
    if((ui32User0 == 0xffffffff) || (ui32User1 == 0xffffffff)){
        // We should never get here.  This is an error if the MAC address has
        // not been programmed into the device.  Exit the program.
        // Let the user know there is no MAC address
        //UARTPutString(UART0_BASE, "No MAC programmed!\n\r");
        while(1)
        {
        }
    }

    // Tell the user what we are doing just now.
    //UARTPutString(UART0_BASE, "Waiting for IP.\n\r");

    // Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
    // address needed to program the hardware registers, then program the MAC
    // address into the Ethernet Controller registers.
    pui8MACArray[0] = ((uint8_t)(ui32User0 >>  0) & 0xff);
    pui8MACArray[1] = ((uint8_t)(ui32User0 >>  8) & 0xff);
    pui8MACArray[2] = ((uint8_t)(ui32User0 >> 16) & 0xff);
    pui8MACArray[3] = ((uint8_t)(ui32User1 >>  0) & 0xff);
    pui8MACArray[4] = ((uint8_t)(ui32User1 >>  8) & 0xff);
    pui8MACArray[5] = ((uint8_t)(ui32User1 >> 16) & 0xff);

    // Initialize the lwIP library, using DHCP.
    lwIPInit(configCPU_CLOCK_HZ, pui8MACArray, 0, 0, 0, IPADDR_USE_DHCP, &sNetIF);

    // Set the interrupt priorities.  We set the SysTick interrupt to a higher
    // priority than the Ethernet interrupt to ensure that the file system
    // tick is processed if SysTick occurs while the Ethernet handler is being
    // processed.  This is very likely since all the TCP/IP and HTTP work is
    // done in the context of the Ethernet interrupt.
    while(lwip_link_up != pdTRUE){
        vTaskDelay(1000);
    }

    /* Para caso o lwip/apps/httpserver_raw esteja ativo
    // Initialize a sample httpd server.
    //httpd_init();


    // Inicia cliente SNTP
    //sntp_init();
    */

    // Initialize a socket Telnet server
    //SocketTelnetServer("useless parameter");
    sys_thread_new("LwIP Telnet Server", SocketTelnetServer, NULL, 2048, 5);


    // Loop forever.  All the work is done in the created tasks
    while(1)
    {
        // Delay ou pode inclusive apagar a tarefa
        vTaskDelay(10000);
    }

}

// ===========  Main  ===========

int main(void)
{
    MAP_SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);
    //
    // Run from the PLL at 120 MHz.
    // Note: SYSCTL_CFG_VCO_240 is a new setting provided in TivaWare 2.2.x and
    // later to better reflect the actual VCO speed due to SYSCTL#22.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);

    //eNABLE STACKING FOR INTERRUPT HANDLER. tHIS ALLOWS FLOATING-POINTS
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage
    MAP_FPUEnable();
    MAP_FPULazyStackingEnable();

    // Configura a GPIO para o PIN0
    configurePIN0();

    // Registrando os comandos
    FreeRTOS_CLIRegisterCommand(&xLEDCommand);
    FreeRTOS_CLIRegisterCommand(&xEchoCommand);
    FreeRTOS_CLIRegisterCommand(&xClearCommand);
    FreeRTOS_CLIRegisterCommand(&xExitCommand);
    FreeRTOS_CLIRegisterCommand(&xTextColorCommand);

    // Instalando tarefa lwIP
    xTaskCreate(UpLwIP, "LwIP Task", 1024, NULL, 10, &lwIP_handler);

    // Instalando uma tarefa - Pressione Ctrl+Space enquanto digita uma função pra ativar o Intellisense
    //xTaskCreate(task1, "Tarefa 1", 256, NULL, 10, &task1_handle);
    //xTaskCreate(task2, "Tarefa 2", 256, NULL, 10, &task2_handle);
    xTaskCreate(Terminal, "Terminal Serial", 256, NULL, 3, &terminal_handle);
    //xTaskCreate(print_task, "Print task", 256, NULL, 7, &print_task_handle);


    // Start the scheduller
    vTaskStartScheduler();
    return 0;
}
