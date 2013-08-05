//******************************************************************************
//
//                Slave
//               MSP430F20x2
//             -----------------          -----------------
//            |              XIN|-       |                 |
//            |                 |        |                 |
//           -|P1.0         XOUT|-       |                 |
//           -|P1.1             |        |                 |
//       ADC -|P1.2             |        |                 |
//           -|P1.3             |        |                 |
//           -|P1.4             |        |                 |
//            |         SDI/P1.7|<-------|MOSI             |
//            |         SDO/P1.6|------->|MISO             |
//            |        SCLK/P1.5|<-------|SCLK             |
//
//						|Address |    Data    | 
//						+--------+------------+
//						|  0x00  | ADC_CHAN_0 |
//						|  0x01  | ADC_CHAN_1 |
//						|  0x02  | ADC_CHAN_2 |
//						|  0x03  | ADC_CHAN_3 |
//						|  0x04  | ADC_CHAN_4 |
//
//
//******************************************************************************

#include <msp430f2002.h>

#define 	CS 			BIT7
#define 	LED			BIT6
#define		MISO		BIT6
#define		MOSI		BIT7
#define		CLK			BIT5

#define 	N_CHANNELS		5
#define		INT_PORT_ON		(P2IE |= CS)
#define		INT_PORT_OFF	(P2IE &= ~CS)
#define		INT_USI_ON		(USICTL1 |=  USIIE)
#define		INT_USI_OFF		(USICTL1 &= ~USIIE)

volatile int dataADC[N_CHANNELS];

void InitADC(int n_chan)
{
	int i;

	ADC10CTL0 = SREF_0 + ADC10SHT_1 + ADC10ON /*+ ADC10IE*/;

	for (i = 0; i < n_chan; ++i)
	{
		// BITX
		ADC10AE0 |= 1 << i;
	}
}

void getDataADC(int chan)
{
	ADC10CTL0 &= ~ENC;

	// chan = INCH_X
	ADC10CTL1 = chan << 12;
	ADC10CTL0 |= ENC + ADC10SC;
	// LPM0;

	while ((ADC10CTL1 & ADC10BUSY) == 0x01);
    dataADC[chan] = ADC10MEM;
}

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer

	// Устанавливаем частоту DCO на калиброванные 1 MHz.
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

	USICTL0 |= USIPE7 + USIPE5 + USIPE6 + USIOE; // Port, SPI slave
	USICTL0 &= ~USISWRST;                 // USI released for operation
	// USICTL1 |= USIIE;
	INT_USI_OFF;
	USISRL = 0xAA;
	USICNT = 8;


	P2DIR &= ~CS;
	P2SEL &= ~CS;
	P2OUT |= CS;
	P2REN |= CS;
	P2IES &= ~CS;
	P2IFG &= ~CS;

	P2SEL &= ~LED;
	P2DIR |= LED;
	P2OUT |= LED;
	P2OUT ^= LED;

	P1DIR  &= ~(MOSI + CLK);
	P1REN |= MOSI + CLK;
	P1DIR &= ~MISO;

	// P1OUT &= ~(MISO);

	// ADC10CTL0 = SREF_1 + ADC10SHT_1 + ADC10ON + ADC10IE + REFON + REF2_5V;

	P2OUT |= LED;
	InitADC(N_CHANNELS);

	_BIS_SR(GIE);

	while(1)
	{
		int i;
		INT_PORT_OFF;
		for (i = 0; i < N_CHANNELS; ++i)
		{
			getDataADC(i);
		}
		INT_PORT_ON;
	}
}

#pragma vector=USI_VECTOR
__interrupt void universal_serial_interface(void)
{
	if (USISRL < 5)	USISR = dataADC[USISRL];
	else			USISR = 1;
	USICNT = 8;
	P2OUT ^= LED;
}

#pragma vector=PORT2_VECTOR
__interrupt void Port_1(void)
{
	INT_PORT_OFF;

	// Переход CS 1 -> 0
	if (P2IES & CS)
	{
		// линию DO - выход
		P1DIR |= MISO;
		INT_USI_ON;
	}
	else
	{	
		// линию DO - вход
		// дабы исключить КЗ
		P1DIR &= ~MISO;
		P2OUT |= LED;
		INT_USI_OFF;
	}

	P2IFG &= ~CS;
	P2IES ^= CS;
	INT_PORT_ON;
}