#ifndef __MY_TIMER_H_
#define __MY_TIMER_H_

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

#include "esp_err.h"

esp_err_t my_timer_init(void);

void Key_Init(void);
uint8_t Key_GetNum(void);
uint8_t Key_GetState(void);
void Key_Tick(void);


#endif
