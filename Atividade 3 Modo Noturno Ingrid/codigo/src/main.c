#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define STACK_SIZE   512
#define PRIORIDADE   5
#define TEMPO_MS     4000

static const struct device *gpio_a = DEVICE_DT_GET(DT_NODELABEL(gpioa));
static const struct device *gpio_d = DEVICE_DT_GET(DT_NODELABEL(gpiod));

#define PIN_VERDE     5    /* PTD5  = D9  (gpiod) */
#define PIN_VERMELHO  13   /* PTA13 = D8  (gpioa) */
#define PIN_MODO      2    /* PTD2  = D11 (gpiod) */

K_MUTEX_DEFINE(mutex_semaforo);

static volatile bool modo_noturno;

/* Espera em fatias; retorna false se entrou em modo noturno */
static bool espera_ms(int ms)
{
    for (int t = 0; t < ms && !modo_noturno; t += 50) {
        k_msleep(50);
    }
    return !modo_noturno;
}

/* Espera em fatias enquanto continuar em modo noturno */
static void pisca_ms(int ms)
{
    for (int t = 0; t < ms && modo_noturno; t += 50) {
        k_msleep(50);
    }
}

/* Botao por polling: alterna modo normal/noturno a cada aperto */
static void thread_botao(void *p1, void *p2, void *p3)
{
    bool anterior = false;

    while (1) {
        bool apertado = gpio_pin_get(gpio_d, PIN_MODO) > 0;

        if (apertado && !anterior) {
            modo_noturno = !modo_noturno;
            printk("Botao apertado -> modo_noturno=%d\n", modo_noturno);
            k_msleep(200); /* debounce */
        }
        anterior = apertado;
        k_msleep(30);
    }
}

static void thread_verde(void *p1, void *p2, void *p3)
{
    while (1) {
        if (modo_noturno) {
            k_msleep(100);
            continue;
        }

        k_mutex_lock(&mutex_semaforo, K_FOREVER);

        if (!modo_noturno) {
            gpio_pin_set(gpio_d, PIN_VERDE, 1);
            printk("Pedestres: VERDE\n");
            espera_ms(TEMPO_MS);
            gpio_pin_set(gpio_d, PIN_VERDE, 0);
        }

        k_mutex_unlock(&mutex_semaforo);
        k_msleep(10);
    }
}

static void thread_vermelho(void *p1, void *p2, void *p3)
{
    while (1) {
        if (modo_noturno) {
            k_msleep(100);
            continue;
        }

        k_mutex_lock(&mutex_semaforo, K_FOREVER);

        if (!modo_noturno) {
            gpio_pin_set(gpio_a, PIN_VERMELHO, 1);
            printk("Pedestres: VERMELHO\n");
            espera_ms(TEMPO_MS);
            gpio_pin_set(gpio_a, PIN_VERMELHO, 0);
        }

        k_mutex_unlock(&mutex_semaforo);
        k_msleep(10);
    }
}

/* Modo noturno: vermelho pisca 1 s aceso / 1 s apagado */
static void thread_noturno(void *p1, void *p2, void *p3)
{
    while (1) {
        if (!modo_noturno) {
            k_msleep(50);
            continue;
        }

        k_mutex_lock(&mutex_semaforo, K_FOREVER);
        printk("Modo NOTURNO\n");

        while (modo_noturno) {
            gpio_pin_set(gpio_a, PIN_VERMELHO, 1);
            pisca_ms(1000);
            gpio_pin_set(gpio_a, PIN_VERMELHO, 0);
            pisca_ms(1000);
        }

        gpio_pin_set(gpio_a, PIN_VERMELHO, 0);
        printk("Modo NORMAL\n");
        k_mutex_unlock(&mutex_semaforo);
    }
}

K_THREAD_DEFINE(tid_botao, STACK_SIZE, thread_botao, NULL, NULL, NULL,
                PRIORIDADE, 0, 0);
K_THREAD_DEFINE(tid_verde, STACK_SIZE, thread_verde, NULL, NULL, NULL,
                PRIORIDADE, 0, 0);
K_THREAD_DEFINE(tid_vermelho, STACK_SIZE, thread_vermelho, NULL, NULL, NULL,
                PRIORIDADE, 0, 0);
K_THREAD_DEFINE(tid_noturno, STACK_SIZE, thread_noturno, NULL, NULL, NULL,
                PRIORIDADE, 0, 0);

int main(void)
{
    if (!device_is_ready(gpio_a) || !device_is_ready(gpio_d)) {
        printk("GPIO nao esta pronto\n");
        return 0;
    }

    gpio_pin_configure(gpio_d, PIN_VERDE,    GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(gpio_a, PIN_VERMELHO, GPIO_OUTPUT_INACTIVE);

    /* Botao ligado ao GND: apertado = nivel baixo (ACTIVE_LOW) */
    gpio_pin_configure(gpio_d, PIN_MODO,
                       GPIO_INPUT | GPIO_PULL_UP | GPIO_ACTIVE_LOW);

    printk("Nivel do botao no boot (1 = solto, 0 = apertado): %d\n",
           gpio_pin_get_raw(gpio_d, PIN_MODO));

    return 0;
}
