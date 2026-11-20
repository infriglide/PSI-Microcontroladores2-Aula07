#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define STACK_SIZE   512
#define PRIORIDADE   5
#define TEMPO_MS     4000

/* PTD5 = D9 (verde) | PTA13 = D8 (vermelho) */
static const struct device *gpio_a = DEVICE_DT_GET(DT_NODELABEL(gpioa));
static const struct device *gpio_d = DEVICE_DT_GET(DT_NODELABEL(gpiod));

#define PIN_VERDE     5    /* PTD5  */
#define PIN_VERMELHO  13   /* PTA13 */

K_MUTEX_DEFINE(mutex_semaforo);

static void thread_verde(void *p1, void *p2, void *p3)
{
    while (1) {
        k_mutex_lock(&mutex_semaforo, K_FOREVER);

        gpio_pin_set(gpio_d, PIN_VERDE, 1);
        printk("Pedestres: VERDE\n");
        k_msleep(TEMPO_MS);
        gpio_pin_set(gpio_d, PIN_VERDE, 0);

        k_mutex_unlock(&mutex_semaforo);
        k_msleep(10); /* dá chance para a outra thread pegar o mutex */
    }
}

static void thread_vermelho(void *p1, void *p2, void *p3)
{
    while (1) {
        k_mutex_lock(&mutex_semaforo, K_FOREVER);

        gpio_pin_set(gpio_a, PIN_VERMELHO, 1);
        printk("Pedestres: VERMELHO\n");
        k_msleep(TEMPO_MS);
        gpio_pin_set(gpio_a, PIN_VERMELHO, 0);

        k_mutex_unlock(&mutex_semaforo);
        k_msleep(10);
    }
}

K_THREAD_DEFINE(tid_verde, STACK_SIZE, thread_verde, NULL, NULL, NULL,
                PRIORIDADE, 0, 0);
K_THREAD_DEFINE(tid_vermelho, STACK_SIZE, thread_vermelho, NULL, NULL, NULL,
                PRIORIDADE, 0, 0);

int main(void)
{
    if (!device_is_ready(gpio_a) || !device_is_ready(gpio_d)) {
        printk("GPIO nao esta pronto\n");
        return 0;
    }

    gpio_pin_configure(gpio_d, PIN_VERDE,    GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure(gpio_a, PIN_VERMELHO, GPIO_OUTPUT_INACTIVE);

    return 0;
}