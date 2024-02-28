/*******/
/* Curso DSPIC Básico							               */
/* Ej. 13: ADC comunicacion SERIAL                            */
/* Killmu Technology LIMA-PERU 2018                            */
/* FACEBOOK-->https://www.facebook.com/MasterProgramadorDsPic/ */
/* WHATSAPP-->(+51)947 812259                                  */
/*******/

// DSPIC33FJ32MC204 Configuration Bit Settings
// 'C' source line config statements
// FBS
#pragma config BWRP = WRPROTECT_OFF     // Boot Segment Write Protect (Boot Segment may be written)
#pragma config BSS = NO_FLASH           // Boot Segment Program Flash Code Protection (No Boot program Flash segment)

// FGS
#pragma config GWRP = OFF               // General Code Segment Write Protect (User program memory is not write-protected)
#pragma config GSS = OFF                // General Segment Code Protection (User program memory is not code-protected)

// FOSCSEL
#pragma config FNOSC = PRIPLL           // Oscillator Mode (Primary Oscillator (XT, HS, EC) w/ PLL)
#pragma config IESO = OFF               // Internal External Switch Over Mode (Start-up device with user-selected oscillator source)

// FOSC
#pragma config POSCMD = HS              // Primary Oscillator Source (HS Oscillator Mode)
#pragma config OSCIOFNC = OFF           // OSC2 Pin Function (OSC2 pin has clock out function)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow Only One Re-configuration)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor (Both Clock Switching and Fail-Safe Clock Monitor are disabled)

// FWDT
#pragma config WDTPOST = PS32768        // Watchdog Timer Postscaler (1:32,768)
#pragma config WDTPRE = PR128           // WDT Prescaler (1:128)
#pragma config WINDIS = OFF             // Watchdog Timer Window (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (Watchdog timer enabled/disabled by user software)

// FPOR
#pragma config FPWRT = PWR128           // POR Timer Value (128ms)
#pragma config ALTI2C = OFF             // Alternate I2C  pins (I2C mapped to SDA1/SCL1 pins)
#pragma config LPOL = ON                // Motor Control PWM Low Side Polarity bit (PWM module low side output pins have active-high output polarity)
#pragma config HPOL = ON                // Motor Control PWM High Side Polarity bit (PWM module high side output pins have active-high output polarity)
#pragma config PWMPIN = ON              // Motor Control PWM Module Pin Mode bit (PWM module pins controlled by PORT register at device Reset)

// FICD
#pragma config ICS = PGD1               // Comm Channel Select (Communicate on PGC1/EMUC1 and PGD1/EMUD1)
#pragma config JTAGEN = OFF             // JTAG Port Enable (JTAG is Disabled)


// librerias
#include "xc.h"
#include <libpic30.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define Speed 40     //MIPS deseados (maximo 40 para el dspic33fj32mc204)
#define Crystal_Externo 20   //valor del cristal en MHZ       
#define Freq Speed*1000000
#define delay_ms(x) __delay32((Freq/1000)*x) //delay en milisegundos
#define delay_us(x) __delay32(Speed*x) //delay en microsegundos

#define BAUDRATE 9600
#define BRGVAL ((Freq/BAUDRATE)/16)-1

#define FILTER_ORDER 4  // Orden del filtro (número de coeficientes - 1)

void Reloj_PLL(void);
void Configurar_UART();
void Configurar_ADC();
void Configurar_IO();
void ADC_Start();
void ADC_Update_Data();
void Serial_PutChar(char);
void Serial_SendString(char *);
void enviarNumero(int);
void enviarNumeroTerminal(int);

float highPassFilterFIR(int);

char initial_Message[] = "Hola ESP32!\n\0";
char RxByte, TxByte;
char Salto[] = "\r";
unsigned long int ADC_Data_Buffer;

float filterCoefficients[FILTER_ORDER + 1] = { // Coeficientes del filtro
    // Estos son solo ejemplos, necesitarás calcular los coeficientes adecuados para tu frecuencia de corte deseada
    0.1, -0.3, 0.5, -0.3, 0.1
};

float sampleBuffer[FILTER_ORDER + 1] = {0};  // Buffer para almacenar las muestras anteriores


/* Función de Interrupción de Recepción

void attribute((interrupt, no_auto_psv)) _U1RXInterrupt(void) {

    U1TXREG = U1RXREG;
    RxByte = U1RXREG;
    _U1RXIF = 0; // Borramos flag.
}

*/

/* Función Principal */

