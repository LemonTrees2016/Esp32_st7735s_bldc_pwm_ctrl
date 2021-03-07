#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

#include "mcpwm_control.h"
#include "st7735s.h"


#define GPIO_INPUT_IO_A     32
#define GPIO_INPUT_IO_C     33
#define GPIO_INPUT_PIN_SEL \ 
	((1ULL<<GPIO_INPUT_IO_A) | \
	(1ULL<<GPIO_INPUT_IO_C))


#define GPIO_INPUT_IO_SWITCH_UP       	19
#define GPIO_INPUT_IO_SWITCH_DOWN     	21
#define GPIO_INPUT_IO_SWITCH_LEFT     	22
#define GPIO_INPUT_IO_SWITCH_RIGHT     	12

#define GPIO_INPUT_IO_SWITCH_OK     	16
#define GPIO_INPUT_IO_SWITCH_SELECT     27

#define GPIO_INPUT_IO_SWITCH_A       	17
#define GPIO_INPUT_IO_SWITCH_B     		14
#define GPIO_INPUT_IO_SWITCH_C     		34
#define GPIO_INPUT_IO_SWITCH_D     		35


#define GPIO_INPUT_IO_ROTATE     		25	

#define GPIO_INPUT_SWITCHS_SEL  ((1ULL<<GPIO_INPUT_IO_SWITCH_UP) | \
								 (1ULL<<GPIO_INPUT_IO_SWITCH_DOWN) | \
								 (1ULL<<GPIO_INPUT_IO_SWITCH_LEFT) | \
								 (1ULL<<GPIO_INPUT_IO_SWITCH_RIGHT) | \
								 (1ULL<<GPIO_INPUT_IO_SWITCH_SELECT) | \
								 (1ULL<<GPIO_INPUT_IO_SWITCH_A) | \
								 (1ULL<<GPIO_INPUT_IO_SWITCH_B) | \
								 (1ULL<<GPIO_INPUT_IO_SWITCH_C) | \
								 (1ULL<<GPIO_INPUT_IO_SWITCH_C) | \
								 (1ULL<<GPIO_INPUT_IO_ROTATE))




#define GPIO_INPUT_IO_SWITCH     25
#define GPIO_INPUT_IO_SWITCH1     19
#define GPIO_INPUT_IO_SWITCH2     21
#define GPIO_INPUT_IO_SWITCH3     22

#define GPIO_INPUT_SWITCH_SEL  ((1ULL<<GPIO_INPUT_IO_SWITCH) | \
	(1ULL<<GPIO_INPUT_IO_SWITCH1) | \
	(1ULL<<GPIO_INPUT_IO_SWITCH2) | \
	(1ULL<<GPIO_INPUT_IO_SWITCH3))



#define ESP_INTR_FLAG_DEFAULT 0


static xQueueHandle gpio_evt_queue = NULL; //����һ�����з��ر���
uint32_t g_switch_flag=0;

uint32_t g_switch1_flag=0;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

char g_LevelRcd[4] = {0};
unsigned int g_rcdIdx = 0;
int g_Cnt = 0;
unsigned int g_ReadFlg = 0;
int g_Cnt1 = 0;

static void IRAM_ATTR ProcRotate()
{
	char PosRotate[] = {3,2,0,1};
	char NegRotate[] = {3,1,0,2};
	char CompareRotate[4]={0};

	g_LevelRcd[g_rcdIdx % 4] = ((gpio_get_level(GPIO_INPUT_IO_A) & 0x1) << 1) + (gpio_get_level(GPIO_INPUT_IO_C) & 0x1);
	
	g_rcdIdx += 1;

	if ((g_rcdIdx >= 4) && (g_rcdIdx % 4 == 0)) {
		CompareRotate[0] = g_LevelRcd[(g_rcdIdx - 4) % 4];
		CompareRotate[1] = g_LevelRcd[(g_rcdIdx - 3) % 4];
		CompareRotate[2] = g_LevelRcd[(g_rcdIdx - 2) % 4];
		CompareRotate[3] = g_LevelRcd[(g_rcdIdx - 1) % 4];
		
#if 0
		// �ҵ���ʼλ��
		char startIdx = 0;
		for (startIdx = 0; startIdx < 4; startIdx++) {
			if (CompareRotate[startIdx] == )
		}
#endif
		ESP_LOGI("CompareRotate","%d,%d,%d,%d\n", CompareRotate[0], CompareRotate[1], CompareRotate[2], CompareRotate[3]);
		if (memcmp(PosRotate, CompareRotate, 4) == 0) {
			ESP_LOGI("PosRotate","%d,%d,%d,%d\n", PosRotate[0], PosRotate[1], PosRotate[2], PosRotate[3]);
			ESP_LOGI("g_Cnt","%d\n", g_Cnt);
			g_Cnt++;
			return;
		}

		if (memcmp(NegRotate, CompareRotate, 4) == 0) {
			ESP_LOGI("NegRotate","%d,%d,%d,%d\n", NegRotate[0], NegRotate[1], NegRotate[2], NegRotate[3]);
			ESP_LOGI("g_Cnt","%d\n", g_Cnt);
			g_Cnt--;
			return;
		}
	}
	
	return;
}

