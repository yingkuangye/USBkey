#include "api.h"
#include "randombytes.h"
#include "hal.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

#define NTESTS 15
#define MLEN 32

// https://stackoverflow.com/a/1489985/1711232
#define PASTER(x, y) x##y
#define EVALUATOR(x, y) PASTER(x, y)
#define NAMESPACE(fun) EVALUATOR(MUPQ_NAMESPACE, fun)


// use different names so we can have empty namespaces
#define MUPQ_CRYPTO_PUBLICKEYBYTES NAMESPACE(CRYPTO_PUBLICKEYBYTES)
#define MUPQ_CRYPTO_SECRETKEYBYTES NAMESPACE(CRYPTO_SECRETKEYBYTES)
#define MUPQ_CRYPTO_BYTES          NAMESPACE(CRYPTO_BYTES)
#define MUPQ_CRYPTO_ALGNAME        NAMESPACE(CRYPTO_ALGNAME)

#define MUPQ_crypto_sign_keypair NAMESPACE(crypto_sign_keypair)
#define MUPQ_crypto_sign NAMESPACE(crypto_sign)
#define MUPQ_crypto_sign_open NAMESPACE(crypto_sign_open)
#define MUPQ_crypto_sign_signature NAMESPACE(crypto_sign_signature)
#define MUPQ_crypto_sign_verify NAMESPACE(crypto_sign_verify)

const uint8_t canary[8] = {
  0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
};

/* allocate a bit more for all keys and messages and
 * make sure it is not touched by the implementations.
 */
static void write_canary(uint8_t *d) {
  for (size_t i = 0; i < 8; i++) {
    d[i] = canary[i];
  }
}

static int check_canary(const uint8_t *d) {
  for (size_t i = 0; i < 8; i++) {
    if (d[i] != canary[i]) {
      return -1;
    }
  }
  return 0;
}


static int test_sign(void)
{
    unsigned char pk[MUPQ_CRYPTO_PUBLICKEYBYTES+16];
    unsigned char sk[MUPQ_CRYPTO_SECRETKEYBYTES+16];
    unsigned char sm[MLEN + MUPQ_CRYPTO_BYTES+16];
    unsigned char m[MLEN+16];

    size_t mlen;
    size_t smlen;

    int i;
    write_canary(pk); write_canary(pk+sizeof(pk)-8);
    write_canary(sk); write_canary(sk+sizeof(sk)-8);
    write_canary(sm); write_canary(sm+sizeof(sm)-8);
    write_canary(m); write_canary(m+sizeof(m)-8);

    for (i = 0; i < NTESTS; i++) {
        MUPQ_crypto_sign_keypair(pk+8, sk+8);
        hal_send_str("crypto_sign_keypair DONE.\n");

        randombytes(m+8, MLEN);
        MUPQ_crypto_sign(sm+8, &smlen, m+8, MLEN, sk+8);
        hal_send_str("crypto_sign DONE.\n");

        // By relying on m == sm we prevent having to allocate CRYPTO_BYTES twice
        if (MUPQ_crypto_sign_open(sm+8, &mlen, sm+8, smlen, pk+8))
        {
           hal_send_str("ERROR Signature did not verify correctly!\n");
        }
        else if(check_canary(pk) || check_canary(pk+sizeof(pk)-8) ||
            check_canary(sk) || check_canary(sk+sizeof(sk)-8) ||
            check_canary(sm) || check_canary(sm+sizeof(sm)-8) ||
            check_canary(m) || check_canary(m+sizeof(m)-8))
        {
            hal_send_str("ERROR canary overwritten\n");
        }
        else
        {
            hal_send_str("OK Signature did verify correctly!\n");
        }
            hal_send_str("crypto_sign_open DONE.\n");
    }

    return 0;
}

static int test_wrong_pk(void)
{
    unsigned char pk[MUPQ_CRYPTO_PUBLICKEYBYTES];
    #ifndef BIG_PUBLIC_KEY_TESTS
    unsigned char pk2[MUPQ_CRYPTO_PUBLICKEYBYTES];
    #else
    unsigned char *pk2 = pk;
    #endif
    unsigned char sk[MUPQ_CRYPTO_SECRETKEYBYTES];
    unsigned char sm[MLEN + MUPQ_CRYPTO_BYTES];
    unsigned char m[MLEN];

    size_t mlen;
    size_t smlen;

    int i;

    for (i = 0; i < NTESTS; i++) {
        #ifndef BIG_PUBLIC_KEY_TESTS
        MUPQ_crypto_sign_keypair(pk2, sk);
       // hal_send_str("crypto_sign_keypair DONE.\n");
        #endif

        MUPQ_crypto_sign_keypair(pk, sk);
        //hal_send_str("crypto_sign_keypair DONE.\n");

        randombytes(m, MLEN);
        MUPQ_crypto_sign(sm, &smlen, m, MLEN, sk);
       // hal_send_str("crypto_sign DONE.\n");

        #ifdef BIG_PUBLIC_KEY_TESTS
        randombytes(pk, sizeof pk);
        #endif

        // By relying on m == sm we prevent having to allocate CRYPTO_BYTES twice
        if (MUPQ_crypto_sign_open(sm, &mlen, sm, smlen, pk2))
        {
           // hal_send_str("OK Signature did not verify correctly under wrong public key!\n");
        }
        else
        {
           // hal_send_str("ERROR Signature did verify correctly under wrong public key!\n");
        }
       // hal_send_str("crypto_sign_open DONE.\n");
    }

    return 0;
}



