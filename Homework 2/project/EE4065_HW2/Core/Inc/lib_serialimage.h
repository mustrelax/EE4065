/*
 * lib_serialimage.h
 */

#ifndef INC_SERIAL_IMAGE_H_
#define INC_SERIAL_IMAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* GÜNCELLEME 1: F7 serisinden F4 serisi HAL kütüphanesine geçirildi */
#include "stm32f4xx_hal.h"
#include "lib_image.h"

#define SERIAL_OK				((int8_t)0)
#define SERIAL_ERROR			((int8_t)-1)

/* * GÜNCELLEME 2: UART Portu değiştirildi.
 * Nucleo kartlarda VCP (PC bağlantısı) genellikle USART2'dir.
 * Eğer .ioc dosyanızda farklı bir UART (örn: USART1) kullandıysanız,
 * buradaki 'huart2' tanımını ona göre (örn: 'huart1') düzeltin.
 */
#define __huart 			huart2
extern UART_HandleTypeDef 	__huart;


int8_t LIB_SERIAL_IMG_Transmit(IMAGE_HandleTypeDef * img);
int8_t LIB_SERIAL_IMG_Receive(IMAGE_HandleTypeDef * img);

#ifdef __cplusplus
}
#endif

#endif /* INC_SERIAL_IMAGE_H_ */