static void IRAM_ATTR ProcRotateNum()
{
	const TickType_t xDelay = 10 / portTICK_PERIOD_MS; //delay 3ms
	esp_err_t rslt;
	uint32_t lastA = gpio_get_level(GPIO_INPUT_IO_A);
	uint32_t lastC = gpio_get_level(GPIO_INPUT_IO_C);
	uint32_t CLevel = 0; 
	uint32_t ALevel = 0; 
	uint8_t delta = 1;

	// ����
	esp_task_wdt_add(NULL);
	
	for(;;) {
		ALevel = gpio_get_level(GPIO_INPUT_IO_A);//GPIO_INPUT_IO_C
		CLevel = gpio_get_level(GPIO_INPUT_IO_C);//GPIO_INPUT_IO_A
	
		if ((lastA != ALevel)) {
			if (lastA == 1) {
				if (CLevel == 1) {
					if (g_switch_flag % 2) {
						g_Cnt1+=delta;
					} else {
						g_Cnt+=delta;
					}
						SetPwmFlag();
					g_ReadFlg=1;
					ESP_LOGI("g_Cnt","%d\n", g_Cnt);
				}
				
				//ESP_LOGI("lastA","%d\n", lastA);
				//ESP_LOGI("CLevel","%d\n", CLevel);
				
			}
			
			lastA = ALevel;
			lastC = CLevel;
		}
		if ((lastC != CLevel)) {
			if (lastC == 1) {
				if (ALevel == 1) {
					if (g_switch_flag % 2) {
						g_Cnt1-=delta;
					} else {
						g_Cnt-=delta;
					}
					SetPwmFlag();
					g_ReadFlg=1;
					ESP_LOGI("g_Cnt","%d\n", g_Cnt);
				}
				
				//ESP_LOGI("lastC","%d\n", lastC);
				//ESP_LOGI("ALevel","%d\n", ALevel);
			}
			lastC = CLevel;
			lastA = ALevel;
		}
		

		vTaskDelay(xDelay);
		rslt = esp_task_wdt_reset();
		if (ESP_OK != rslt) {
			ESP_LOGI("rslt","%d, %d, %d\n", rslt, ESP_ERR_NOT_FOUND, ESP_ERR_INVALID_STATE);
		}
	}
}


static void IRAM_ATTR ProcRotateNum2()
{
	const TickType_t xDelay = 5 / portTICK_PERIOD_MS; //delay 3ms
	esp_err_t rslt;
	uint32_t lastA = gpio_get_level(GPIO_INPUT_IO_A);
	uint32_t CLevel = 0; 
	uint32_t ALevel = 0; 
	uint32_t flag = 0;
	uint32_t turn = 0;
	
	
	for(;;) {
		ALevel = gpio_get_level(GPIO_INPUT_IO_A);
		CLevel = gpio_get_level(GPIO_INPUT_IO_C);

		if ((CLevel == 1) && (ALevel == 1)) {
			flag = 1;
		}

		if (flag) {
			if (CLevel != ALevel) {
				turn = ALevel;
			}

			if ((CLevel == 0) && (ALevel == 0)) {
				flag = 0;
				if (turn) {
					g_Cnt--;
				} else {
					g_Cnt++;
				}
				ESP_LOGI("g_Cnt","%d\n", g_Cnt);
			}
			
		}

		vTaskDelay(xDelay);

	}
}

