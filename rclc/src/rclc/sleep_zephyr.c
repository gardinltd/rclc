#include "rclc/sleep.h"

#include <zephyr/kernel.h>

void rclc_sleep_ms(unsigned int ms)
{
	k_msleep(ms);
}