int main(void) {
    AD1PCFGL = 0xffff;
    Reloj_PLL();
    Configurar_IO();
    Configurar_ADC();
    ADC_Start();
    Configurar_UART();
    Serial_SendString(initial_Message);
    while (1) {
        while (!IFS0bits.AD1IF);
        IFS0bits.AD1IF = 0;
        ADC_Update_Data();
        float result = highPassFilterFIR(ADC_Data_Buffer);
        enviarNumeroTerminal((int) ADC_Data_Buffer);
        Serial_PutChar('-');
        enviarNumeroTerminal((int) result);
        Serial_PutChar('.');
        Serial_SendString(Salto);
    }
    return 0;
}

void Reloj_PLL(void) {//con oscilador externo
    int M = Speed * 8 / Crystal_Externo;
    PLLFBD = M - 2; // M = 28
    CLKDIVbits.PLLPRE = 0; // N1 = 2
    CLKDIVbits.PLLPOST = 0; // N2 = 2
    while (_LOCK == 0);
}

/* Función de Configuración del UART */
void Configurar_UART() { // Baudrate is set to 9600 (@ 7.37MHz) 
    //TRISFbits.TRISF5 = 0;	/* Configura como pin Transmisor */
    RPINR18 = 0x0000; //RP0//rp14 como U1RX
    RPOR0 = 0x0300; //RP1 como U1TX

    U1BRG = BRGVAL;
    // 8-bits, 1 bit stop, Flow ctrl mode
    U1MODE = 0;
    U1STA = 0;
    // Enable UART1
    U1MODEbits.UARTEN = 1;
    // Enable Transmit
    U1STAbits.UTXEN = 1;
    // reset RX flag
    IFS0bits.U1RXIF = 0;
    //Enable Rx Interrupt
    IEC0bits.U1RXIE = 1;
}

/* Función de Configuración de los Puertos de Entrada/Salida */
void Configurar_ADC() {
    AD1CON1 = 0x00E4; // Formato del resultado entero, Comienzo de la conversión automático, 
    // selección de la muestra después que la conversión termina,10bits            

    AD1CON2 = 0x003C; // Referencias: AVDD y AVSS, se deshabilita el Modo Scan para el Canal 0
    // Se selecciona interrup. después de una secuencia de 16 s/c
    // Se selecciona niveo de buffer 16*1 y siempre se usa el MUX A           

    AD1CON3 = 0x0353; // Clock del ADC del clock interno, Tiempo de muestreo= 13 TAD
    // Frecuencia del Clock ADC->16 muestras en 1 ms            

    AD1CHS0 = 0x0000; // Entrada positiva AN0 para la entrada de la muestra
    // Entrada negativa se usa el VR-

    AD1PCFGL = 0xFFFE; // Se configura el AD1PCFG para que el único pin 
    // usado analógicamente sea el AN0

    AD1CSSL = 0x0000; // Como el scaneo de canales no está habilitado
    // Ningún canal debe ser seleccionado para escaneo
}

void Configurar_IO() {
    RPINR18 = 0x0000; //RP0 como U1RX
    RPOR0 = 0x0300; //RP1 como U1TX
}

void ADC_Start() {
    AD1CON1bits.ADON = 1; // Prende el ADC
}

void ADC_Update_Data() {
    unsigned long int temp;
    temp = ADC1BUF0;
    ADC_Data_Buffer = temp / 2; /* De 10 bits a 8 bits */
}

// Send a character out to the serial interface.

void Serial_PutChar(char Ch) { // wait for empty buffer 
    while (U1STAbits.UTXBF == 1);
    U1TXREG = Ch;
}

// Send a string out to the serial interface.

void Serial_SendString(char *str) {
    char * p;
    p = str;
    while (*p)
        Serial_PutChar(*p++);
}

void enviarNumero(int n) {
    int cant = 0, r, i;
    char s[10];
    do {
        r = n - (n / 10)*10; //residuo
        n = n / 10;
        r = r + 48;
        s[cant] = (char) (r);
        cant++;
    } while (n != 0);

    i = cant - 1;
    while (s[i]) // loop until *s == ?\0?, end of string
        Serial_PutChar(s[i--]); // send the character and point to the next one
}

void enviarNumeroTerminal(int n) {
    int u, d, c;
    u = n / 100;
    d = (n % 100) / 10;
    c = n % 10;
    enviarNumero(u);
    enviarNumero(d);
    enviarNumero(c);
}

// Función para aplicar el filtro FIR de paso alto
float highPassFilterFIR(int inputValue) {
    // Desplazar las muestras antiguas hacia abajo en el buffer
    for (int i = FILTER_ORDER; i > 0; i--) {
        sampleBuffer[i] = sampleBuffer[i - 1];
    }

    // Añadir la nueva muestra al inicio del buffer
    sampleBuffer[0] = (float)inputValue;

    // Calcular la salida del filtro
    float outputValue = 0;
    for (int i = 0; i <= FILTER_ORDER; i++) {
        outputValue += filterCoefficients[i] * sampleBuffer[i];
    }

    return outputValue;
}