void IRAM_ATTR initSwitchGpio()
{
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;//interrupt of rising edge
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;//bit mask of the pins, use GPIO32 here
	io_conf.mode = GPIO_MODE_INPUT;//set as input mode
	io_conf.pull_up_en = 0;//enable pull-up mode
	gpio_config(&io_conf);

}

static void IRAM_ATTR AppRotate()
{
	initSwitchGpio();
	xTaskCreate(ProcRotateNum, "ProcRotateNum", 2048, NULL, 10, NULL);	
}

typedef struct {
	uint32_t io_num;
	char switchName[30];
} switchStru;

switchStru switchName[10] = {
	{GPIO_INPUT_IO_SWITCH_UP, "UP"},
	{GPIO_INPUT_IO_SWITCH_DOWN, "DOWN"},
	{GPIO_INPUT_IO_SWITCH_LEFT, "LEFT"},
	{GPIO_INPUT_IO_SWITCH_RIGHT, "RIGHT"},
	{GPIO_INPUT_IO_SWITCH_OK, "OK"},
	{GPIO_INPUT_IO_SWITCH_SELECT, "SELECT"},
	{GPIO_INPUT_IO_SWITCH_A, "A"},
	{GPIO_INPUT_IO_SWITCH_B, "B"},
	{GPIO_INPUT_IO_SWITCH_C, "C"},
	{GPIO_INPUT_IO_SWITCH_D, "D"},
	{GPIO_INPUT_IO_ROTATE, "ROTATE"}
};

char* GetSwitchName(uint32_t io_num)
{

	for (int i = 0; i < 10; i++) {
		if (io_num == switchName[i].io_num) {
			return switchName[i].switchName;
		}
	}

	return "NULL";
}



static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
		if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
		{
			//ProcRotate();
			ESP_LOGI("BUTTON","GPIO[%d] name[%s] intr, val: %d\n", io_num, GetSwitchName(io_num), gpio_get_level(io_num));

			if (io_num == GPIO_INPUT_IO_ROTATE) {
				g_switch_flag++;
			} else {
				if (g_switch_flag % 2) {
					SetPwmFlag();
					g_Cnt1++;
				} else {
					SetPwmFlag();
					g_Cnt++;
				}
				g_ReadFlg=1;
			}
		}
    }
}

void app_main_gpio()
{
   gpio_config_t io_conf;
   io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;//interrupt of rising edge
   io_conf.pin_bit_mask = GPIO_INPUT_SWITCHS_SEL;//bit mask of the pins, use GPIO32 here
   io_conf.mode = GPIO_MODE_INPUT;//set as input mode
   io_conf.pull_up_en = 0;//enable pull-up mode
   gpio_config(&io_conf);

   //change gpio intrrupt type for one pin
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_UP, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_DOWN, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_LEFT, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_RIGHT, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_OK, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_SELECT, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_A, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_B, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_C, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_SWITCH_D, GPIO_INTR_NEGEDGE);
   gpio_set_intr_type(GPIO_INPUT_IO_ROTATE, GPIO_INTR_NEGEDGE);

   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_UP, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_DOWN, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_LEFT, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_RIGHT, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_OK, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_SELECT, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_A, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_B, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_C, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_SWITCH_D, GPIO_PULLUP_ONLY);
   gpio_set_pull_mode(GPIO_INPUT_IO_ROTATE, GPIO_PULLUP_ONLY);
   
   //create a queue to handle gpio event from isr
   gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));


   //install gpio isr service
   gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
   //hook isr handler for specific gpio pin
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_UP, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_UP);
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_DOWN, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_DOWN);
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_LEFT, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_LEFT);
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_RIGHT, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_RIGHT);
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_OK, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_OK);
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_SELECT, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_SELECT);
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_A, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_A);
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_B, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_B);
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_C, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_C);
   gpio_isr_handler_add(GPIO_INPUT_IO_SWITCH_D, gpio_isr_handler, (void*) GPIO_INPUT_IO_SWITCH_D);
   gpio_isr_handler_add(GPIO_INPUT_IO_ROTATE, gpio_isr_handler, (void*) GPIO_INPUT_IO_ROTATE);

   xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

	AppRotate();

  
}

