/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_AI_H
#define __APP_AI_H
#ifdef __cplusplus
extern "C" {
#endif
/**
  ******************************************************************************
  * @file    app_x-cube-ai.h
  * @author  X-CUBE-AI C code generator
  * @brief   AI entry function definitions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stai.h"
#include "ai_datatypes_defines.h"

#include "network.h"

#define MIN_HEAP_SIZE 0x800
#define MIN_STACK_SIZE 0x2000

/* Input defs ----------------------------------------------------------------*/


#define STAI_MNETWORK_IN_NUM 1
#define STAI_MNETWORK_IN_1_SIZE_BYTES 4
#define STAI_MNETWORK_IN_1_SIZE 1

#define DEF_DATA_IN \
STAI_ALIGNED(32) static uint8_t data_in_1[STAI_MNETWORK_IN_1_SIZE_BYTES]; \
stai_ptr data_ins[] = { \
  data_in_1 \
}; 


/* Output defs ----------------------------------------------------------------*/

#define STAI_MNETWORK_OUT_NUM 1
#define STAI_MNETWORK_OUT_1_SIZE_BYTES 4
#define STAI_MNETWORK_OUT_1_SIZE 1

#define DEF_DATA_OUT \
STAI_ALIGNED(32) static uint8_t data_out_1[STAI_MNETWORK_OUT_1_SIZE_BYTES]; \
stai_ptr data_outs[] = { \
  data_out_1 \
}; 

/* IO buffers ----------------------------------------------------------------*/

extern stai_ptr data_ins[];
extern stai_ptr data_outs[];

extern stai_ptr data_activations[];
extern stai_ptr data_states[];
void STM32CubeAI_Studio_AI_Init(void);
void STM32CubeAI_Studio_AI_Process(void);
/* USER CODE BEGIN includes */
/* USER CODE END includes */


/* Multiple network support --------------------------------------------------*/

typedef struct {
    const char *name;
    stai_size (*get_context_size)(void);
    stai_return_code (*init)(stai_network* network);
    stai_return_code (*deinit)(stai_network* network);
    stai_return_code (*run)(stai_network* network, const stai_run_mode mode);
    stai_return_code (*get_info)(stai_network* network, stai_network_info* info);
    stai_return_code (*get_error)(stai_network* network);

    stai_return_code (*get_activations)(stai_network* network, stai_ptr* activations, stai_size *n_activations);
    stai_return_code (*get_states)(stai_network* network, stai_ptr* states, stai_size *n_states);
    stai_return_code (*get_weights)(stai_network* network, stai_ptr* weights, stai_size *n_weights);
    stai_return_code (*get_inputs)(stai_network* network, stai_ptr* inputs, stai_size* n_inputs);
    stai_return_code (*get_outputs)(stai_network* network, stai_ptr* outputs, stai_size* n_outputs);

    stai_return_code (*set_activations)(stai_network* network, const stai_ptr* activations, const stai_size n_activations);
    stai_return_code (*set_states)(stai_network* network, const stai_ptr* states, const stai_size n_states);
    stai_return_code (*set_weights)(stai_network* network, const stai_ptr* weights, const stai_size n_weights);
    stai_return_code (*set_inputs)(stai_network* network, const stai_ptr* inputs, const stai_size n_inputs);
    stai_return_code (*set_outputs)(stai_network* network, const stai_ptr* outputs, const stai_size n_outputs);
    stai_return_code (*set_callback)(stai_network* network, const stai_event_cb cb, void* cb_cookie);

    stai_network *context;
    stai_size n_inputs;
    stai_size n_outputs;
    stai_size n_activations;
    stai_size n_states;
    stai_size n_weights;
    uintptr_t *weights_addr;

    const stai_network_details *details;

} stai_network_entry_t;

#define STAI_MNETWORK_NUMBER  (1)

STAI_API_DECLARE_BEGIN

STAI_API_ENTRY
const char* stai_mnetwork_find(const char *name, int idx);

STAI_API_ENTRY
stai_return_code stai_mnetwork_init(const char *name, stai_ptr* network);

STAI_API_ENTRY
stai_return_code stai_mnetwork_deinit(stai_ptr network);

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_info(stai_ptr network, stai_network_info* report);

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_error(stai_ptr network);

STAI_API_ENTRY
stai_return_code stai_mnetwork_run(stai_ptr network);

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_activations(stai_ptr network, const stai_ptr* activations);

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_states(stai_ptr network, const stai_ptr* states);

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_weights(stai_ptr network, const stai_ptr* weights);

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_inputs(stai_ptr network, const stai_ptr* inputs);

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_outputs(stai_ptr network, const stai_ptr* outputs);

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_callback(stai_ptr network, const stai_event_cb cb, void* cb_cookie);

STAI_API_ENTRY
const stai_network_details *stai_mnetwork_get_details(stai_ptr network);

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_activations(stai_ptr network, stai_ptr* activations);

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_states(stai_ptr network, stai_ptr* states);

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_weights(stai_ptr network, stai_ptr* weights);

STAI_API_ENTRY
stai_ptr *stai_mnetwork_get_weights_ext(stai_ptr network);

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_inputs(stai_ptr network, stai_ptr* inputs);

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_outputs(stai_ptr network, stai_ptr* outputs);

STAI_API_DECLARE_END
    

#ifdef __cplusplus
}
#endif
#endif /*__STMicroelectronics_ST_EDGE_AI_3.0.0-20426 123672867_H */
