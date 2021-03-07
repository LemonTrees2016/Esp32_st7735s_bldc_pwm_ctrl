#ifndef MCPWM_CONTROL_H_
#define MCPWM_CONTROL_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

//void initSwitchGpio();

void SetPwmFlag();
void ClearPwmFlag();
uint8_t GetPwmFlag();
void McPwmCtrl_AppMain(void);

#endif /* BUTTON_INT_H_ */

