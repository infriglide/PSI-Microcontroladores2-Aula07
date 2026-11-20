#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>

#define STACK_SIZE 1024
#define PRIO 5

enum { VERDE, AMARELO, VERMELHO };

struct led {
    const struct device *port;
    gpio_pin_t pin;
    const char *nome;
};

static const struct led leds[3] = {
    [VERDE]    = { DEVICE_DT_GET(DT_NODELABEL(gpiod)), 4,  "VERDE" },
    [AMARELO]  = { DEVICE_DT_GET(DT_NODELABEL(gpioa)), 12, "AMARELO" },
    [VERMELHO] = { DEVICE_DT_GET(DT_NODELABEL(gpioa)), 5,  "VERMELHO" },
};

static const int tempo_ms[3] = { 3000, 1000, 4000 };

K_MUTEX_DEFINE(mtx);
static volatile int vez = VERDE;

static void semaforo_thread(void *p1, void *p2, void *p3)
{
    int id = (int)(intptr_t)p1;

    while (1) {
        k_mutex_lock(&mtx, K_FOREVER);
        if (vez == id) {
            gpio_pin_set(leds[id].port, leds[id].pin, 1);
            printk("[%lld ms] %s ON\n", k_uptime_get(), leds[id].nome);
            k_msleep(tempo_ms[id]);
            gpio_pin_set(leds[id].port, leds[id].pin, 0);
            printk("[%lld ms] %s OFF\n", k_uptime_get(), leds[id].nome);
            vez = (id + 1) % 3;
        }
        k_mutex_unlock(&mtx);
        k_msleep(10);
    }
}

K_THREAD_DEFINE(t_verde,    STACK_SIZE, semaforo_thread, (void *)VERDE,    NULL, NULL, PRIO, 0, 200);
K_THREAD_DEFINE(t_amarelo,  STACK_SIZE, semaforo_thread, (void *)AMARELO,  NULL, NULL, PRIO, 0, 200);
K_THREAD_DEFINE(t_vermelho, STACK_SIZE, semaforo_thread, (void *)VERMELHO, NULL, NULL, PRIO, 0, 200);

int main(void)
{
    for (int i = 0; i < 3; i++) {
        if (!device_is_ready(leds[i].port)) {
            printk("Porta do LED %s nao esta pronta\n", leds[i].nome);
            return 0;
        }
        gpio_pin_configure(leds[i].port, leds[i].pin, GPIO_OUTPUT_INACTIVE);
    }
    printk("Semaforo de veiculos iniciado\n");
    return 0;
}