static void clock_setup(void)
{
	/* Enable GPIOD clock for LED & USARTs. */
	rcc_periph_clock_enable(LED0_CLK);
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for USART2. */
	rcc_periph_clock_enable(RCC_USART2);
}

static void usart_setup(void)
{
	/* Enable the USART2 interrupt. */
	nvic_enable_irq(NVIC_USART2_IRQ);

	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);

	/* Setup GPIO pins for USART2 receive. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO3);

	/* Setup USART2 TX and RX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO3);

	/* Setup USART2 parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Enable USART2 Receive interrupt. */
	usart_enable_rx_interrupt(USART2);

	/* Finally enable the USART. */
	usart_enable(USART2);
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO12 on GPIO port D for LED. */
	gpio_mode_setup(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED1_PIN);
}

unsigned char pk[MUPQ_CRYPTO_PUBLICKEYBYTES+16];
unsigned char sk[MUPQ_CRYPTO_SECRETKEYBYTES+16];
unsigned char sm[MLEN + MUPQ_CRYPTO_BYTES+16];  //signed message
unsigned char m[MLEN+16];//message
size_t mlen;
size_t smlen;

int main(void)
{
    hal_setup(CLOCK_FAST);
	  clock_setup();
	  gpio_setup();
	  usart_setup();	

    //hal_send_str("please enter message!");
    write_canary(pk); write_canary(pk+sizeof(pk)-8);
    write_canary(sk); write_canary(sk+sizeof(sk)-8);
    write_canary(sm); write_canary(sm+sizeof(sm)-8);
    write_canary(m); write_canary(m+sizeof(m)-8);
    MUPQ_crypto_sign_keypair(pk+8, sk+8);
    while(1){
       
    }

    return 0;
}


void usart2_isr(void)
{
	static uint8_t data = 'A';
  static uint8_t len;

	if (((USART_CR1(USART2) & USART_CR1_RXNEIE) != 0) &&
	    ((USART_SR(USART2) & USART_SR_RXNE) != 0)) {
		gpio_toggle(GPIOF, LED1_PIN);
		data = usart_recv(USART2);
    //usart_disable_tx_interrupt(USART2);
	}
  //hal_send_str("please continue entering message!");
  if(data == '\r')  //\n means end of one message
  {
    //sign the message
    //delay
    //hal_send_str("unsigned message");
    //hal_send_str(m+8);

    //m[len+8] = data;
    //hal_send_str("pk");
    //hal_send_str(pk+8);
    //hal_send_str("sk");
    //hal_send_str(sk+8);
   // hal_send_str("crypto_sign_keypair DONE.");
   // randombytes(m+8, MLEN);
    MUPQ_crypto_sign(sm+8, &smlen, m+8, MLEN, sk+8);
    hal_send_str("pk");
    hal_send_str(pk+8);
    hal_send_str("sm");
    hal_send_str(sm+8);
    hal_send_str("transmit done");
    //hal_send_str("message has been cryptoed done!");
    //usart_enable_tx_interrupt(USART2);
   // for(int i = 0;i<=)
   /*
    hal_send_str("Now we vertify the signed message!");
    hal_send_str("message has been sent done!");
            if (MUPQ_crypto_sign_open(sm+8, &mlen, sm+8, smlen, pk+8))
        {
           hal_send_str("ERROR Signature did not verify correctly!\n");
        }
        else if(check_canary(pk) || check_canary(pk+sizeof(pk)-8) ||
            check_canary(sk) || check_canary(sk+sizeof(sk)-8) ||
            check_canary(sm) || check_canary(sm+sizeof(sm)-8) ||
            check_canary(m) || check_canary(m+sizeof(m)-8))
        {
            hal_send_str("ERROR canary overwritten\n");
        }
        else
        {
            hal_send_str("OK Signature did verify correctly!\n");
        }
    hal_send_str("crypto_sign_open DONE.\n");
    */
    len = 0;
    //strcpy(m+8,"");
    for(int i=0;i<=MLEN+16-1-8;i++)
    {
      m[i+8] = '\0';
    }
  }
  else
  { 
    //pingjie string
    m[len+8] = data;
    len = len + 1;
    //hal_send_str(m+8);
    //hal_send_str("please continue entering message!");
  }

}
