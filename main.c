/*
 * main.c
 *
 * Autor: Rodrigo Chang
 * Fecha: 2 de agosto de 2014
 *
 * Programa para probar la configuracion de una interfaz SPI three-wire en el modulo 0.
 * El programa envía un numero entre 0 y 4095 para poner un voltaje de salida en un DAC SPI.
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"

#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"


#define DAC_SPD_FAST	0x4000
#define DAC_SPD_SLOW	0x0000
#define DAC_PWR_DOWN	0x2000
#define DAC_PWR_NORMAL	0x0000

//#define __CONFIGURACION_CON_REGISTROS__


// Prototipos
void SSI0_Init(void);
void SSI0_Out(unsigned short);
void DAC_Out(unsigned short);

/*
 * Funcion principal
 */
int main(void) {
	// Configuracion del reloj a 40MHz
	SysCtlClockSet(SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ | SYSCTL_USE_PLL | SYSCTL_SYSDIV_5);

	// Configurar la interfaz SSI
	SSI0_Init();

	while (1) {
		// Enviar un dato
		DAC_Out(100);
		SysCtlDelay(26666666);
		DAC_Out(500);
		SysCtlDelay(26666666);
		DAC_Out(1000);
		SysCtlDelay(26666666);
		DAC_Out(2000);
		SysCtlDelay(26666666);
		SSIDataPutNonBlocking(SSI0_BASE, 2500);
		SysCtlDelay(26666666);
	}
}


/*
 * Configura la interfaz SSI0
 *
 * Notas: El formato Freescale SPI con SPO=1 y SPH=1 funciona bien con el TLV5616,
 * 	pero en cambio se configura con el formato TI, que es especifico para este integrado
 *
 */
void SSI0_Init(void) {
	#ifdef __CONFIGURACION_CON_REGISTROS__
		volatile unsigned long delay;
		// Habilitar el modulo SSI y el puerto A
		SYSCTL_RCGCSSI_R |= SYSCTL_RCGCSSI_R0;
		SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0;
		delay = SYSCTL_RCGCGPIO_R;		// Esperar a que se activen los modulos
		// Configurar funciones alternativas en los pines PA2, 3, 5
		GPIO_PORTA_AFSEL_R |= 0x2c;
		// Configurar la funcion de SSI en los pines
		GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xff0f00ff) + GPIO_PCTL_PA2_SSI0CLK + GPIO_PCTL_PA3_SSI0FSS + GPIO_PCTL_PA5_SSI0TX;
		// Configurar los registros de funciones digitales, configurar pull-up o pull-down alternativamente
		GPIO_PORTA_AMSEL_R = 0;
		GPIO_PORTA_DEN_R |= 0x2c;

		// Configurar los formatos de trama
		// Deshabilitar el modulo SSI antes de hacer cambios
		SSI0_CR1_R &= ~SSI_CR1_SSE;
		// Connfigurar la operacion como maestro
		SSI0_CR1_R &= ~SSI_CR1_MS;
		// Configurar la fuente de reloj como reloj del sistema basado en un factor de division
		SSI0_CC_R &= ~SSI_CC_CS_M;
		// Configurar el prescaler para una frecuencia del modulo SSI de 1Mhz = 40MHz/40
		SSI0_CPSR_R = (SSI0_CPSR_R & ~SSI_CPSR_CPSDVSR_M) + 20;
		// Configurar el serial clock rate, polaridad del reloj y fase, protocolo y tamaño de los datos
		SSI0_CR0_R &= ~(SSI_CR0_SCR_M); 	// SCR = 0
	//	SSI0_CR0_R |= SSI_CR0_SPO;			// SPO = 1
	//	SSI0_CR0_R |= SSI_CR0_SPH;			// SPH = 1
	//	SSI0_CR0_R = (SSI0_CR0_R & ~SSI_CR0_FRF_M) + SSI_CR0_FRF_MOTO;	// Freescale SPI
		SSI0_CR0_R = (SSI0_CR0_R & ~SSI_CR0_FRF_M) + SSI_CR0_FRF_TI;	// Texas Instruments Synchronous Serial Frame Format
		SSI0_CR0_R = (SSI0_CR0_R & ~SSI_CR0_DSS_M) + SSI_CR0_DSS_16;	// 16 bits de datos

		// Finalmente, habilitar el modulo SPI
		SSI0_CR1_R |= SSI_CR1_SSE;
	#else
		// Habilitar el reloj
		SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

		// Configurar los pines
		GPIOPinConfigure(GPIO_PA2_SSI0CLK);
		GPIOPinConfigure(GPIO_PA3_SSI0FSS);
		GPIOPinConfigure(GPIO_PA5_SSI0TX);
		GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5);

		// Configurar el modulo SSI y habilitarlo
		SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_TI, SSI_MODE_MASTER, 2000000, 16);
		SSIEnable(SSI0_BASE);
	#endif
}

void SSI0_Out(unsigned short code) {
	while ((SSI0_SR_R & SSI_SR_TNF) == 0) {}; // Esperar a que haya espacio en la FIFO
	SSI0_DR_R = code;	// Enviar un codigo
}

void DAC_Out(unsigned short code) {
	SSI0_Out((code & 0xfff) + DAC_SPD_SLOW + DAC_PWR_NORMAL);
}
