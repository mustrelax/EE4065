
/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @author  X-CUBE-AI C code generator
  * @brief   AI program body
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

  /**
    * Description
    * v1.0: Minimum template to show how to use the Embedded Client API ST-AI 
    *
        */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#if defined ( __ICCARM__ )
#define AI_RAM   _Pragma("location=\"AI_RAM\"")
#elif defined ( __CC_ARM ) || ( __GNUC__ )
#define AI_RAM   __attribute__((section(".AI_RAM")))
#endif

/* System headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "app_x-cube-ai.h"
#include "bsp_ai.h"

#include "aiValidation.h"
#include "stai.h"


/* USER CODE BEGIN includes */
/* USER CODE END includes */

/* IO buffers ----------------------------------------------------------------*/


DEF_DATA_IN
DEF_DATA_OUT


/* Global byte buffer to save instantiated C-model network context */
STAI_NETWORK_CONTEXT_DECLARE(network_context, STAI_NETWORK_CONTEXT_SIZE)

/* Activations buffers -------------------------------------------------------*/
STAI_ALIGNED(32) 
AI_RAM 
static uint8_t POOL_0_RAM[STAI_NETWORK_ACTIVATION_1_SIZE_BYTES];


/* Global c-array to handle the activations buffer */
stai_ptr data_activations[] = { POOL_0_RAM };

STAI_ALIGNED(32) static uint8_t states_1[4];
stai_ptr data_states[] = {
    states_1
};



/* Entry points --------------------------------------------------------------*/


void STM32CubeAI_Studio_AI_Init(void)
{

  MX_UARTx_Init();

    aiValidationInit();
    /* USER CODE BEGIN 5 */
    /* USER CODE END 5 */
}

void STM32CubeAI_Studio_AI_Process(void)
{
    aiValidationProcess();
} 

void STM32CubeAI_Studio_AI_Deinit(void)
{
} 

/* Multiple network support --------------------------------------------------*/

#include <string.h>

extern const stai_network_details g_network_details;
uintptr_t _weights_addr[STAI_NETWORK_WEIGHTS_NUM + 1];


static const stai_network_entry_t networks[STAI_MNETWORK_NUMBER] = {
    {
        .name = (const char *)STAI_NETWORK_MODEL_NAME,
        .get_context_size = stai_network_get_context_size,
        .init = stai_network_init,
        .deinit = stai_network_deinit,
        .run = stai_network_run,
        .get_info = stai_network_get_info,
        .get_error = stai_network_get_error,

        .get_activations = stai_network_get_activations,
        .get_states = stai_network_get_states,
        .get_weights = stai_network_get_weights,
        .get_inputs = stai_network_get_inputs,
        .get_outputs = stai_network_get_outputs,

        .set_activations = stai_network_set_activations,
        .set_states = stai_network_set_states,
        .set_weights = stai_network_set_weights,
        .set_inputs = stai_network_set_inputs,
        .set_outputs = stai_network_set_outputs,
        .set_callback = stai_network_set_callback,

        .context = network_context,

        .n_inputs = STAI_NETWORK_IN_NUM,
        .n_outputs = STAI_NETWORK_OUT_NUM,

        .n_activations = STAI_NETWORK_ACTIVATIONS_NUM,
        .n_states = STAI_NETWORK_STATES_NUM,
        .n_weights = STAI_NETWORK_WEIGHTS_NUM,
        .weights_addr = _weights_addr,

        .details = &g_network_details,
    },
};

struct network_instance {
     const stai_network_entry_t *entry;
     stai_ptr handle;
};

/* Number of instance is aligned on the number of network */
static struct network_instance gnetworks[STAI_MNETWORK_NUMBER] = {0};

static ai_bool _stai_mnetwork_is_valid(const char* name,
    const stai_network_entry_t *entry)
{
  if (name && (strlen(entry->name) == strlen(name)) &&
      (strncmp(entry->name, name, strlen(entry->name)) == 0))
    return true;
  return false;
}

static struct network_instance *_stai_mnetwork_handle(struct network_instance *inst)
{
  for (int i=0; i<STAI_MNETWORK_NUMBER; i++) {
    if ((inst) && (&gnetworks[i] == inst))
      return inst;
    else if ((!inst) && (gnetworks[i].entry == NULL))
      return &gnetworks[i];
  }
  return NULL;
}

static void _stai_mnetwork_release_handle(struct network_instance *inst)
{
  for (int i=0; i<STAI_MNETWORK_NUMBER; i++) {
    if ((inst) && (&gnetworks[i] == inst)) {
      gnetworks[i].entry = NULL;
      return;
    }
  }
}

