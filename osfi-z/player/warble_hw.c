#include "warble_hw.h"

xSemaphoreHandle xI2S_semaphore;
xSemaphoreHandle xI2S_semaphore_h;

static uint8_t playback_running = 0;
static uint16_t playback_buffer[2][PLAYBACK_BUFFER_SIZE];
static int playback_decode_ind;
static int playback_decode_pos;


void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    static BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xI2S_semaphore_h, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    //printf("c\n");
    static BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xI2S_semaphore, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}


uint8_t warble_hw_init()
{
    return 1;
}

uint8_t warble_hw_start()
{
    xI2S_semaphore = xSemaphoreCreateCounting(10, 0);
    xI2S_semaphore_h = xSemaphoreCreateCounting(10, 0);

    HAL_I2S_Transmit_DMA(&hi2s3,
			 (uint16_t*)playback_buffer,
			 PLAYBACK_BUFFER_SIZE * 2);
    return 1;
}

uint8_t warble_hw_stop()
{
    HAL_I2S_DMAStop(&hi2s3);
    return 1;
}

uint8_t warble_hw_insert(const void *ch1, const void *ch2,
			 int count,
			 uint8_t stereo_mode)
{
    int i;
    if(stereo_mode == STEREO_INTERLEAVED)
    {
	count *= 2;
    }
    for (i = 0; i < count; i ++) {
	playback_buffer[playback_decode_ind][playback_decode_pos] =
	    (uint16_t)(((uint32_t*)ch1)[i] >> 13);
	playback_decode_pos ++;
	if(stereo_mode == STEREO_NONINTERLEAVED)
	{
	    playback_buffer[playback_decode_ind][playback_decode_pos] =
		(uint16_t)(((uint32_t*)ch2)[i] >> 13);
	    playback_decode_pos ++;
	}
	    
	if(playback_decode_pos >= PLAYBACK_BUFFER_SIZE)
	{
	    if (!playback_running && playback_decode_ind)
		warble_hw_start();
	    if (playback_running && playback_decode_ind)
		xSemaphoreTake(xI2S_semaphore_h, portMAX_DELAY);
	    if (playback_running && !playback_decode_ind)
		xSemaphoreTake(xI2S_semaphore, portMAX_DELAY);

		
	    playback_decode_pos = 0;
	    playback_decode_ind = !playback_decode_ind;
	}
	    
    }

    return 1;
}