STAI_API_ENTRY
const char* stai_mnetwork_find(const char *name, int idx)
{
  const stai_network_entry_t *entry;

  for (int i=0; i<STAI_MNETWORK_NUMBER; i++) {
    entry = &networks[i];
    if (_stai_mnetwork_is_valid(name, entry))
      return entry->name;
    else {
      if (!idx--)
        return entry->name;
    }
  }
  return NULL;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_init(const char *name, stai_ptr* network)
{
  const stai_network_entry_t *entry;
  const stai_network_entry_t *found = NULL;
  stai_return_code stai_err;

  struct network_instance *inst = _stai_mnetwork_handle(NULL);

  if (!inst || !network) {
    return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;
  }

  *network = NULL;

  for (int i=0; i<STAI_MNETWORK_NUMBER; i++) {
    entry = &networks[i];
    if (_stai_mnetwork_is_valid(name, entry)) {
      found = entry;
      break;
    }
  }

  if (!found) {
    return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;
  }

  stai_err = found->init(found->context);
  if (stai_err == STAI_SUCCESS) {
    stai_size tmp_;
    inst->entry = found;
    inst->handle = found->context;
    found->get_weights(found->context, (stai_ptr *)found->weights_addr, &tmp_);
    *network = (stai_ptr)inst;
    return STAI_SUCCESS;
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_activations(stai_ptr network, const stai_ptr* activations)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn =  _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_err = inn->entry->set_activations(inn->handle, activations, inn->entry->n_activations);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_states(stai_ptr network, const stai_ptr* states)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn =  _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_err = inn->entry->set_states(inn->handle, states, inn->entry->n_states);
  }
  return stai_err;
}


STAI_API_ENTRY
stai_return_code stai_mnetwork_set_weights(stai_ptr network, const stai_ptr* weights)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn =  _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_size tmp_;
    stai_err = inn->entry->set_weights(inn->handle, weights, inn->entry->n_weights);
    inn->entry->get_weights(inn->handle, (stai_ptr *)inn->entry->weights_addr, &tmp_);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_inputs(stai_ptr network, const stai_ptr* inputs)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_err = inn->entry->set_inputs(inn->handle, inputs, inn->entry->n_inputs);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_outputs(stai_ptr network, const stai_ptr* outputs)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_err = inn->entry->set_outputs(inn->handle, outputs, inn->entry->n_outputs);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_set_callback(stai_ptr network, const stai_event_cb cb, void* cb_cookie)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_err = inn->entry->set_callback(inn->handle, cb, cb_cookie);
  }
  return stai_err;
}

STAI_API_ENTRY
const stai_network_details *stai_mnetwork_get_details(stai_ptr network)
{
  struct network_instance *inn;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    return inn->entry->details;
  }
  return NULL;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_activations(stai_ptr network, stai_ptr* activations)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_size tmp_;
    stai_err = inn->entry->get_activations(inn->handle, activations, &tmp_);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_states(stai_ptr network, stai_ptr* states)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_size tmp_;
    stai_err = inn->entry->get_states(inn->handle, states, &tmp_);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_weights(stai_ptr network, stai_ptr* weights)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_size tmp_;
    stai_err = inn->entry->get_weights(inn->handle, weights, &tmp_);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_ptr *stai_mnetwork_get_weights_ext(stai_ptr network)
{
  struct network_instance *inn;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    return (stai_ptr *)inn->entry->weights_addr;
  }
  return NULL;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_inputs(stai_ptr network, stai_ptr* inputs)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_size tmp_;
    stai_err = inn->entry->get_inputs(inn->handle, inputs, &tmp_);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_outputs(stai_ptr network, stai_ptr* outputs)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_size tmp_;
    stai_err = inn->entry->get_outputs(inn->handle, outputs, &tmp_);
  }
  return stai_err;
}


STAI_API_ENTRY
stai_return_code stai_mnetwork_deinit(stai_ptr network)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    stai_err = inn->entry->deinit(inn->handle);
    if (stai_err == STAI_SUCCESS) {
      _stai_mnetwork_release_handle(inn);
    }
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_info(stai_ptr network, stai_network_info* report)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    return inn->entry->get_info(inn->handle, report);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_get_error(stai_ptr network)
{
  struct network_instance *inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    return inn->entry->get_error(inn->handle);
  }
  return stai_err;
}

STAI_API_ENTRY
stai_return_code stai_mnetwork_run(stai_ptr network)
{
  struct network_instance* inn;
  stai_return_code stai_err = STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE;

  inn = _stai_mnetwork_handle((struct network_instance *)network);
  if (inn) {
    return inn->entry->run(inn->handle, STAI_MODE_SYNC);
  }
  return stai_err;
}
      


#ifdef __cplusplus
}
#endif
