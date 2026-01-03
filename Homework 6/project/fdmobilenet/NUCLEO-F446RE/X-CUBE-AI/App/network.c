/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2025-12-31T12:36:00+0000
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#include "ai_lite_inspect.h"
#include "ai_platform_interface.h"
#include "layers.h"
#include "core_convert.h"
#include "network.h"
#include "network_details.h"
#include "network_data.h"
#include "stai_events.h"

#include "lite_operators.h"

#include "ai_lite_inspect.h"
/*****************************************************************************/
#define STAI_INTERNAL_API_MAJOR               (1)
#define STAI_INTERNAL_API_MINOR               (0)
#define STAI_INTERNAL_API_MICRO               (0)

#define STAI_MAGIC                            (0xB1C00100)

/*****************************************************************************/
#define _STAI_CONCAT_ARG(a, b)     a ## b
#define STAI_CONCAT(a, b)         _STAI_CONCAT_ARG(a, b)

/*!  STAI_CAST SECTION                       *********************************/
#define STAI_CAST(type, expr) \
  ((type)(expr))


/*****************************************************************************/
#define STAI_SIZE(_size) \
  ((stai_size)(_size))

/*****************************************************************************/
#define STAI_INIT_BUFFER(_flags, _size, _address) \
  { \
    .size = (_size), \
    .address = (uintptr_t)(_address), \
    .flags = (_flags), \
  }

#define STAI_INIT_TENSOR(_name, _flags, _fmt, _size_bytes, _shape, _scale, _zeropoint) \
  { \
    .size_bytes = (_size_bytes), \
    .flags = (_flags), \
    .format = (stai_format)(_fmt), \
    .shape = STAI_PACK(_shape), \
    .scale = STAI_PACK(_scale), \
    .zeropoint = STAI_PACK(_zeropoint), \
    .name = (_name) \
  }

#define STAI_INIT_ARRAY(_size, _ptr) \
  { .size = STAI_SIZE(_size), .data = STAI_PACK(_ptr) }


#define STAI_CAST_ARRAY(_type, _size, _ptr) \
  { .size = STAI_SIZE(_size), .data = (_type)STAI_PACK(_ptr) }


#define STAI_DECLARE_ARRAY(_type, _size, ...) \
  { .size = STAI_SIZE(_size), .data = (_type[_size]) { STAI_PACK(__VA_ARGS__) } }


#define STAI_EMPTY_ARRAY() \
  { .size = 0, .data = NULL }


#define STAI_INIT_VERSION(_major, _minor, _micro) \
  { .major = (_major), .minor = (_minor), .micro = (_micro), .reserved = 0x0 }

/*****************************************************************************/
/**  Getters and setters  **/

#define STAI_GET_ARRAY_SIZE(nd_array) \
  (nd_array.size)


#define STAI_GET_ARRAY_ELEM(nd_array, pos) \
  (nd_array.data[(pos)])

#define _STAI_SET_ERROR(net_ctx, cond, value, exit) { \
  if (!(net_ctx)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE; } \
  if (((uintptr_t)net_ctx) & (_STAI_CONTEXT_ALIGNMENT-1)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_ALIGNMENT; } \
  if (((value) >= STAI_ERROR_GENERIC) && (cond)) { \
    if ((net_ctx)->_return_code == STAI_SUCCESS) { \
      (net_ctx)->_return_code = (value); \
    } \
    return (exit); \
  } \
}

/*****************************************************************************/
/* TODO REMOVE THESE TWO MACROS */
#define STAI_EVENT_NODE_START_CB
#define STAI_EVENT_NODE_STOP_CB

#ifdef STAI_EVENT_NODE_START_CB
#ifndef _STAI_NETWORK_EVENT_NODE_START_CB
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _start_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(const stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_START, (const void*)&_start_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_START_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_START_CB */

#ifdef STAI_EVENT_NODE_STOP_CB
#ifndef _STAI_NETWORK_EVENT_NODE_STOP_CB
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _stop_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_STOP, (const void*)&_stop_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_STOP_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_STOP_CB */


/*****************************************************************************/
#define _STAI_NETWORK_MODEL_SIGNATURE     "0x59b1c5c61b603cd5f08ad657424ea9bc"
#define _STAI_NETWORK_DATETIME            "2025-12-31T12:36:00+0000"
#define _STAI_NETWORK_COMPILE_DATETIME    __DATE__ " " __TIME__

#define _STAI_CONTEXT_ALIGNMENT        STAI_NETWORK_CONTEXT_ALIGNMENT

/*****************************************************************************/
#define g_network_activations_1     (NULL)




#if defined(HAVE_NETWORK_INFO)
/*****************************************************************************/
static const stai_network_info g_network_info = {
  .model_signature = _STAI_NETWORK_MODEL_SIGNATURE,
  .c_compile_datetime = _STAI_NETWORK_COMPILE_DATETIME,
  .c_model_name = STAI_NETWORK_MODEL_NAME,
  .c_model_datetime = _STAI_NETWORK_DATETIME,
  .c_model_signature = 0x0,
  .runtime_version = STAI_INIT_VERSION(11, 0, 0),
  .tool_version = STAI_INIT_VERSION(3, 0, 0),
  .api_version = STAI_INIT_VERSION(1, 0, 0),
  .n_macc = STAI_NETWORK_MACC_NUM,
  .n_nodes = STAI_NETWORK_NODES_NUM,
  .flags = STAI_NETWORK_FLAGS,
  .n_inputs = STAI_NETWORK_IN_NUM,
  .n_outputs = STAI_NETWORK_OUT_NUM,
  .n_activations = STAI_NETWORK_ACTIVATIONS_NUM,
  .n_weights = STAI_NETWORK_WEIGHTS_NUM,
  .n_states = STAI_NETWORK_STATES_NUM,
  .inputs = (stai_tensor[STAI_NETWORK_IN_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_IN_1_NAME,
      STAI_NETWORK_IN_1_FLAGS,
      STAI_NETWORK_IN_1_FORMAT,
      STAI_NETWORK_IN_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 32, 32, 3),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
    .outputs = (stai_tensor[STAI_NETWORK_OUT_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_1_NAME,
      STAI_NETWORK_OUT_1_FLAGS,
      STAI_NETWORK_OUT_1_FORMAT,
      STAI_NETWORK_OUT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 2, 1, 10),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .activations = (stai_tensor[STAI_NETWORK_ACTIVATIONS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_ACTIVATION_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_ACTIVATION_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 12288),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .weights = (stai_tensor[STAI_NETWORK_WEIGHTS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 216),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_2_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_2_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 32),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_3_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_3_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 72),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_4_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_4_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 32),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_5_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_5_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_6_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_6_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_7_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_7_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 144),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_8_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_8_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_9_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_9_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_10_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_10_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_11_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_11_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 288),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_12_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_12_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_13_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_13_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1024),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_14_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_14_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_15_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_15_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 288),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_16_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_16_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 128),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_17_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_17_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2048),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_18_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_18_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_19_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_19_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_20_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_20_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_21_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_21_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 4096),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_22_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_22_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_23_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_23_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_24_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_24_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_25_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_25_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 8192),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_26_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_26_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_27_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_27_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_28_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_28_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_29_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_29_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 16384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_30_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_30_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_31_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_31_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_32_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_32_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_33_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_33_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 16384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_34_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_34_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_35_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_35_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_36_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_36_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_37_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_37_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 16384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_38_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_38_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_39_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_39_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_40_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_40_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_41_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_41_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 16384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_42_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_42_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_43_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_43_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_44_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_44_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_45_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_45_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 32768),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_46_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_46_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1024),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_47_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_47_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2560),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_48_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_48_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 40),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },

  .states = NULL
};
#endif

#define _STAI_CONTEXT_ACQUIRE(_net_ctx, _net_handle) \
  _stai_network_context* _net_ctx = (_stai_network_context*)(_net_handle); \
  STAI_ASSERT(_net_ctx != NULL) \
  _STAI_SET_ERROR(_net_ctx, _net_ctx->_magic != STAI_MAGIC, \
                  STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE, _net_ctx->_return_code)


/*****************************************************************************/
static
void _stai_network_check(_stai_network_context* net_ctx)
{
  stai_size idx;

// Check activations status
  for (idx=0; idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    if (net_ctx->_activations[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_ACTIVATIONS_NUM) ? STAI_FLAG_ACTIVATIONS : STAI_FLAG_NONE;
// Check inputs status
  for (idx=0; idx<STAI_NETWORK_IN_NUM; idx++) {
    if (net_ctx->_inputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_IN_NUM) ? STAI_FLAG_INPUTS : STAI_FLAG_NONE;

  // Check outputs status
  for (idx=0; idx<STAI_NETWORK_OUT_NUM; idx++) {
    if (net_ctx->_outputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_OUT_NUM) ? STAI_FLAG_OUTPUTS : STAI_FLAG_NONE;

// Check weights status
  for (idx=0; idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    if (net_ctx->_weights[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_WEIGHTS_NUM) ? STAI_FLAG_WEIGHTS : STAI_FLAG_NONE;
STAI_PRINT("  [_stai_network_check] flags: 0x%08x\n", net_ctx->_flags)
}


/*****************************************************************************/
STAI_API_ENTRY
stai_return_code stai_network_init(
  stai_network* network)
{
  /* Memory where to store internal context is provided by applications as a raw byte buffer */
  _stai_network_context* net_ctx = (_stai_network_context*)(network);
  net_ctx->_return_code = STAI_SUCCESS;
  STAI_PRINT("[Entering Network Init] network(%p) context_size(%d)\n", net_ctx, (int32_t)sizeof(_stai_network_context))

  _STAI_SET_ERROR(net_ctx, STAI_NETWORK_CONTEXT_SIZE != sizeof(_stai_network_context),
                 STAI_ERROR_NETWORK_INVALID_CONTEXT_SIZE, net_ctx->_return_code)

  {
    const _stai_network_context _network_context = {
      ._magic = STAI_MAGIC,
      ._signature = STAI_NETWORK_MODEL_SIGNATURE,
      ._flags = STAI_NETWORK_FLAGS,
      ._return_code = STAI_SUCCESS,
      ._callback = NULL,
      ._callback_cookie = NULL,
      ._activations = {
      (stai_ptr)g_network_activations_1
      },
      ._weights = {
      (stai_ptr)g_network_conv2d_1_weights_array,(stai_ptr)g_network_conv2d_1_bias_array,(stai_ptr)g_network_conv2d_2_weights_array,(stai_ptr)g_network_conv2d_2_bias_array,(stai_ptr)g_network_conv2d_3_weights_array,(stai_ptr)g_network_conv2d_3_bias_array,(stai_ptr)g_network_conv2d_4_weights_array,(stai_ptr)g_network_conv2d_4_bias_array,(stai_ptr)g_network_conv2d_5_weights_array,(stai_ptr)g_network_conv2d_5_bias_array,(stai_ptr)g_network_conv2d_6_weights_array,(stai_ptr)g_network_conv2d_6_bias_array,(stai_ptr)g_network_conv2d_7_weights_array,(stai_ptr)g_network_conv2d_7_bias_array,(stai_ptr)g_network_conv2d_8_weights_array,(stai_ptr)g_network_conv2d_8_bias_array,(stai_ptr)g_network_conv2d_9_weights_array,(stai_ptr)g_network_conv2d_9_bias_array,(stai_ptr)g_network_conv2d_10_weights_array,(stai_ptr)g_network_conv2d_10_bias_array,(stai_ptr)g_network_conv2d_11_weights_array,(stai_ptr)g_network_conv2d_11_bias_array,(stai_ptr)g_network_conv2d_12_weights_array,(stai_ptr)g_network_conv2d_12_bias_array,(stai_ptr)g_network_conv2d_13_weights_array,(stai_ptr)g_network_conv2d_13_bias_array,(stai_ptr)g_network_conv2d_14_weights_array,(stai_ptr)g_network_conv2d_14_bias_array,(stai_ptr)g_network_conv2d_15_weights_array,(stai_ptr)g_network_conv2d_15_bias_array,(stai_ptr)g_network_conv2d_16_weights_array,(stai_ptr)g_network_conv2d_16_bias_array,(stai_ptr)g_network_conv2d_17_weights_array,(stai_ptr)g_network_conv2d_17_bias_array,(stai_ptr)g_network_conv2d_18_weights_array,(stai_ptr)g_network_conv2d_18_bias_array,(stai_ptr)g_network_conv2d_19_weights_array,(stai_ptr)g_network_conv2d_19_bias_array,(stai_ptr)g_network_conv2d_20_weights_array,(stai_ptr)g_network_conv2d_20_bias_array,(stai_ptr)g_network_conv2d_21_weights_array,(stai_ptr)g_network_conv2d_21_bias_array,(stai_ptr)g_network_conv2d_22_weights_array,(stai_ptr)g_network_conv2d_22_bias_array,(stai_ptr)g_network_conv2d_23_weights_array,(stai_ptr)g_network_conv2d_23_bias_array,(stai_ptr)g_network_conv2d_29_weights_array,(stai_ptr)g_network_conv2d_29_bias_array
      },
      ._inputs = {
    NULL},
      ._outputs = {
    NULL},
    };

    // Deep copy of internal context to opaque buffer provided by app
    *net_ctx = _network_context;

    _stai_network_check(net_ctx);
  }

  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_deinit(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /*  Reset flags to initial state  */
  net_ctx->_flags = STAI_NETWORK_FLAGS;
  return net_ctx->_return_code;
}

/*****************************************************************************/



/* Int quant #0 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_22_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.005966615863144398f),
    AI_PACK_INTQ_ZP(65)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_23_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.01855890266597271f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_23_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 256,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.014990835450589657f, 0.01024994533509016f, 0.018636351451277733f, 0.0151712317019701f, 0.016051067039370537f, 0.015184778720140457f, 0.01378614827990532f, 0.01222709845751524f, 0.015927499160170555f, 0.012027157470583916f, 0.016349881887435913f, 0.012447218410670757f, 0.013255966827273369f, 0.012599872425198555f, 0.014027204364538193f, 0.013788185082376003f, 0.017582962289452553f, 0.012497230432927608f, 0.012910966761410236f, 0.015371744520962238f, 0.00955598708242178f, 0.01664004474878311f, 0.020074918866157532f, 0.014215092174708843f, 0.015874791890382767f, 0.014503476209938526f, 0.012069419026374817f, 0.017266057431697845f, 0.0160396508872509f, 0.01663019135594368f, 0.02007152885198593f, 0.011228149756789207f, 0.015868021175265312f, 0.014458724297583103f, 0.010487054474651814f, 0.014334683306515217f, 0.010853609070181847f, 0.010552586987614632f, 0.017907418310642242f, 0.01585248112678528f, 0.012675019912421703f, 0.013685381971299648f, 0.015492652542889118f, 0.014415054582059383f, 0.014550269581377506f, 0.013819308020174503f, 0.011303931474685669f, 0.014486701227724552f, 0.013829147443175316f, 0.018480446189641953f, 0.01345074363052845f, 0.016880981624126434f, 0.015627868473529816f, 0.017863096669316292f, 0.014386550523340702f, 0.01491195522248745f, 0.013014482334256172f, 0.016144001856446266f, 0.01628842204809189f, 0.01487642154097557f, 0.01544485054910183f, 0.014423181302845478f, 0.014459420926868916f, 0.015989599749445915f, 0.014455089345574379f, 0.014928865246474743f, 0.013950304128229618f, 0.012439260259270668f, 0.014122671447694302f, 0.016689427196979523f, 0.012092712335288525f, 0.015189297497272491f, 0.011936320923268795f, 0.01637030951678753f, 0.014886898919939995f, 0.014205927960574627f, 0.015238048508763313f, 0.015137122943997383f, 0.015531505458056927f, 0.01694699190557003f, 0.015935048460960388f, 0.015815436840057373f, 0.015405820682644844f, 0.015571125783026218f, 0.015511667355895042f, 0.015036381781101227f, 0.016573762521147728f, 0.016061536967754364f, 0.016417980194091797f, 0.01749519817531109f, 0.019179122522473335f, 0.014149130322039127f, 0.020707683637738228f, 0.014683229848742485f, 0.011527901515364647f, 0.016011493280529976f, 0.014967633411288261f, 0.01892661303281784f, 0.014744951389729977f, 0.014947074465453625f, 0.017813779413700104f, 0.015908248722553253f, 0.013608467765152454f, 0.01782044768333435f, 0.015239791013300419f, 0.014820977114140987f, 0.016393817961215973f, 0.015787942335009575f, 0.018182389438152313f, 0.014684563502669334f, 0.014043753035366535f, 0.014266673475503922f, 0.017418304458260536f, 0.013965938240289688f, 0.011981667019426823f, 0.016697950661182404f, 0.014145943336188793f, 0.015267929993569851f, 0.02101551927626133f, 0.013723345473408699f, 0.013848438858985901f, 0.021379655227065086f, 0.010073209181427956f, 0.013521517626941204f, 0.01880393549799919f, 0.011915227398276329f, 0.014408267103135586f, 0.013327473774552345f, 0.019730811938643456f, 0.01166187971830368f, 0.013554652221500874f, 0.018011977896094322f, 0.011190413497388363f, 0.013513355515897274f, 0.01686929538846016f, 0.012889419682323933f, 0.012468128465116024f, 0.01829216443002224f, 0.01886769011616707f, 0.014082657173275948f, 0.015118295326828957f, 0.014695124700665474f, 0.016559230163693428f, 0.014018183574080467f, 0.016614479944109917f, 0.01320947241038084f, 0.013172825798392296f, 0.01376319583505392f, 0.017756592482328415f, 0.011094129644334316f, 0.014479582197964191f, 0.019162869080901146f, 0.013781125657260418f, 0.015957171097397804f, 0.019518164917826653f, 0.0156265739351511f, 0.013734125532209873f, 0.015839997678995132f, 0.011741045862436295f, 0.01306696143001318f, 0.01286157127469778f, 0.016830848529934883f, 0.014405866153538227f, 0.012982557527720928f, 0.011752067133784294f, 0.010952743701636791f, 0.017523162066936493f, 0.015736432746052742f, 0.017028547823429108f, 0.014971856959164143f, 0.014855870045721531f, 0.01613019034266472f, 0.016348138451576233f, 0.015672283247113228f, 0.012785892002284527f, 0.013941848650574684f, 0.01859341189265251f, 0.014218040741980076f, 0.015326370485126972f, 0.010676391422748566f, 0.013143975287675858f, 0.015491635538637638f, 0.01569806970655918f, 0.013964341022074223f, 0.0164065919816494f, 0.014952244237065315f, 0.020584164187312126f, 0.014818319119513035f, 0.015268283896148205f, 0.015817314386367798f, 0.018264854326844215f, 0.015273264609277248f, 0.018857967108488083f, 0.014496663585305214f, 0.013425049372017384f, 0.014274382963776588f, 0.013196452520787716f, 0.013660884462296963f, 0.012225001119077206f, 0.013622410595417023f, 0.014150617644190788f, 0.01620580069720745f, 0.011279099620878696f, 0.014942033216357231f, 0.012572243809700012f, 0.014541344717144966f, 0.015629317611455917f, 0.01610504649579525f, 0.013407417573034763f, 0.01519996952265501f, 0.012266376987099648f, 0.014300215989351273f, 0.013344645500183105f, 0.015518523752689362f, 0.014116286300122738f, 0.019173067063093185f, 0.014623765833675861f, 0.017060795798897743f, 0.014954205602407455f, 0.022676432505249977f, 0.017955850809812546f, 0.019140541553497314f, 0.013806505128741264f, 0.013575946912169456f, 0.012894051149487495f, 0.01193339005112648f, 0.010576872155070305f, 0.013840068131685257f, 0.013990811072289944f, 0.017861351370811462f, 0.01435533445328474f, 0.01144996378570795f, 0.015101339668035507f, 0.022838251665234566f, 0.015925375744700432f, 0.014062080532312393f, 0.01335102878510952f, 0.012233157642185688f, 0.016287120059132576f, 0.01813896745443344f, 0.018283529207110405f, 0.015374673530459404f, 0.012230431661009789f, 0.013121780008077621f, 0.017833707854151726f, 0.010746169835329056f, 0.013877677731215954f, 0.01286353450268507f, 0.012179029174149036f, 0.01545694563537836f, 0.015089106746017933f, 0.011721691116690636f, 0.013788275420665741f, 0.012502763420343399f, 0.01683986745774746f, 0.013863030821084976f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_23_scratch1_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.01855890266597271f),
    AI_PACK_INTQ_ZP(-128)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_22_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 128, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_23_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 256, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_23_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 32768, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_23_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 256, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_23_scratch0_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3072, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_23_scratch1_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 256, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_22_output, AI_STATIC,
  53, 0x1,
  AI_SHAPE_INIT(4, 1, 128, 1, 1), AI_STRIDE_INIT(4, 1, 1, 128, 128),
  1, &conv2d_22_output_array, &conv2d_22_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_23_bias, AI_STATIC,
  56, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 1), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &conv2d_23_bias_array, NULL)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_23_output, AI_STATIC,
  57, 0x1,
  AI_SHAPE_INIT(4, 1, 256, 1, 1), AI_STRIDE_INIT(4, 1, 1, 256, 256),
  1, &conv2d_23_output_array, &conv2d_23_output_array_intq)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_23_scratch0, AI_STATIC,
  58, 0x0,
  AI_SHAPE_INIT(4, 1, 3072, 1, 1), AI_STRIDE_INIT(4, 1, 1, 3072, 3072),
  1, &conv2d_23_scratch0_array, NULL)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_23_scratch1, AI_STATIC,
  59, 0x1,
  AI_SHAPE_INIT(4, 1, 256, 1, 1), AI_STRIDE_INIT(4, 1, 1, 256, 256),
  1, &conv2d_23_scratch1_array, &conv2d_23_scratch1_array_intq)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_23_weights, AI_STATIC,
  60, 0x1,
  AI_SHAPE_INIT(4, 128, 1, 1, 256), AI_STRIDE_INIT(4, 1, 128, 32768, 32768),
  1, &conv2d_23_weights_array, &conv2d_23_weights_array_intq)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  conv2d_23_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_22_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_23_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &conv2d_23_weights, &conv2d_23_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_23_scratch0, &conv2d_23_scratch1)
)

AI_LAYER_OBJ_DECLARE(
  conv2d_23_layer, 24,
  OPTIMIZED_CONV2D_TYPE, 0x0, NULL,
  conv2d_nl_pool, forward_pw_sssa8_ch_nl_pool,
  &conv2d_23_chain,
  NULL, &conv2d_23_layer, AI_STATIC, 
  .groups = 1, 
  .filter_stride = AI_SHAPE_2D_INIT(1, 1), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
  .pool_size = AI_SHAPE_2D_INIT(1, 1), 
  .pool_stride = AI_SHAPE_2D_INIT(1, 1), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
  .pool_func = AI_HANDLE_PTR(pool_func_ap_array_integer_INT8), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_conv2d_23(_stai_network_context* net_ctx)
{
  conv2d_22_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_22_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_23_weights_array.data = AI_PTR(net_ctx->_weights[44] + 0);
  conv2d_23_weights_array.data_start = AI_PTR(net_ctx->_weights[44] + 0);
  conv2d_23_bias_array.data = AI_PTR(net_ctx->_weights[45] + 0);
  conv2d_23_bias_array.data_start = AI_PTR(net_ctx->_weights[45] + 0);
  conv2d_23_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  conv2d_23_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  conv2d_23_scratch1_array.data = AI_PTR(net_ctx->_activations[0] + 3200);
  conv2d_23_scratch1_array.data_start = AI_PTR(net_ctx->_activations[0] + 3200);
  conv2d_23_output_array.data = AI_PTR(net_ctx->_activations[0] + 3456);
  conv2d_23_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 3456);
  _STAI_NETWORK_EVENT_NODE_START_CB(24, 1, { conv2d_22_output.data->data});
  forward_pw_sssa8_ch_nl_pool(&conv2d_23_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(24, 1, { conv2d_23_output.data->data});
}

/*****************************************************************************/


static const ai_u32 conversion_0_t_out_0_shape_h_w_ch_d_prod_const_u32 = 3072;
static const ai_float conversion_0_t_out_0_fmt_scale_const_f32 = 0.003921568859368563f;
static const ai_i8 conversion_0_t_out_0_fmt_zero_const_s8 = -128;

static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16 = 32;
static const ai_u16 conv2d_1_t_out_0_shape_ch_const_u16 = 8;
static const ai_u16 conv2d_1_t_weight_0_shape_w_const_u16 = 3;
static const ai_i32 conv2d_1_l_pad_W_0_const_s32 = 0;
static const ai_u16 conv2d_1_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_1_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_1_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32 = 0.003921568859368563f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.01810968667268753f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004504018928855658f, 0.007424172479659319f, 0.005063188262283802f, 0.004945645108819008f, 0.006680886261165142f, 0.008502102456986904f, 0.004191288724541664f, 0.004406883381307125f);
static const ai_layer_format_type conv2d_1_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 16;

static const ai_u16 conv2d_2_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_2_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_2_t_in_0_shape_ch_const_u16 = 8;
static const ai_u16 conv2d_2_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_2_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_2_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_2_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_2_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_2_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_2_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_2_t_out_0_fmt_zero_const_s8 = -10;
static const ai_float conv2d_2_t_in_0_fmt_scale_const_f32 = 0.01810968667268753f;
static const ai_float conv2d_2_t_out_0_fmt_scale_const_f32 = 0.030066171661019325f;
static const ai_float conv2d_2_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0033551673404872417f, 0.0027051325887441635f, 0.00255069462582469f, 0.002350460272282362f, 0.0027236801106482744f, 0.002850413555279374f, 0.0017146453028544784f, 0.002246363088488579f);
static const ai_u16 conv2d_2_t_out_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_2_t_out_0_shape_h_const_u16 = 8;

static const ai_u16 conv2d_3_t_in_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_3_t_in_0_shape_h_const_u16 = 8;
static const ai_u16 conv2d_3_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_3_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_3_t_in_0_shape_ch_const_u16 = 8;
static const ai_u16 conv2d_3_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_3_t_in_0_fmt_zero_const_s8 = -10;
static const ai_i8 conv2d_3_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_3_t_in_0_fmt_scale_const_f32 = 0.030066171661019325f;
static const ai_float conv2d_3_t_out_0_fmt_scale_const_f32 = 0.020546862855553627f;
static const ai_float conv2d_3_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.005698780994862318f, 0.006954965181648731f, 0.010312255471944809f, 0.007279993500560522f, 0.0070707290433347225f, 0.009315356612205505f, 0.005874354857951403f, 0.0047551365569233894f, 0.008092276751995087f, 0.008071511052548885f, 0.007041578181087971f, 0.0056004757061600685f, 0.007266441825777292f, 0.023725600913167f, 0.004116641357541084f, 0.012639375403523445f);
static const ai_layer_format_type conv2d_3_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_4_t_in_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_4_t_in_0_shape_h_const_u16 = 8;
static const ai_u16 conv2d_4_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_4_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_4_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_4_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_4_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_4_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_4_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_4_t_out_0_fmt_zero_const_s8 = 13;
static const ai_float conv2d_4_t_in_0_fmt_scale_const_f32 = 0.020546862855553627f;
static const ai_float conv2d_4_t_out_0_fmt_scale_const_f32 = 0.018508896231651306f;
static const ai_float conv2d_4_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002300923690199852f, 0.0026854544412344694f, 0.0022804532200098038f, 0.0023683207109570503f, 0.0025202783290296793f, 0.002191829029470682f, 0.0019397924188524485f, 0.001971626188606024f, 0.0026704876217991114f, 0.00319315935485065f, 0.0018837982788681984f, 0.0018704727990552783f, 0.0024780835956335068f, 0.0014871439198032022f, 0.0024907286278903484f, 0.0022493049036711454f);
static const ai_u16 conv2d_4_t_out_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_4_t_out_0_shape_h_const_u16 = 4;

static const ai_u16 conv2d_5_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_5_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_5_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_5_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_5_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_5_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_5_t_in_0_fmt_zero_const_s8 = 13;
static const ai_i8 conv2d_5_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_5_t_in_0_fmt_scale_const_f32 = 0.018508896231651306f;
static const ai_float conv2d_5_t_out_0_fmt_scale_const_f32 = 0.02012551948428154f;
static const ai_float conv2d_5_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.01295680645853281f, 0.012348553165793419f, 0.01529544685035944f, 0.008419510908424854f, 0.007869224064052105f, 0.010129210539162159f, 0.009675043635070324f, 0.009160509333014488f, 0.008811542764306068f, 0.014762012287974358f, 0.010212559252977371f, 0.010278729721903801f, 0.009016635827720165f, 0.007680035661906004f, 0.009031569585204124f, 0.010497733019292355f, 0.009798825718462467f, 0.008206959813833237f, 0.013308076187968254f, 0.011084155179560184f, 0.009129468351602554f, 0.006540919188410044f, 0.010008322075009346f, 0.009757302701473236f, 0.011258164420723915f, 0.011460049077868462f, 0.009206930175423622f, 0.0087786465883255f, 0.010695635341107845f, 0.007117561064660549f, 0.013250465504825115f, 0.0095711974427104f);
static const ai_layer_format_type conv2d_5_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_6_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_6_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_6_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_6_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_6_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_6_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_6_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_6_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_6_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_6_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_6_t_out_0_fmt_zero_const_s8 = 6;
static const ai_float conv2d_6_t_in_0_fmt_scale_const_f32 = 0.02012551948428154f;
static const ai_float conv2d_6_t_out_0_fmt_scale_const_f32 = 0.01366101298481226f;
static const ai_float conv2d_6_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0014530671760439873f, 0.0019927341490983963f, 0.0021689075510948896f, 0.001579896779730916f, 0.002053492236882448f, 0.0019467496313154697f, 0.002217917237430811f, 0.0022374799009412527f, 0.0025535584427416325f, 0.0021883328445255756f, 0.0014505458530038595f, 0.002354872180148959f, 0.0019005839712917805f, 0.0015487911878153682f, 0.001996231498196721f, 0.002472394611686468f, 0.0019550416618585587f, 0.0016202720580622554f, 0.001639840891584754f, 0.0026881766971200705f, 0.0012192135909572244f, 0.0015401681885123253f, 0.0027906072791665792f, 0.00148631795309484f, 0.0017953391652554274f, 0.002846544375643134f, 0.002010050695389509f, 0.001708152354694903f, 0.0014980152482166886f, 0.001959854504093528f, 0.0020740299951285124f, 0.0018135039135813713f);
static const ai_u16 conv2d_6_t_out_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_6_t_out_0_shape_h_const_u16 = 4;

static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_7_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_7_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_7_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8 = 6;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32 = 0.01366101298481226f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32 = 0.018456758931279182f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009534615091979504f, 0.014037862420082092f, 0.012244820594787598f, 0.012830669991672039f, 0.010637541301548481f, 0.012155747041106224f, 0.009296800941228867f, 0.009823950938880444f, 0.008990971371531487f, 0.011278408579528332f, 0.01201591081917286f, 0.011941062286496162f, 0.010760331526398659f, 0.0096253901720047f, 0.012941364198923111f, 0.009195738472044468f, 0.012623008340597153f, 0.010628581047058105f, 0.01143188402056694f, 0.010658018290996552f, 0.01258420292288065f, 0.009399055503308773f, 0.010167510248720646f, 0.016673753038048744f, 0.008016376756131649f, 0.01043359562754631f, 0.01235164050012827f, 0.008830433711409569f, 0.008901230059564114f, 0.01303983386605978f, 0.011143969371914864f, 0.00920107401907444f);
static const ai_layer_format_type conv2d_7_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_8_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_8_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_8_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_8_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_8_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_8_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8 = -6;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32 = 0.018456758931279182f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32 = 0.013123159296810627f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0016206896398216486f, 0.001958482200279832f, 0.0020547916647046804f, 0.0019375409465283155f, 0.002021844033151865f, 0.0021846629679203033f, 0.0014352751895785332f, 0.0015079216100275517f, 0.0019028851529583335f, 0.0016326064942404628f, 0.0013923009391874075f, 0.0019275652011856437f, 0.0022153835743665695f, 0.0018851436907425523f, 0.002218266949057579f, 0.002164386212825775f, 0.002259802306070924f, 0.002214843640103936f, 0.001871192129328847f, 0.0018168814713135362f, 0.0017971316119655967f, 0.0018991834949702024f, 0.0015546568902209401f, 0.0018478228012099862f, 0.0017840171931311488f, 0.002401093253865838f, 0.0018500713631510735f, 0.0017417111666873097f, 0.0022196131758391857f, 0.0021330625750124454f, 0.001563705038279295f, 0.0019474561559036374f);
static const ai_u16 conv2d_8_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_8_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_9_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_9_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_9_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_9_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_9_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_9_t_in_0_fmt_zero_const_s8 = -6;
static const ai_i8 conv2d_9_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_9_t_in_0_fmt_scale_const_f32 = 0.013123159296810627f;
static const ai_float conv2d_9_t_out_0_fmt_scale_const_f32 = 0.019708655774593353f;
static const ai_float conv2d_9_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.010796836577355862f, 0.010427668690681458f, 0.015170902945101261f, 0.0093250572681427f, 0.009542888961732388f, 0.012060512788593769f, 0.01233555655926466f, 0.01270110160112381f, 0.012378744781017303f, 0.011765304952859879f, 0.012888874858617783f, 0.012570000253617764f, 0.01093673799186945f, 0.01254934910684824f, 0.011245151981711388f, 0.010820088908076286f, 0.015775436535477638f, 0.01383853331208229f, 0.009251796640455723f, 0.01325327530503273f, 0.008353069424629211f, 0.012353627011179924f, 0.015372983179986477f, 0.011893954128026962f, 0.01383181195706129f, 0.01270676776766777f, 0.01134486123919487f, 0.013682455755770206f, 0.019082671031355858f, 0.014792692847549915f, 0.013124740682542324f, 0.00994930136948824f, 0.009654083289206028f, 0.012979417107999325f, 0.011095537804067135f, 0.014259898103773594f, 0.01766277104616165f, 0.009114154614508152f, 0.015729175880551338f, 0.010617026127874851f, 0.012147719040513039f, 0.015663346275687218f, 0.010946881026029587f, 0.01101543940603733f, 0.011348260566592216f, 0.012065225280821323f, 0.011867511086165905f, 0.010090559720993042f, 0.012165986932814121f, 0.011081622913479805f, 0.014360405504703522f, 0.013392134569585323f, 0.01577124372124672f, 0.011193709447979927f, 0.013713051564991474f, 0.011435536667704582f, 0.011440531350672245f, 0.01303409319370985f, 0.014231677167117596f, 0.011850491166114807f, 0.008802944794297218f, 0.013815412297844887f, 0.01681351289153099f, 0.011813373304903507f);
static const ai_layer_format_type conv2d_9_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_10_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_10_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_10_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_10_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_10_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_10_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_10_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_10_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_10_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_10_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_10_t_out_0_fmt_zero_const_s8 = 11;
static const ai_float conv2d_10_t_in_0_fmt_scale_const_f32 = 0.019708655774593353f;
static const ai_float conv2d_10_t_out_0_fmt_scale_const_f32 = 0.01049538142979145f;
static const ai_float conv2d_10_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0016082240035757422f, 0.0018311719177290797f, 0.0017108980100601912f, 0.0014622969320043921f, 0.0018032685620710254f, 0.001290802494622767f, 0.0016770194051787257f, 0.0015719300135970116f, 0.0017153039807453752f, 0.0017097516683861613f, 0.00191043084487319f, 0.0014518441166728735f, 0.0016363040776923299f, 0.0023590619675815105f, 0.0015422789147123694f, 0.00149825238622725f, 0.0014707256341353059f, 0.001606002333573997f, 0.0014589340426027775f, 0.0019282056018710136f, 0.001617595087736845f, 0.0016602566465735435f, 0.0015343474224209785f, 0.001420661574229598f, 0.0015214784070849419f, 0.0016523582162335515f, 0.0015132577391341329f, 0.0014494245406240225f, 0.0017050508176907897f, 0.0020809888374060392f, 0.0017781099304556847f, 0.0023519969545304775f, 0.0010236313100904226f, 0.0012779004173353314f, 0.0016289723571389914f, 0.0019509053090587258f, 0.0014878481160849333f, 0.0019002191256731749f, 0.002395888091996312f, 0.0018507589120417833f, 0.0017670306842774153f, 0.001953697297722101f, 0.0016080697532743216f, 0.0013806003844365478f, 0.001873486558906734f, 0.0020055652130395174f, 0.001877736416645348f, 0.0016360412118956447f, 0.0021939834114164114f, 0.001284539233893156f, 0.002172130160033703f, 0.0018543479964137077f, 0.0024481844156980515f, 0.0015626122476533055f, 0.0014979884726926684f, 0.0011472958140075207f, 0.0019494788721203804f, 0.0013887544628232718f, 0.0017550945049151778f, 0.002204564632847905f, 0.0017266863724216819f, 0.0015732890460640192f, 0.0018406114540994167f, 0.0013008129317313433f);
static const ai_u16 conv2d_10_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_10_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_11_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_11_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_11_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = 11;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.01049538142979145f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.0200013630092144f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.020868005231022835f, 0.0129860145971179f, 0.011932413093745708f, 0.015239089727401733f, 0.012264450080692768f, 0.012102216482162476f, 0.01210401114076376f, 0.01148221641778946f, 0.012409145943820477f, 0.015465186908841133f, 0.013349905610084534f, 0.016070691868662834f, 0.01166206318885088f, 0.01212052907794714f, 0.016204267740249634f, 0.01156788319349289f, 0.012991946190595627f, 0.013406592421233654f, 0.014463881030678749f, 0.01074898336082697f, 0.012845899909734726f, 0.01416905876249075f, 0.013637288473546505f, 0.011881496757268906f, 0.012563854455947876f, 0.012651813216507435f, 0.015685202553868294f, 0.011704176664352417f, 0.013643678277730942f, 0.011420776136219501f, 0.012466621585190296f, 0.012615326792001724f, 0.01169615052640438f, 0.012327619828283787f, 0.01134987361729145f, 0.009493522346019745f, 0.010186673142015934f, 0.011179869063198566f, 0.01074750442057848f, 0.013408946804702282f, 0.01281800027936697f, 0.009786444716155529f, 0.01601465232670307f, 0.012610316276550293f, 0.011560888029634953f, 0.012494828552007675f, 0.013436108827590942f, 0.01411118358373642f, 0.015185905620455742f, 0.014446698129177094f, 0.010289018973708153f, 0.014782757498323917f, 0.014144869521260262f, 0.014447784051299095f, 0.011396913789212704f, 0.015325469896197319f, 0.01409859023988247f, 0.009362899698317051f, 0.014538503251969814f, 0.009595687501132488f, 0.014724714681506157f, 0.012366353534162045f, 0.012312340550124645f, 0.00919651985168457f);
static const ai_layer_format_type conv2d_11_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_12_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_12_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_12_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_12_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_12_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_12_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_12_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_12_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_12_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_12_t_out_0_fmt_zero_const_s8 = -2;
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.0200013630092144f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.008548066951334476f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001317560556344688f, 0.0013561784289777279f, 0.0014095117803663015f, 0.001250319997780025f, 0.0009235703037120402f, 0.0013774508843198419f, 0.0011384907411411405f, 0.000960403005592525f, 0.0014367104740813375f, 0.0013173932675272226f, 0.0014953463105484843f, 0.001506849774159491f, 0.0012420805869624019f, 0.0011764508672058582f, 0.0012759530218318105f, 0.0011247687507420778f, 0.0010624510468915105f, 0.0015139493625611067f, 0.0016534070018678904f, 0.0013613882474601269f, 0.0010090185096487403f, 0.0013665190199390054f, 0.0010994193144142628f, 0.0010109811555594206f, 0.0015104832127690315f, 0.0012750175083056092f, 0.0016061734640970826f, 0.0013796247076243162f, 0.0016040459740906954f, 0.0012694448232650757f, 0.0010858892928808928f, 0.0011950850021094084f, 0.0011526849120855331f, 0.0011402826057747006f, 0.0011891672620549798f, 0.0016124570975080132f, 0.0013228588504716754f, 0.0017809856217354536f, 0.0015323380939662457f, 0.0016058386536315084f, 0.0017686064820736647f, 0.0013457525055855513f, 0.0014485011342912912f, 0.0012284201802685857f, 0.0010653067147359252f, 0.001459175138734281f, 0.001325731398537755f, 0.0020158833358436823f, 0.0018564267084002495f, 0.0012456926051527262f, 0.0018260566284880042f, 0.0013748856727033854f, 0.0014411351876333356f, 0.001500045065768063f, 0.0016664854483678937f, 0.0012896092375740409f, 0.0014178500277921557f, 0.0016416349681094289f, 0.0015763085102662444f, 0.001039219438098371f, 0.0013048963155597448f, 0.0013259187107905746f, 0.0014221047749742866f, 0.0011831352021545172f);
static const ai_u16 conv2d_12_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_12_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_13_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_13_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_13_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_13_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_13_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_13_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_13_t_in_0_fmt_zero_const_s8 = -2;
static const ai_i8 conv2d_13_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_13_t_in_0_fmt_scale_const_f32 = 0.008548066951334476f;
static const ai_float conv2d_13_t_out_0_fmt_scale_const_f32 = 0.014447614550590515f;
static const ai_float conv2d_13_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009052262641489506f, 0.008704730309545994f, 0.00902447011321783f, 0.009600180201232433f, 0.008885969407856464f, 0.00975857675075531f, 0.015219099819660187f, 0.009478017687797546f, 0.011707057245075703f, 0.009087787009775639f, 0.012030822224915028f, 0.007035855203866959f, 0.008613462559878826f, 0.0097136739641428f, 0.011028282344341278f, 0.013497012667357922f, 0.008564364165067673f, 0.013013133779168129f, 0.011057289317250252f, 0.01035922672599554f, 0.01046062633395195f, 0.008847841061651707f, 0.011540330946445465f, 0.013670400716364384f, 0.00987069308757782f, 0.01192481443285942f, 0.010684683918952942f, 0.012470689602196217f, 0.01366847287863493f, 0.01303181517869234f, 0.008866517804563046f, 0.008965089917182922f, 0.016473060473799706f, 0.00925956666469574f, 0.009308233857154846f, 0.012732736766338348f, 0.011889871209859848f, 0.009726112708449364f, 0.011905754916369915f, 0.007243366912007332f, 0.008530287072062492f, 0.010587261989712715f, 0.012171106413006783f, 0.011935333721339703f, 0.014317171648144722f, 0.011885399930179119f, 0.010778902098536491f, 0.01059053000062704f, 0.00952381081879139f, 0.007289232686161995f, 0.009236703626811504f, 0.009097876958549023f, 0.010426095686852932f, 0.01458327379077673f, 0.015445426106452942f, 0.011805137619376183f, 0.01172320544719696f, 0.008417760021984577f, 0.01114699337631464f, 0.008961399085819721f, 0.010187815874814987f, 0.009338884614408016f, 0.009050981141626835f, 0.010574543848633766f, 0.009032231755554676f, 0.008758052252233028f, 0.007982172071933746f, 0.010199296288192272f, 0.013546524569392204f, 0.00714838458225131f, 0.013616160489618778f, 0.011270038783550262f, 0.00791814923286438f, 0.008737522177398205f, 0.016369568184018135f, 0.01782490871846676f, 0.012997709214687347f, 0.0103427330031991f, 0.014915167354047298f, 0.013833860866725445f, 0.0050392453558743f, 0.011580297723412514f, 0.009899290278553963f, 0.01032670121639967f, 0.007663832977414131f, 0.006809890270233154f, 0.011312410235404968f, 0.011132623068988323f, 0.012136814184486866f, 0.01434097159653902f, 0.008883899077773094f, 0.012879624962806702f, 0.007088550366461277f, 0.0096534863114357f, 0.009888305328786373f, 0.012594730593264103f, 0.00565881235525012f, 0.009836596436798573f, 0.01056678220629692f, 0.009468335658311844f, 0.014079094864428043f, 0.008826668374240398f, 0.010097486898303032f, 0.00830908864736557f, 0.01177462562918663f, 0.009693852625787258f, 0.01401108130812645f, 0.007248796056956053f, 0.01296805776655674f, 0.0071129086427390575f, 0.011100002564489841f, 0.011245528236031532f, 0.00922447070479393f, 0.009399015456438065f, 0.00949705857783556f, 0.005415824707597494f, 0.010638360865414143f, 0.011263485997915268f, 0.01380511000752449f, 0.00842286366969347f, 0.0072211455553770065f, 0.008869878947734833f, 0.007549306843429804f, 0.010151411406695843f, 0.008017851039767265f, 0.020217441022396088f, 0.013844638131558895f, 0.008597946725785732f);
static const ai_layer_format_type conv2d_13_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_14_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_14_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_14_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_14_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_14_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_14_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_14_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_14_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_14_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_14_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_14_t_out_0_fmt_zero_const_s8 = 1;
static const ai_float conv2d_14_t_in_0_fmt_scale_const_f32 = 0.014447614550590515f;
static const ai_float conv2d_14_t_out_0_fmt_scale_const_f32 = 0.004470477811992168f;
static const ai_float conv2d_14_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0010796966962516308f, 0.0003844976599793881f, 0.0007644927245564759f, 0.0008462510886602104f, 0.0005012486362829804f, 0.000918237492442131f, 0.0008213226101361215f, 0.0009822347201406956f, 0.0008247814839705825f, 0.0010760590666905046f, 0.0008447885629720986f, 0.0005217079888097942f, 0.0005256124422885478f, 0.0013810874661430717f, 0.0006275614723563194f, 0.0009652540320530534f, 0.0007252117502503097f, 0.0009322246769443154f, 0.0004950807197019458f, 0.0003315657959319651f, 0.0005923935095779598f, 0.0008720951736904681f, 0.0009821587009355426f, 0.0009041936718858778f, 0.0011318501783534884f, 0.0007007764070294797f, 0.0008998566772788763f, 0.0008159729768522084f, 0.000682308804243803f, 0.0010714755626395345f, 0.000993233174085617f, 0.00143449439201504f, 0.0007972404127940536f, 0.0008548149489797652f, 0.0012375941732898355f, 0.0011149829952046275f, 0.0006033542449586093f, 0.0006020381697453558f, 0.0005616738344542682f, 0.0007843311177566648f, 0.0007080133073031902f, 0.0009893652750179172f, 0.0006896273698657751f, 0.0005390084115788341f, 0.0010062844958156347f, 0.0010671353666111827f, 0.0007306492771022022f, 0.0007142844842746854f, 0.0007683357689529657f, 0.0007273326627910137f, 0.000898867379873991f, 0.0009258104837499559f, 0.0005527357570827007f, 0.0008245436474680901f, 0.0007552063325420022f, 0.0012280879309400916f, 0.0013466349337249994f, 0.000946998770814389f, 0.0009768136078491807f, 0.0006929192459210753f, 0.0010954708559438586f, 0.0008730982081033289f, 0.0005766610847786069f, 0.000565466471016407f, 0.0005274424911476672f, 0.0007936526671983302f, 0.001150508294813335f, 0.0004948034766130149f, 0.0005158817511983216f, 0.0005875024944543839f, 0.0005514647345989943f, 0.0008474974310956895f, 0.0005438849329948425f, 0.0009711352176964283f, 0.0006308118463493884f, 0.0010386994108557701f, 0.0010573719628155231f, 0.000619579921476543f, 0.0008799045463092625f, 0.0005934705259278417f, 0.0005607172497548163f, 0.000714593508746475f, 0.001462500891648233f, 0.0005241928738541901f, 0.00048533277004025877f, 0.0007574374321848154f, 0.0007972524035722017f, 0.0010412088595330715f, 0.0011378985363990068f, 0.0005427876603789628f, 0.0014396065380424261f, 0.0009536214056424797f, 0.0005492522614076734f, 0.0011327180545777082f, 0.0007704243180342019f, 0.0005635529523715377f, 0.0005591263761743903f, 0.0004048196133226156f, 0.000630093680229038f, 0.0005608721985481679f, 0.0008388249552808702f, 0.0008156876428984106f, 0.0013968278653919697f, 0.0012384976726025343f, 0.0005923896096646786f, 0.0012034603860229254f, 0.0013213810743764043f, 0.0005078322719782591f, 0.0005463400739245117f, 0.0009296536445617676f, 0.0006855715182609856f, 0.001432610908523202f, 0.0005581617006100714f, 0.0005834120092913508f, 0.00047912742593325675f, 0.0005380413495004177f, 0.0008670138195157051f, 0.0010302274022251368f, 0.001159877167083323f, 0.0008592742960900068f, 0.00139694067183882f, 0.0008505433797836304f, 0.0004803815681952983f, 0.000423160643549636f, 0.0011628378415480256f, 0.001212894101627171f, 0.00067012885119766f, 0.0005549729103222489f);
static const ai_u16 conv2d_14_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_14_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_15_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_15_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_15_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_15_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_15_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_15_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_15_t_in_0_fmt_zero_const_s8 = 1;
static const ai_i8 conv2d_15_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_15_t_in_0_fmt_scale_const_f32 = 0.004470477811992168f;
static const ai_float conv2d_15_t_out_0_fmt_scale_const_f32 = 0.013698581606149673f;
static const ai_float conv2d_15_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.01369031798094511f, 0.007116987835615873f, 0.018814926967024803f, 0.014342814683914185f, 0.019107267260551453f, 0.014079743064939976f, 0.017291761934757233f, 0.01511998288333416f, 0.01284729316830635f, 0.017402401193976402f, 0.017419159412384033f, 0.014915859326720238f, 0.01955580525100231f, 0.007108514662832022f, 0.02194775640964508f, 0.014223726466298103f, 0.017781948670744896f, 0.019198836758732796f, 0.013648312538862228f, 0.017618516460061073f, 0.007759117055684328f, 0.010583436116576195f, 0.016311531886458397f, 0.014596548862755299f, 0.015242232009768486f, 0.01664857566356659f, 0.006757889874279499f, 0.01596366986632347f, 0.018276449292898178f, 0.025993280112743378f, 0.019181711599230766f, 0.015204892493784428f, 0.018387475982308388f, 0.014782767742872238f, 0.013300498947501183f, 0.011776990257203579f, 0.016809841617941856f, 0.011609533801674843f, 0.014912085607647896f, 0.01490392442792654f, 0.014207745902240276f, 0.013493843376636505f, 0.013888747431337833f, 0.01360359601676464f, 0.020581085234880447f, 0.015331684611737728f, 0.013025588355958462f, 0.013460115529596806f, 0.009805582463741302f, 0.016209734603762627f, 0.013122282922267914f, 0.014055428095161915f, 0.021951189264655113f, 0.01854441687464714f, 0.016293341293931007f, 0.0143860699608922f, 0.016858965158462524f, 0.013849515467882156f, 0.015499327331781387f, 0.010441392660140991f, 0.014282104559242725f, 0.011756873689591885f, 0.019270144402980804f, 0.012726660817861557f, 0.006517812609672546f, 0.019710969179868698f, 0.01750912144780159f, 0.016302989795804024f, 0.018667781725525856f, 0.016613835468888283f, 0.014772405847907066f, 0.01876426674425602f, 0.017448505386710167f, 0.014352782629430294f, 0.014744020998477936f, 0.01460487861186266f, 0.015676526352763176f, 0.017032450065016747f, 0.01178548764437437f, 0.016363345086574554f, 0.020479809492826462f, 0.013743368908762932f, 0.016164399683475494f, 0.015188165940344334f, 0.013272108510136604f, 0.020009364932775497f, 0.01667594723403454f, 0.016272151842713356f, 0.012432940304279327f, 0.01781620644032955f, 0.022608494386076927f, 0.010967394337058067f, 0.014520404860377312f, 0.014810849912464619f, 0.013865679502487183f, 0.015252617187798023f, 0.021044336259365082f, 0.015325760468840599f, 0.012746903114020824f, 0.007582306396216154f, 0.013663536868989468f, 0.016986127942800522f, 0.014691190794110298f, 0.015000732615590096f, 0.0173721294850111f, 0.016032688319683075f, 0.010918443091213703f, 0.014458357356488705f, 0.018412671983242035f, 0.019762616604566574f, 0.020003745332360268f, 0.01844138279557228f, 0.01715894415974617f, 0.01769823580980301f, 0.018524186685681343f, 0.01690833456814289f, 0.014203568920493126f, 0.013040998950600624f, 0.01022961176931858f, 0.015651065856218338f, 0.01442977786064148f, 0.016660604625940323f, 0.020637307316064835f, 0.016407623887062073f, 0.014887228608131409f, 0.015479901805520058f, 0.018016716465353966f, 0.012345901690423489f);
static const ai_layer_format_type conv2d_15_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_16_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_16_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_16_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_16_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_16_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_16_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_16_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_16_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_16_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_16_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_16_t_out_0_fmt_zero_const_s8 = -14;
static const ai_float conv2d_16_t_in_0_fmt_scale_const_f32 = 0.013698581606149673f;
static const ai_float conv2d_16_t_out_0_fmt_scale_const_f32 = 0.004728487227112055f;
static const ai_float conv2d_16_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0010843885829672217f, 0.0005499893450178206f, 0.0005601543816737831f, 0.00045270859845913947f, 0.0004718564741779119f, 0.0005782282096333802f, 0.0004989283625036478f, 0.0005101881106384099f, 0.0004352133546490222f, 0.0005255020223557949f, 0.0008632761891931295f, 0.0004891273565590382f, 0.00075834448216483f, 0.000495345622766763f, 0.0011659084120765328f, 0.0011913124471902847f, 0.0012210500426590443f, 0.0005978752742521465f, 0.0010266610188409686f, 0.0005981391877867281f, 0.0005357737536542118f, 0.0005469541647471488f, 0.0007369131781160831f, 0.0005519356345757842f, 0.0005330072017386556f, 0.0008475257200188935f, 0.000409975356888026f, 0.0009951244574040174f, 0.0005070035113021731f, 0.0005501903360709548f, 0.0010715501848608255f, 0.0005651208339259028f, 0.0007060696370899677f, 0.001291477121412754f, 0.0008406498818658292f, 0.0013607481960207224f, 0.0004290239012334496f, 0.0005560768768191338f, 0.0005225706263445318f, 0.0004748157225549221f, 0.0009829102782532573f, 0.0005561579600907862f, 0.0005289834225550294f, 0.0007682563154958189f, 0.0007440014160238206f, 0.0006387805915437639f, 0.0008770643617026508f, 0.0005444369744509459f, 0.0005133600789122283f, 0.0012762013357132673f, 0.0007407049997709692f, 0.0007011635461822152f, 0.0005343049997463822f, 0.0013316783588379622f, 0.000598883896600455f, 0.0007006447413004935f, 0.0008675801218487322f, 0.0012628959957510233f, 0.0006400837446562946f, 0.00039047191967256367f, 0.000690581277012825f, 0.0006818269030191004f, 0.0007338912109844387f, 0.0005367923295125365f, 0.0005484775174409151f, 0.0006414433009922504f, 0.0012738120276480913f, 0.0006018396816216409f, 0.0007121228845790029f, 0.0004663596919272095f, 0.001016078400425613f, 0.0004548649303615093f, 0.001150730182416737f, 0.000984537648037076f, 0.000667615095153451f, 0.0013938097981736064f, 0.0010589489247649908f, 0.0006631039432249963f, 0.00044978398364037275f, 0.001483315136283636f, 0.0009032443049363792f, 0.0010374188423156738f, 0.0014403567183762789f, 0.000986841507256031f, 0.0005738717154599726f, 0.0009939068695530295f, 0.0014027925208210945f, 0.0008329256088472903f, 0.000921297469176352f, 0.0013259206898510456f, 0.0009287580032832921f, 0.00045686436351388693f, 0.0011665122583508492f, 0.0016082065412774682f, 0.0007864395738579333f, 0.0006306276773102582f, 0.0009270264417864382f, 0.0009612026624381542f, 0.0009458930580876768f, 0.0005286043160595f, 0.0005405113915912807f, 0.0009149381658062339f, 0.0004846183001063764f, 0.0004644412256311625f, 0.0007613392663188279f, 0.0009093045955523849f, 0.0005155221442691982f, 0.0010933317244052887f, 0.0009227327536791563f, 0.001140935462899506f, 0.0009639723575673997f, 0.0013368549989536405f, 0.00043402364826761186f, 0.000543214671779424f, 0.0011216573184356093f, 0.001110806711949408f, 0.0010615141363814473f, 0.0007029255502857268f, 0.0005020885728299618f, 0.0008993531810119748f, 0.0006111618131399155f, 0.0009854405652731657f, 0.0008442340767942369f, 0.0013594714691862464f, 0.00047999544767662883f, 0.00031409383518621325f, 0.0005234880372881889f, 0.000469998485641554f);
static const ai_u16 conv2d_16_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_16_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_17_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_17_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_17_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_17_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_17_t_in_0_fmt_zero_const_s8 = -14;
static const ai_i8 conv2d_17_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_17_t_in_0_fmt_scale_const_f32 = 0.004728487227112055f;
static const ai_float conv2d_17_t_out_0_fmt_scale_const_f32 = 0.01523957122117281f;
static const ai_float conv2d_17_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.012687531299889088f, 0.015879036858677864f, 0.01298667211085558f, 0.016629153862595558f, 0.015132671222090721f, 0.012343023903667927f, 0.01445224229246378f, 0.01563872955739498f, 0.0141607029363513f, 0.013364026322960854f, 0.016520995646715164f, 0.01558882649987936f, 0.010177231393754482f, 0.013119593262672424f, 0.019560588523745537f, 0.013670727610588074f, 0.01424561906605959f, 0.015381399542093277f, 0.015291600488126278f, 0.014181301929056644f, 0.011374515481293201f, 0.01115395501255989f, 0.01258965115994215f, 0.016505509614944458f, 0.014267511665821075f, 0.01705573871731758f, 0.012801711447536945f, 0.010032749734818935f, 0.012649759650230408f, 0.010962363332509995f, 0.013662890531122684f, 0.008981176652014256f, 0.013055887073278427f, 0.015902576968073845f, 0.012222912162542343f, 0.011792270466685295f, 0.013162282295525074f, 0.012495236471295357f, 0.012788957916200161f, 0.010654790326952934f, 0.02169697731733322f, 0.015617912635207176f, 0.013094032183289528f, 0.011715603061020374f, 0.01325643714517355f, 0.013187471777200699f, 0.01578732579946518f, 0.01138023380190134f, 0.013227427378296852f, 0.012250238098204136f, 0.01392244640737772f, 0.01129058562219143f, 0.013594157062470913f, 0.012176919728517532f, 0.011323319748044014f, 0.01267244666814804f, 0.011456741020083427f, 0.012035553343594074f, 0.01678716018795967f, 0.009575599804520607f, 0.018716396763920784f, 0.012727966532111168f, 0.009396890178322792f, 0.013194691389799118f, 0.016371889039874077f, 0.012507978826761246f, 0.015300010330975056f, 0.019009307026863098f, 0.011681673116981983f, 0.009054240770637989f, 0.012561921030282974f, 0.01601969636976719f, 0.014716454781591892f, 0.01606929674744606f, 0.01611088216304779f, 0.012402789667248726f, 0.013386327773332596f, 0.01303687784820795f, 0.006704950239509344f, 0.006758891046047211f, 0.013276995159685612f, 0.011627547442913055f, 0.014121465384960175f, 0.018736738711595535f, 0.011840428225696087f, 0.008185887709259987f, 0.01612960360944271f, 0.01221668254584074f, 0.011501257307827473f, 0.014706436544656754f, 0.014330830425024033f, 0.011729240417480469f, 0.011502950452268124f, 0.008786487393081188f, 0.012310243211686611f, 0.016445405781269073f, 0.01408917736262083f, 0.013095393776893616f, 0.012827032245695591f, 0.017755474895238876f, 0.01498806569725275f, 0.017165793105959892f, 0.016749471426010132f, 0.012803538702428341f, 0.012089557945728302f, 0.012918686494231224f, 0.012454750016331673f, 0.014542534947395325f, 0.014986369758844376f, 0.011884200386703014f, 0.023348968476057053f, 0.010097996331751347f, 0.011431927792727947f, 0.012167813256382942f, 0.009513532742857933f, 0.011399699375033379f, 0.013371247798204422f, 0.021890150383114815f, 0.01781892031431198f, 0.014897583983838558f, 0.011221369728446007f, 0.015181676484644413f, 0.014638491906225681f, 0.007136071566492319f, 0.010846429504454136f, 0.009701479226350784f, 0.015316624194383621f, 0.01552608609199524f);
static const ai_layer_format_type conv2d_17_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_18_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_18_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_18_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_18_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_18_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_18_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_18_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_18_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_18_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_18_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_18_t_out_0_fmt_zero_const_s8 = 1;
static const ai_float conv2d_18_t_in_0_fmt_scale_const_f32 = 0.01523957122117281f;
static const ai_float conv2d_18_t_out_0_fmt_scale_const_f32 = 0.005260588135570288f;
static const ai_float conv2d_18_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0005405393894761801f, 0.0005520228296518326f, 0.0014881775714457035f, 0.0007423321367241442f, 0.001321442541666329f, 0.0005223427433520555f, 0.0005508935428224504f, 0.0010479153133928776f, 0.0005795909091830254f, 0.0011052371701225638f, 0.0006247167475521564f, 0.0006149315158836544f, 0.0004928578273393214f, 0.0008570860372856259f, 0.001249112538062036f, 0.0008357964688912034f, 0.0006412589573301375f, 0.001097212778404355f, 0.0006905790069140494f, 0.0005457608494907618f, 0.0007725986652076244f, 0.001180513296276331f, 0.0015487874625250697f, 0.0009712675237096846f, 0.0005464921123348176f, 0.00055898871505633f, 0.0008638918516226113f, 0.0005256343865767121f, 0.0005232776165939867f, 0.0005098545807413757f, 0.0007870525005273521f, 0.0005619209259748459f, 0.0005606836057268083f, 0.0007693406078033149f, 0.0005562325823120773f, 0.0011227044742554426f, 0.0009661964722909033f, 0.0014290993567556143f, 0.0005044151330366731f, 0.0005190431838855147f, 0.00041795498691499233f, 0.0005135011160746217f, 0.000908931833691895f, 0.0005819855723530054f, 0.0012720339000225067f, 0.00114802410826087f, 0.0005610582884401083f, 0.000534599821548909f, 0.0011319308541715145f, 0.0005611599772237241f, 0.0010858410969376564f, 0.0005265354411676526f, 0.0010033720172941685f, 0.0007343917386606336f, 0.00037582271033897996f, 0.0008592374506406486f, 0.0009755302453413606f, 0.0009131048573181033f, 0.001257921103388071f, 0.0005887500010430813f, 0.001197014469653368f, 0.0005420775851234794f, 0.0004627160669770092f, 0.0005591881927102804f, 0.0010746733751147985f, 0.0005224830820225179f, 0.0006444710888899863f, 0.0005462320405058563f, 0.0005288788815960288f, 0.0004349258670117706f, 0.0007201277767308056f, 0.0005179230938665569f, 0.0012035079998895526f, 0.00046757585369050503f, 0.000632464129012078f, 0.0011345167877152562f, 0.0005150108481757343f, 0.0005155048565939069f, 0.0005415531923063099f, 0.0005596945411525667f, 0.0013694086810573936f, 0.0007567813736386597f, 0.0015614200383424759f, 0.0010715732350945473f, 0.0006213767337612808f, 0.0005536086391657591f, 0.0012376547092571855f, 0.0005153179517947137f, 0.0005629454972222447f, 0.0014659642474725842f, 0.000525290728546679f, 0.0005015777423977852f, 0.0005566512118093669f, 0.0005152856465429068f, 0.0011407567653805017f, 0.0008409207803197205f, 0.0006297333748079836f, 0.0006758187664672732f, 0.0007747742929495871f, 0.0010349858785048127f, 0.0011216311249881983f, 0.0005382008966989815f, 0.0005366891855373979f, 0.00046733461203984916f, 0.0008187654311768711f, 0.0005311129498295486f, 0.0012618087930604815f, 0.0008183883619494736f, 0.0012171672424301505f, 0.0013074654852971435f, 0.001252736896276474f, 0.0005346154794096947f, 0.0007271575159393251f, 0.0004650185874197632f, 0.0005387813434936106f, 0.0014425787376239896f, 0.0005653818952850997f, 0.0005404825205914676f, 0.0006840917048975825f, 0.0012738908408209682f, 0.0004158248193562031f, 0.0005044632125645876f, 0.0007452124846167862f, 0.0005044840509071946f, 0.0012894603423774242f, 0.0005485969595611095f, 0.0004943288513459265f, 0.00098859251011163f);
static const ai_u16 conv2d_18_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_18_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_19_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_19_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_19_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_19_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_19_t_in_0_fmt_zero_const_s8 = 1;
static const ai_i8 conv2d_19_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_19_t_in_0_fmt_scale_const_f32 = 0.005260588135570288f;
static const ai_float conv2d_19_t_out_0_fmt_scale_const_f32 = 0.01449789758771658f;
static const ai_float conv2d_19_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.014757588505744934f, 0.015008280985057354f, 0.014563513919711113f, 0.01314404048025608f, 0.012005756609141827f, 0.014850876294076443f, 0.010054741986095905f, 0.011487913317978382f, 0.016428150236606598f, 0.014335598796606064f, 0.00604217080399394f, 0.011472508311271667f, 0.0133413877338171f, 0.013365897350013256f, 0.014815458096563816f, 0.011662773787975311f, 0.013531542383134365f, 0.011955621652305126f, 0.012730698101222515f, 0.013913396745920181f, 0.012257769703865051f, 0.010376629419624805f, 0.011250617913901806f, 0.011396928690373898f, 0.016004309058189392f, 0.00961929839104414f, 0.015556609258055687f, 0.017773546278476715f, 0.009847139939665794f, 0.010027839802205563f, 0.01424731407314539f, 0.011893700808286667f, 0.016726002097129822f, 0.009891046211123466f, 0.01129503920674324f, 0.015117730014026165f, 0.013511525467038155f, 0.011583262123167515f, 0.01340374257415533f, 0.01493407879024744f, 0.014273887500166893f, 0.013525889255106449f, 0.010960849933326244f, 0.0153218237683177f, 0.011843339540064335f, 0.012337075546383858f, 0.015801802277565002f, 0.006432769820094109f, 0.014254332520067692f, 0.014134135097265244f, 0.014636007137596607f, 0.00938908476382494f, 0.013118809089064598f, 0.01425036508589983f, 0.012175701558589935f, 0.01346734818071127f, 0.009270458482205868f, 0.011859658174216747f, 0.01500607468187809f, 0.01243309210985899f, 0.014494326896965504f, 0.014442973770201206f, 0.010598891414701939f, 0.01116788201034069f, 0.01527506671845913f, 0.012740637175738811f, 0.006132039707154036f, 0.010198189876973629f, 0.013832435943186283f, 0.008879702538251877f, 0.014773732051253319f, 0.018543308600783348f, 0.011355419643223286f, 0.0105590233579278f, 0.014827380888164043f, 0.013166038319468498f, 0.014582637697458267f, 0.010693483054637909f, 0.011978176422417164f, 0.013836297206580639f, 0.010651101358234882f, 0.015596194192767143f, 0.008846777491271496f, 0.012457914650440216f, 0.009067438542842865f, 0.012785042636096478f, 0.013000844977796078f, 0.012534377165138721f, 0.011832206510007381f, 0.011037222109735012f, 0.011964784003794193f, 0.012133335694670677f, 0.01763821765780449f, 0.012162791565060616f, 0.013491044752299786f, 0.012347562238574028f, 0.013896928168833256f, 0.011369972489774227f, 0.011748192831873894f, 0.016096748411655426f, 0.012834048829972744f, 0.01379112433642149f, 0.00848342664539814f, 0.013128279708325863f, 0.01579264923930168f, 0.01511375792324543f, 0.015068280510604382f, 0.007400451228022575f, 0.010126590728759766f, 0.011296686716377735f, 0.010241163894534111f, 0.011610273271799088f, 0.010132263414561749f, 0.012236570008099079f, 0.013256010599434376f, 0.010268812999129295f, 0.016005368903279305f, 0.01654163748025894f, 0.010023258626461029f, 0.014086397364735603f, 0.015571328811347485f, 0.01414170116186142f, 0.018170339986681938f, 0.013205681927502155f, 0.008832856081426144f, 0.015150611288845539f, 0.016416676342487335f, 0.018022570759058f);
static const ai_layer_format_type conv2d_19_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_20_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_20_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_20_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_20_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_20_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_20_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_20_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_20_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_20_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_20_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_20_t_out_0_fmt_zero_const_s8 = 17;
static const ai_float conv2d_20_t_in_0_fmt_scale_const_f32 = 0.01449789758771658f;
static const ai_float conv2d_20_t_out_0_fmt_scale_const_f32 = 0.005288952030241489f;
static const ai_float conv2d_20_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0012621794594451785f, 0.0007957236375659704f, 0.0005429284065030515f, 0.0004585441784001887f, 0.0006248839781619608f, 0.0013243682915344834f, 0.0006393491639755666f, 0.0004295659309718758f, 0.0005656126886606216f, 0.0004131986934226006f, 0.0005643164040520787f, 0.000540158012881875f, 0.0007917164475657046f, 0.0007483870722353458f, 0.0008217556169256568f, 0.0007908664993010461f, 0.00045968222548253834f, 0.0009141658083535731f, 0.0010353511897847056f, 0.0005167865310795605f, 0.0011958916438743472f, 0.0015066365012899041f, 0.0009230677969753742f, 0.00048501594574190676f, 0.0009469585493206978f, 0.0005619548610411584f, 0.0009859900455921888f, 0.0004045911191496998f, 0.0005267880042083561f, 0.0005127801559865475f, 0.0007299788994714618f, 0.0009778996463865042f, 0.0008693264680914581f, 0.0005434628110378981f, 0.0006372321513481438f, 0.0011925039580091834f, 0.0009408863261342049f, 0.0005629958468489349f, 0.0011902058031409979f, 0.0007760156295262277f, 0.000513143721036613f, 0.0007632613414898515f, 0.0004315285477787256f, 0.0012525265337899327f, 0.001640112604945898f, 0.0008061709231697023f, 0.0006484019686467946f, 0.0005336414906196296f, 0.0007012803689576685f, 0.001109361881390214f, 0.0010140743106603622f, 0.0005443424452096224f, 0.000883872271515429f, 0.000986758735962212f, 0.00099583575502038f, 0.0008489895262755454f, 0.0005526648601517081f, 0.0008513355860486627f, 0.0009881866862997413f, 0.0005228028749115765f, 0.0008584539173170924f, 0.0004765699850395322f, 0.0005415564519353211f, 0.0005058511742390692f, 0.0006237018969841301f, 0.0005117267719469965f, 0.0005643657059408724f, 0.0009526842040941119f, 0.001149025745689869f, 0.0005648810183629394f, 0.0010843456257134676f, 0.0008857973734848201f, 0.0005462109111249447f, 0.0005467640003189445f, 0.0013390042586252093f, 0.0007428355165757239f, 0.0011874539777636528f, 0.0005530350608751178f, 0.0005523297586478293f, 0.0005175800761207938f, 0.0005225006025284529f, 0.0008066915906965733f, 0.0004599785024765879f, 0.0008555820095352829f, 0.0004608617746271193f, 0.0008594916434958577f, 0.0004888533730991185f, 0.0007555148331448436f, 0.0012481975136324763f, 0.000533592770807445f, 0.0006918471190147102f, 0.0014498833334073424f, 0.000644252693746239f, 0.0004911074065603316f, 0.0009024290484376252f, 0.0007262869621627033f, 0.0006438071723096073f, 0.0012529902160167694f, 0.0007652930798940361f, 0.0009693909669294953f, 0.0013302526203915477f, 0.0009822652209550142f, 0.0005442609544843435f, 0.0005485287983901799f, 0.0005210018716752529f, 0.00118594104424119f, 0.000501974776852876f, 0.00041004637023434043f, 0.0004396740405354649f, 0.0005633472464978695f, 0.0004232663777656853f, 0.00101473240647465f, 0.0005645605269819498f, 0.0005466244765557349f, 0.0012537556467577815f, 0.0005337486509233713f, 0.0013790444936603308f, 0.0009829781483858824f, 0.0007970989681780338f, 0.0006413087830878794f, 0.00141510262619704f, 0.0005476766382344067f, 0.0008642703760415316f, 0.0011319906916469336f, 0.0005553239607252181f, 0.0008027207222767174f, 0.001201934996061027f, 0.0005831205635331571f);
static const ai_u16 conv2d_20_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_20_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_21_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_21_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_21_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_21_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_21_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_21_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_21_t_in_0_fmt_zero_const_s8 = 17;
static const ai_i8 conv2d_21_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_21_t_in_0_fmt_scale_const_f32 = 0.005288952030241489f;
static const ai_float conv2d_21_t_out_0_fmt_scale_const_f32 = 0.01512584276497364f;
static const ai_float conv2d_21_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.016066309064626694f, 0.00540109584107995f, 0.012357625178992748f, 0.013402472250163555f, 0.014795366674661636f, 0.012301583774387836f, 0.012566271238029003f, 0.011074642650783062f, 0.012941828928887844f, 0.01539556123316288f, 0.01491097267717123f, 0.00970467273145914f, 0.012904779054224491f, 0.01493388507515192f, 0.007748022209852934f, 0.012354329228401184f, 0.013214894570410252f, 0.01072593405842781f, 0.011544017121195793f, 0.012299938127398491f, 0.017668450251221657f, 0.010673646815121174f, 0.010335037484765053f, 0.013498895801603794f, 0.013594787567853928f, 0.014040464535355568f, 0.012111041694879532f, 0.012036715634167194f, 0.009658966213464737f, 0.01469398569315672f, 0.013381298631429672f, 0.010654060170054436f, 0.01271172147244215f, 0.0135861961171031f, 0.009411952458322048f, 0.011445729993283749f, 0.021629225462675095f, 0.01174328662455082f, 0.012916384264826775f, 0.015014303848147392f, 0.014801024459302425f, 0.020268630236387253f, 0.010374132543802261f, 0.012267889454960823f, 0.011576310731470585f, 0.011885475367307663f, 0.013021044433116913f, 0.011309251189231873f, 0.015881329774856567f, 0.004902931395918131f, 0.006822807248681784f, 0.012180041521787643f, 0.014883152209222317f, 0.009866716340184212f, 0.01630244217813015f, 0.014423426240682602f, 0.013929770328104496f, 0.017860151827335358f, 0.017209118232131004f, 0.012004740536212921f, 0.012371651828289032f, 0.011241152882575989f, 0.013604240491986275f, 0.012663700617849827f, 0.01394190639257431f, 0.009007325395941734f, 0.013879334554076195f, 0.01991373673081398f, 0.014146201312541962f, 0.016204962506890297f, 0.017886260524392128f, 0.012791143730282784f, 0.013877329416573048f, 0.013857784681022167f, 0.012956815771758556f, 0.013693507760763168f, 0.01258973777294159f, 0.011562992818653584f, 0.012125382199883461f, 0.01273807231336832f, 0.010166988708078861f, 0.013168161734938622f, 0.01574740931391716f, 0.011716685257852077f, 0.01383046992123127f, 0.012343261390924454f, 0.013798603788018227f, 0.010236036963760853f, 0.016043899580836296f, 0.010907665826380253f, 0.014857834205031395f, 0.01403074897825718f, 0.019181760028004646f, 0.012425197288393974f, 0.013048260472714901f, 0.014376469887793064f, 0.013700147159397602f, 0.01680479757487774f, 0.011929517611861229f, 0.008844350464642048f, 0.01615283638238907f, 0.017083892598748207f, 0.010960816405713558f, 0.01440806221216917f, 0.0135119017213583f, 0.010392485186457634f, 0.011111539788544178f, 0.018270613625645638f, 0.013823572546243668f, 0.010946813970804214f, 0.01341303065419197f, 0.012829187326133251f, 0.010797044262290001f, 0.01153947226703167f, 0.010697203688323498f, 0.016201430931687355f, 0.01576533354818821f, 0.012097484432160854f, 0.014355413615703583f, 0.010611705482006073f, 0.010874687694013119f, 0.016033755615353584f, 0.012580041773617268f, 0.013889125548303127f, 0.014647472649812698f, 0.009634324349462986f, 0.012419425882399082f, 0.010449448600411415f);
static const ai_layer_format_type conv2d_21_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_22_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_22_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_22_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_22_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_22_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_22_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_22_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_22_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_22_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_22_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_22_t_out_0_fmt_zero_const_s8 = 65;
static const ai_float conv2d_22_t_in_0_fmt_scale_const_f32 = 0.01512584276497364f;
static const ai_float conv2d_22_t_out_0_fmt_scale_const_f32 = 0.005966615863144398f;
static const ai_float conv2d_22_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0006030527874827385f, 0.0004929183633066714f, 0.001434304635040462f, 0.0005015460192225873f, 0.001223151688463986f, 0.0006703277467750013f, 0.0005586190382018685f, 0.0005570084904320538f, 0.0005399803048931062f, 0.0009205428650602698f, 0.0010093733435496688f, 0.0005409949808381498f, 0.0009063594625331461f, 0.0004989149747416377f, 0.0004937889752909541f, 0.0011288239620625973f, 0.0008029513992369175f, 0.0007128908182494342f, 0.0006508329533971846f, 0.0007387277437373996f, 0.00039194669807329774f, 0.0007334648980759084f, 0.0007238651160150766f, 0.0007428410463035107f, 0.0005611169035546482f, 0.0007426739321090281f, 0.0014100285479798913f, 0.0007862843340262771f, 0.0007470489363186061f, 0.0013395664282143116f, 0.0007703010342083871f, 0.0004822276532649994f, 0.0004854176368098706f, 0.001127041527070105f, 0.0006307399016804993f, 0.000930057605728507f, 0.0007815695134922862f, 0.00045697783934883773f, 0.001153155928477645f, 0.0007774630794301629f, 0.0005419306689873338f, 0.00044794820132665336f, 0.0008128456538543105f, 0.0010794885456562042f, 0.0014897959772497416f, 0.0012979211751371622f, 0.0014014497864991426f, 0.0008498851675540209f, 0.0005174779798835516f, 0.00039454520447179675f, 0.0004961681552231312f, 0.0012379654217511415f, 0.0005626209895126522f, 0.0005529392510652542f, 0.0008735448936931789f, 0.0007562049431726336f, 0.0004565503913909197f, 0.000980200944468379f, 0.0008772051078267395f, 0.0007987184799276292f, 0.0007978409994393587f, 0.000553021440282464f, 0.0007752400706522167f, 0.00040190209983848035f, 0.000545317423529923f, 0.0005493652424775064f, 0.0006305178394541144f, 0.0005518507095985115f, 0.0009899461874738336f, 0.0005654890555888414f, 0.0011031394824385643f, 0.001694782287813723f, 0.0005552988732233644f, 0.0008933893404901028f, 0.0005598562420345843f, 0.0005425201379694045f, 0.0010714973323047161f, 0.000462374824564904f, 0.0004368530644569546f, 0.0009610764100216329f, 0.0005107673350721598f, 0.0010311453370377421f, 0.0009433520026504993f, 0.00046456640120595694f, 0.0009903969475999475f, 0.0008848428260535002f, 0.0010067871771752834f, 0.0008068179013207555f, 0.0006849090568721294f, 0.0005161938606761396f, 0.0005505029112100601f, 0.0007680097478441894f, 0.0007113227620720863f, 0.001022984622977674f, 0.000925844069570303f, 0.000431892549386248f, 0.0009030441287904978f, 0.002835631836205721f, 0.0009086245554499328f, 0.0005078756366856396f, 0.0011098412796854973f, 0.0007010299596004188f, 0.0007319838041439652f, 0.0005286395316943526f, 0.0009370788466185331f, 0.0005006308201700449f, 0.0004885637317784131f, 0.0006868050550110638f, 0.001027515041641891f, 0.0005116454558447003f, 0.000570449628867209f, 0.0010415182914584875f, 0.0005307877436280251f, 0.00045907613821327686f, 0.0005359968054108322f, 0.0009538072044961154f, 0.0005550483474507928f, 0.0013349899090826511f, 0.001466957968659699f, 0.0007395740249194205f, 0.0004983919789083302f, 0.0013174137566238642f, 0.0010749493958428502f, 0.0006850597565062344f, 0.0011066105216741562f, 0.0008304517832584679f, 0.0009154733852483332f, 0.0005219517624936998f);
static const ai_u16 conv2d_22_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_22_t_out_0_shape_h_const_u16 = 1;


static const ai_u16 conv2d_29_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_29_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_29_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_29_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_29_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_29_t_out_0_shape_ch_const_u16 = 10;
static const ai_i8 conv2d_29_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_29_t_out_0_fmt_zero_const_s8 = 26;
static const ai_float conv2d_29_t_in_0_fmt_scale_const_f32 = 0.01855890266597271f;
static const ai_float conv2d_29_t_out_0_fmt_scale_const_f32 = 0.20673346519470215f;
static const ai_float conv2d_29_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.006350325420498848f, 0.00428654532879591f, 0.004504958633333445f, 0.0034281101543456316f, 0.00399406161159277f, 0.005114847794175148f, 0.00594656728208065f, 0.004178408533334732f, 0.003596055554226041f, 0.004024783615022898f);
static const ai_layer_format_type conv2d_29_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u32 nl_30_t_in_0_shape_ch_h_w_prod_const_u32 = 10;

static const ai_u32 conversion_35_t_out_0_shape_h_w_ch_d_prod_const_u32 = 10;
static const ai_float conversion_35_t_in_0_fmt_scale_const_f32 = 0.00390625f;
static const ai_i8 conversion_35_t_in_0_fmt_zero_const_s8 = -128;
STAI_API_ENTRY
stai_return_code stai_network_run(
  stai_network* network,
  const stai_run_mode mode)
{
   STAI_UNUSED(mode)
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_ACTIVATIONS) != STAI_FLAG_ACTIVATIONS,
        STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_INPUTS) != STAI_FLAG_INPUTS,
                  STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_OUTPUTS) != STAI_FLAG_OUTPUTS,
                  STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_WEIGHTS) != STAI_FLAG_WEIGHTS,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)


  /* LITE_KERNEL_SECTION BEGIN conversion_0 */
  {
      const ai_float* conversion_0_t_in_0_ptr_const_f32 = (ai_float*)(net_ctx->_inputs[0] + 0);
    ai_i8* conversion_0_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(0, 1, {(stai_ptr) conversion_0_t_in_0_ptr_const_f32});
    
  forward_lite_node_convert_integer_if32os8(conversion_0_t_in_0_ptr_const_f32, conversion_0_t_out_0_ptr_s8, conversion_0_t_out_0_shape_h_w_ch_d_prod_const_u32, conversion_0_t_out_0_fmt_scale_const_f32, conversion_0_t_out_0_fmt_zero_const_s8);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(0, 1, {(stai_ptr) conversion_0_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conversion_0 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_1 */
  {
      const ai_i8* conv2d_1_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_1_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 0);
    const ai_i32* conv2d_1_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[1] + 0);
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 3728);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 3076);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_rgb_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_t_out_0_shape_ch_const_u16, conv2d_1_t_weight_0_shape_w_const_u16, conv2d_1_l_pad_W_0_const_s32, conv2d_1_l_stride_0_const_u16, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_l_out_ch_format_const_layer_format_type, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, 652, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_2 */
  {
      const ai_i8* conv2d_2_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 3728);
    const ai_i8* conv2d_2_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[2] + 0);
    const ai_i32* conv2d_2_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[3] + 0);
    ai_i8* conv2d_2_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 300);
    ai_i16* conv2d_2_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) conv2d_2_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_2_t_in_0_ptr_const_s8, conv2d_2_t_in_0_shape_w_const_u16, conv2d_2_t_in_0_shape_h_const_u16, conv2d_2_t_in_0_shape_ch_const_u16, conv2d_2_t_weight_0_ptr_const_s8, conv2d_2_t_weight_0_shape_w_const_u16, conv2d_2_t_weight_0_shape_h_const_u16, conv2d_2_l_pad_W_0_const_s32, conv2d_2_l_pad_H_0_const_s32, conv2d_2_l_stride_1_const_u16, conv2d_2_l_stride_0_const_u16, conv2d_2_t_weight_1_ptr_const_s32, conv2d_2_t_in_0_fmt_zero_const_s8, conv2d_2_t_out_0_fmt_zero_const_s8, conv2d_2_t_in_0_fmt_scale_const_f32, conv2d_2_t_out_0_fmt_scale_const_f32, conv2d_2_t_weight_0_fmt_scale_const_f32, conv2d_2_t_out_0_ptr_s8, conv2d_2_t_out_0_shape_w_const_u16, conv2d_2_t_out_0_shape_h_const_u16, 0, 297, conv2d_2_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) conv2d_2_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_2 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_3 */
  {
      const ai_i8* conv2d_3_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 300);
    const ai_i8* conv2d_3_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[4] + 0);
    const ai_i32* conv2d_3_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[5] + 0);
    ai_i8* conv2d_3_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 812);
    ai_i16* conv2d_3_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr) conv2d_3_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_3_t_in_0_ptr_const_s8, conv2d_3_t_in_0_shape_w_const_u16, conv2d_3_t_in_0_shape_h_const_u16, conv2d_3_l_stride_1_const_u16, conv2d_3_l_stride_0_const_u16, conv2d_3_t_in_0_shape_ch_const_u16, conv2d_3_t_weight_0_ptr_const_s8, conv2d_3_t_out_0_shape_ch_const_u16, conv2d_3_t_weight_1_ptr_const_s32, conv2d_3_t_in_0_fmt_zero_const_s8, conv2d_3_t_out_0_fmt_zero_const_s8, conv2d_3_t_in_0_fmt_scale_const_f32, conv2d_3_t_out_0_fmt_scale_const_f32, conv2d_3_t_weight_0_fmt_scale_const_f32, conv2d_3_l_out_ch_format_const_layer_format_type, conv2d_3_t_out_0_ptr_s8, 1, 192, conv2d_3_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr) conv2d_3_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_3 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_4 */
  {
      const ai_i8* conv2d_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 812);
    const ai_i8* conv2d_4_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[6] + 0);
    const ai_i32* conv2d_4_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[7] + 0);
    ai_i8* conv2d_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1836);
    ai_i16* conv2d_4_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) conv2d_4_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_4_t_in_0_ptr_const_s8, conv2d_4_t_in_0_shape_w_const_u16, conv2d_4_t_in_0_shape_h_const_u16, conv2d_4_t_in_0_shape_ch_const_u16, conv2d_4_t_weight_0_ptr_const_s8, conv2d_4_t_weight_0_shape_w_const_u16, conv2d_4_t_weight_0_shape_h_const_u16, conv2d_4_l_pad_W_0_const_s32, conv2d_4_l_pad_H_0_const_s32, conv2d_4_l_stride_1_const_u16, conv2d_4_l_stride_0_const_u16, conv2d_4_t_weight_1_ptr_const_s32, conv2d_4_t_in_0_fmt_zero_const_s8, conv2d_4_t_out_0_fmt_zero_const_s8, conv2d_4_t_in_0_fmt_scale_const_f32, conv2d_4_t_out_0_fmt_scale_const_f32, conv2d_4_t_weight_0_fmt_scale_const_f32, conv2d_4_t_out_0_ptr_s8, conv2d_4_t_out_0_shape_w_const_u16, conv2d_4_t_out_0_shape_h_const_u16, 0, 593, conv2d_4_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) conv2d_4_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_4 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5 */
  {
      const ai_i8* conv2d_5_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1836);
    const ai_i8* conv2d_5_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[8] + 0);
    const ai_i32* conv2d_5_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[9] + 0);
    ai_i8* conv2d_5_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 384);
    ai_i16* conv2d_5_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_5_t_in_0_ptr_const_s8, conv2d_5_t_in_0_shape_w_const_u16, conv2d_5_t_in_0_shape_h_const_u16, conv2d_5_l_stride_1_const_u16, conv2d_5_l_stride_0_const_u16, conv2d_5_t_in_0_shape_ch_const_u16, conv2d_5_t_weight_0_ptr_const_s8, conv2d_5_t_out_0_shape_ch_const_u16, conv2d_5_t_weight_1_ptr_const_s32, conv2d_5_t_in_0_fmt_zero_const_s8, conv2d_5_t_out_0_fmt_zero_const_s8, conv2d_5_t_in_0_fmt_scale_const_f32, conv2d_5_t_out_0_fmt_scale_const_f32, conv2d_5_t_weight_0_fmt_scale_const_f32, conv2d_5_l_out_ch_format_const_layer_format_type, conv2d_5_t_out_0_ptr_s8, 1, 384, conv2d_5_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_5 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_6 */
  {
      const ai_i8* conv2d_6_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 384);
    const ai_i8* conv2d_6_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[10] + 0);
    const ai_i32* conv2d_6_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[11] + 0);
    ai_i8* conv2d_6_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2084);
    ai_i16* conv2d_6_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 896);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 1, {(stai_ptr) conv2d_6_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_6_t_in_0_ptr_const_s8, conv2d_6_t_in_0_shape_w_const_u16, conv2d_6_t_in_0_shape_h_const_u16, conv2d_6_t_in_0_shape_ch_const_u16, conv2d_6_t_weight_0_ptr_const_s8, conv2d_6_t_weight_0_shape_w_const_u16, conv2d_6_t_weight_0_shape_h_const_u16, conv2d_6_l_pad_W_0_const_s32, conv2d_6_l_pad_H_0_const_s32, conv2d_6_l_stride_1_const_u16, conv2d_6_l_stride_0_const_u16, conv2d_6_t_weight_1_ptr_const_s32, conv2d_6_t_in_0_fmt_zero_const_s8, conv2d_6_t_out_0_fmt_zero_const_s8, conv2d_6_t_in_0_fmt_scale_const_f32, conv2d_6_t_out_0_fmt_scale_const_f32, conv2d_6_t_weight_0_fmt_scale_const_f32, conv2d_6_t_out_0_ptr_s8, conv2d_6_t_out_0_shape_w_const_u16, conv2d_6_t_out_0_shape_h_const_u16, 0, 1185, conv2d_6_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, {(stai_ptr) conv2d_6_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
  {
      const ai_i8* conv2d_7_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2084);
    const ai_i8* conv2d_7_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[12] + 0);
    const ai_i32* conv2d_7_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[13] + 0);
    ai_i8* conv2d_7_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 448);
    ai_i16* conv2d_7_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_7_t_in_0_ptr_const_s8, conv2d_7_t_in_0_shape_w_const_u16, conv2d_7_t_in_0_shape_h_const_u16, conv2d_7_l_stride_1_const_u16, conv2d_7_l_stride_0_const_u16, conv2d_7_t_in_0_shape_ch_const_u16, conv2d_7_t_weight_0_ptr_const_s8, conv2d_7_t_out_0_shape_ch_const_u16, conv2d_7_t_weight_1_ptr_const_s32, conv2d_7_t_in_0_fmt_zero_const_s8, conv2d_7_t_out_0_fmt_zero_const_s8, conv2d_7_t_in_0_fmt_scale_const_f32, conv2d_7_t_out_0_fmt_scale_const_f32, conv2d_7_t_weight_0_fmt_scale_const_f32, conv2d_7_l_out_ch_format_const_layer_format_type, conv2d_7_t_out_0_ptr_s8, 1, 448, conv2d_7_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_7 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_8 */
  {
      const ai_i8* conv2d_8_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 448);
    const ai_i8* conv2d_8_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[14] + 0);
    const ai_i32* conv2d_8_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[15] + 0);
    ai_i8* conv2d_8_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_8_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 960);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16, conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_t_in_0_shape_ch_const_u16, conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_weight_0_shape_w_const_u16, conv2d_8_t_weight_0_shape_h_const_u16, conv2d_8_l_pad_W_0_const_s32, conv2d_8_l_pad_H_0_const_s32, conv2d_8_l_stride_1_const_u16, conv2d_8_l_stride_0_const_u16, conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8, conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32, conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32, conv2d_8_t_out_0_ptr_s8, conv2d_8_t_out_0_shape_w_const_u16, conv2d_8_t_out_0_shape_h_const_u16, 0, 1185, conv2d_8_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_8 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_9 */
  {
      const ai_i8* conv2d_9_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_9_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[16] + 0);
    const ai_i32* conv2d_9_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[17] + 0);
    ai_i8* conv2d_9_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 896);
    ai_i16* conv2d_9_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 128);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_9_t_in_0_ptr_const_s8, conv2d_9_t_in_0_shape_w_const_u16, conv2d_9_t_in_0_shape_h_const_u16, conv2d_9_l_stride_1_const_u16, conv2d_9_l_stride_0_const_u16, conv2d_9_t_in_0_shape_ch_const_u16, conv2d_9_t_weight_0_ptr_const_s8, conv2d_9_t_out_0_shape_ch_const_u16, conv2d_9_t_weight_1_ptr_const_s32, conv2d_9_t_in_0_fmt_zero_const_s8, conv2d_9_t_out_0_fmt_zero_const_s8, conv2d_9_t_in_0_fmt_scale_const_f32, conv2d_9_t_out_0_fmt_scale_const_f32, conv2d_9_t_weight_0_fmt_scale_const_f32, conv2d_9_l_out_ch_format_const_layer_format_type, conv2d_9_t_out_0_ptr_s8, 1, 768, conv2d_9_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_9 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_10 */
  {
      const ai_i8* conv2d_10_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 896);
    const ai_i8* conv2d_10_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[18] + 0);
    const ai_i32* conv2d_10_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[19] + 0);
    ai_i8* conv2d_10_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_10_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1152);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(10, 1, {(stai_ptr) conv2d_10_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_10_t_in_0_ptr_const_s8, conv2d_10_t_in_0_shape_w_const_u16, conv2d_10_t_in_0_shape_h_const_u16, conv2d_10_t_in_0_shape_ch_const_u16, conv2d_10_t_weight_0_ptr_const_s8, conv2d_10_t_weight_0_shape_w_const_u16, conv2d_10_t_weight_0_shape_h_const_u16, conv2d_10_l_pad_W_0_const_s32, conv2d_10_l_pad_H_0_const_s32, conv2d_10_l_stride_1_const_u16, conv2d_10_l_stride_0_const_u16, conv2d_10_t_weight_1_ptr_const_s32, conv2d_10_t_in_0_fmt_zero_const_s8, conv2d_10_t_out_0_fmt_zero_const_s8, conv2d_10_t_in_0_fmt_scale_const_f32, conv2d_10_t_out_0_fmt_scale_const_f32, conv2d_10_t_weight_0_fmt_scale_const_f32, conv2d_10_t_out_0_ptr_s8, conv2d_10_t_out_0_shape_w_const_u16, conv2d_10_t_out_0_shape_h_const_u16, 0, 2369, conv2d_10_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, {(stai_ptr) conv2d_10_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_10 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[20] + 0);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[21] + 0);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1152);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 256);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_l_stride_1_const_u16, conv2d_11_l_stride_0_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_t_out_0_shape_ch_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_l_out_ch_format_const_layer_format_type, conv2d_11_t_out_0_ptr_s8, 1, 896, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1152);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[22] + 0);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[23] + 0);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1408);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_12_t_in_0_ptr_const_s8, conv2d_12_t_in_0_shape_w_const_u16, conv2d_12_t_in_0_shape_h_const_u16, conv2d_12_t_in_0_shape_ch_const_u16, conv2d_12_t_weight_0_ptr_const_s8, conv2d_12_t_weight_0_shape_w_const_u16, conv2d_12_t_weight_0_shape_h_const_u16, conv2d_12_l_pad_W_0_const_s32, conv2d_12_l_pad_H_0_const_s32, conv2d_12_l_stride_1_const_u16, conv2d_12_l_stride_0_const_u16, conv2d_12_t_weight_1_ptr_const_s32, conv2d_12_t_in_0_fmt_zero_const_s8, conv2d_12_t_out_0_fmt_zero_const_s8, conv2d_12_t_in_0_fmt_scale_const_f32, conv2d_12_t_out_0_fmt_scale_const_f32, conv2d_12_t_weight_0_fmt_scale_const_f32, conv2d_12_t_out_0_ptr_s8, conv2d_12_t_out_0_shape_w_const_u16, conv2d_12_t_out_0_shape_h_const_u16, 0, 2369, conv2d_12_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_12 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_13 */
  {
      const ai_i8* conv2d_13_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_13_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[24] + 0);
    const ai_i32* conv2d_13_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[25] + 0);
    ai_i8* conv2d_13_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1600);
    ai_i16* conv2d_13_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 64);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) conv2d_13_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_13_t_in_0_ptr_const_s8, conv2d_13_t_in_0_shape_w_const_u16, conv2d_13_t_in_0_shape_h_const_u16, conv2d_13_l_stride_1_const_u16, conv2d_13_l_stride_0_const_u16, conv2d_13_t_in_0_shape_ch_const_u16, conv2d_13_t_weight_0_ptr_const_s8, conv2d_13_t_out_0_shape_ch_const_u16, conv2d_13_t_weight_1_ptr_const_s32, conv2d_13_t_in_0_fmt_zero_const_s8, conv2d_13_t_out_0_fmt_zero_const_s8, conv2d_13_t_in_0_fmt_scale_const_f32, conv2d_13_t_out_0_fmt_scale_const_f32, conv2d_13_t_weight_0_fmt_scale_const_f32, conv2d_13_l_out_ch_format_const_layer_format_type, conv2d_13_t_out_0_ptr_s8, 1, 1536, conv2d_13_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) conv2d_13_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_13 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_14 */
  {
      const ai_i8* conv2d_14_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1600);
    const ai_i8* conv2d_14_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[26] + 0);
    const ai_i32* conv2d_14_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[27] + 0);
    ai_i8* conv2d_14_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_14_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1728);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 1, {(stai_ptr) conv2d_14_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_14_t_in_0_ptr_const_s8, conv2d_14_t_in_0_shape_w_const_u16, conv2d_14_t_in_0_shape_h_const_u16, conv2d_14_t_in_0_shape_ch_const_u16, conv2d_14_t_weight_0_ptr_const_s8, conv2d_14_t_weight_0_shape_w_const_u16, conv2d_14_t_weight_0_shape_h_const_u16, conv2d_14_l_pad_W_0_const_s32, conv2d_14_l_pad_H_0_const_s32, conv2d_14_l_stride_1_const_u16, conv2d_14_l_stride_0_const_u16, conv2d_14_t_weight_1_ptr_const_s32, conv2d_14_t_in_0_fmt_zero_const_s8, conv2d_14_t_out_0_fmt_zero_const_s8, conv2d_14_t_in_0_fmt_scale_const_f32, conv2d_14_t_out_0_fmt_scale_const_f32, conv2d_14_t_weight_0_fmt_scale_const_f32, conv2d_14_t_out_0_ptr_s8, conv2d_14_t_out_0_shape_w_const_u16, conv2d_14_t_out_0_shape_h_const_u16, 0, 4737, conv2d_14_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, {(stai_ptr) conv2d_14_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_14 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_15 */
  {
      const ai_i8* conv2d_15_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_15_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[28] + 0);
    const ai_i32* conv2d_15_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[29] + 0);
    ai_i8* conv2d_15_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1920);
    ai_i16* conv2d_15_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 128);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) conv2d_15_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_15_t_in_0_ptr_const_s8, conv2d_15_t_in_0_shape_w_const_u16, conv2d_15_t_in_0_shape_h_const_u16, conv2d_15_l_stride_1_const_u16, conv2d_15_l_stride_0_const_u16, conv2d_15_t_in_0_shape_ch_const_u16, conv2d_15_t_weight_0_ptr_const_s8, conv2d_15_t_out_0_shape_ch_const_u16, conv2d_15_t_weight_1_ptr_const_s32, conv2d_15_t_in_0_fmt_zero_const_s8, conv2d_15_t_out_0_fmt_zero_const_s8, conv2d_15_t_in_0_fmt_scale_const_f32, conv2d_15_t_out_0_fmt_scale_const_f32, conv2d_15_t_weight_0_fmt_scale_const_f32, conv2d_15_l_out_ch_format_const_layer_format_type, conv2d_15_t_out_0_ptr_s8, 1, 1792, conv2d_15_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) conv2d_15_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_15 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_16 */
  {
      const ai_i8* conv2d_16_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1920);
    const ai_i8* conv2d_16_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[30] + 0);
    const ai_i32* conv2d_16_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[31] + 0);
    ai_i8* conv2d_16_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_16_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2048);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(16, 1, {(stai_ptr) conv2d_16_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_16_t_in_0_ptr_const_s8, conv2d_16_t_in_0_shape_w_const_u16, conv2d_16_t_in_0_shape_h_const_u16, conv2d_16_t_in_0_shape_ch_const_u16, conv2d_16_t_weight_0_ptr_const_s8, conv2d_16_t_weight_0_shape_w_const_u16, conv2d_16_t_weight_0_shape_h_const_u16, conv2d_16_l_pad_W_0_const_s32, conv2d_16_l_pad_H_0_const_s32, conv2d_16_l_stride_1_const_u16, conv2d_16_l_stride_0_const_u16, conv2d_16_t_weight_1_ptr_const_s32, conv2d_16_t_in_0_fmt_zero_const_s8, conv2d_16_t_out_0_fmt_zero_const_s8, conv2d_16_t_in_0_fmt_scale_const_f32, conv2d_16_t_out_0_fmt_scale_const_f32, conv2d_16_t_weight_0_fmt_scale_const_f32, conv2d_16_t_out_0_ptr_s8, conv2d_16_t_out_0_shape_w_const_u16, conv2d_16_t_out_0_shape_h_const_u16, 0, 4737, conv2d_16_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(16, 1, {(stai_ptr) conv2d_16_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_16 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_17 */
  {
      const ai_i8* conv2d_17_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_17_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[32] + 0);
    const ai_i32* conv2d_17_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[33] + 0);
    ai_i8* conv2d_17_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1920);
    ai_i16* conv2d_17_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 128);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 1, {(stai_ptr) conv2d_17_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_17_t_in_0_ptr_const_s8, conv2d_17_t_in_0_shape_w_const_u16, conv2d_17_t_in_0_shape_h_const_u16, conv2d_17_l_stride_1_const_u16, conv2d_17_l_stride_0_const_u16, conv2d_17_t_in_0_shape_ch_const_u16, conv2d_17_t_weight_0_ptr_const_s8, conv2d_17_t_out_0_shape_ch_const_u16, conv2d_17_t_weight_1_ptr_const_s32, conv2d_17_t_in_0_fmt_zero_const_s8, conv2d_17_t_out_0_fmt_zero_const_s8, conv2d_17_t_in_0_fmt_scale_const_f32, conv2d_17_t_out_0_fmt_scale_const_f32, conv2d_17_t_weight_0_fmt_scale_const_f32, conv2d_17_l_out_ch_format_const_layer_format_type, conv2d_17_t_out_0_ptr_s8, 1, 1792, conv2d_17_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, {(stai_ptr) conv2d_17_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_17 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_18 */
  {
      const ai_i8* conv2d_18_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1920);
    const ai_i8* conv2d_18_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[34] + 0);
    const ai_i32* conv2d_18_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[35] + 0);
    ai_i8* conv2d_18_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_18_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2048);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(18, 1, {(stai_ptr) conv2d_18_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_18_t_in_0_ptr_const_s8, conv2d_18_t_in_0_shape_w_const_u16, conv2d_18_t_in_0_shape_h_const_u16, conv2d_18_t_in_0_shape_ch_const_u16, conv2d_18_t_weight_0_ptr_const_s8, conv2d_18_t_weight_0_shape_w_const_u16, conv2d_18_t_weight_0_shape_h_const_u16, conv2d_18_l_pad_W_0_const_s32, conv2d_18_l_pad_H_0_const_s32, conv2d_18_l_stride_1_const_u16, conv2d_18_l_stride_0_const_u16, conv2d_18_t_weight_1_ptr_const_s32, conv2d_18_t_in_0_fmt_zero_const_s8, conv2d_18_t_out_0_fmt_zero_const_s8, conv2d_18_t_in_0_fmt_scale_const_f32, conv2d_18_t_out_0_fmt_scale_const_f32, conv2d_18_t_weight_0_fmt_scale_const_f32, conv2d_18_t_out_0_ptr_s8, conv2d_18_t_out_0_shape_w_const_u16, conv2d_18_t_out_0_shape_h_const_u16, 0, 4737, conv2d_18_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(18, 1, {(stai_ptr) conv2d_18_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_18 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_19 */
  {
      const ai_i8* conv2d_19_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_19_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[36] + 0);
    const ai_i32* conv2d_19_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[37] + 0);
    ai_i8* conv2d_19_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1920);
    ai_i16* conv2d_19_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 128);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_19_t_in_0_ptr_const_s8, conv2d_19_t_in_0_shape_w_const_u16, conv2d_19_t_in_0_shape_h_const_u16, conv2d_19_l_stride_1_const_u16, conv2d_19_l_stride_0_const_u16, conv2d_19_t_in_0_shape_ch_const_u16, conv2d_19_t_weight_0_ptr_const_s8, conv2d_19_t_out_0_shape_ch_const_u16, conv2d_19_t_weight_1_ptr_const_s32, conv2d_19_t_in_0_fmt_zero_const_s8, conv2d_19_t_out_0_fmt_zero_const_s8, conv2d_19_t_in_0_fmt_scale_const_f32, conv2d_19_t_out_0_fmt_scale_const_f32, conv2d_19_t_weight_0_fmt_scale_const_f32, conv2d_19_l_out_ch_format_const_layer_format_type, conv2d_19_t_out_0_ptr_s8, 1, 1792, conv2d_19_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_19 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_20 */
  {
      const ai_i8* conv2d_20_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1920);
    const ai_i8* conv2d_20_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[38] + 0);
    const ai_i32* conv2d_20_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[39] + 0);
    ai_i8* conv2d_20_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_20_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2048);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(20, 1, {(stai_ptr) conv2d_20_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_20_t_in_0_ptr_const_s8, conv2d_20_t_in_0_shape_w_const_u16, conv2d_20_t_in_0_shape_h_const_u16, conv2d_20_t_in_0_shape_ch_const_u16, conv2d_20_t_weight_0_ptr_const_s8, conv2d_20_t_weight_0_shape_w_const_u16, conv2d_20_t_weight_0_shape_h_const_u16, conv2d_20_l_pad_W_0_const_s32, conv2d_20_l_pad_H_0_const_s32, conv2d_20_l_stride_1_const_u16, conv2d_20_l_stride_0_const_u16, conv2d_20_t_weight_1_ptr_const_s32, conv2d_20_t_in_0_fmt_zero_const_s8, conv2d_20_t_out_0_fmt_zero_const_s8, conv2d_20_t_in_0_fmt_scale_const_f32, conv2d_20_t_out_0_fmt_scale_const_f32, conv2d_20_t_weight_0_fmt_scale_const_f32, conv2d_20_t_out_0_ptr_s8, conv2d_20_t_out_0_shape_w_const_u16, conv2d_20_t_out_0_shape_h_const_u16, 0, 4737, conv2d_20_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(20, 1, {(stai_ptr) conv2d_20_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_20 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_21 */
  {
      const ai_i8* conv2d_21_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_21_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[40] + 0);
    const ai_i32* conv2d_21_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[41] + 0);
    ai_i8* conv2d_21_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1920);
    ai_i16* conv2d_21_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 128);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, {(stai_ptr) conv2d_21_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_21_t_in_0_ptr_const_s8, conv2d_21_t_in_0_shape_w_const_u16, conv2d_21_t_in_0_shape_h_const_u16, conv2d_21_l_stride_1_const_u16, conv2d_21_l_stride_0_const_u16, conv2d_21_t_in_0_shape_ch_const_u16, conv2d_21_t_weight_0_ptr_const_s8, conv2d_21_t_out_0_shape_ch_const_u16, conv2d_21_t_weight_1_ptr_const_s32, conv2d_21_t_in_0_fmt_zero_const_s8, conv2d_21_t_out_0_fmt_zero_const_s8, conv2d_21_t_in_0_fmt_scale_const_f32, conv2d_21_t_out_0_fmt_scale_const_f32, conv2d_21_t_weight_0_fmt_scale_const_f32, conv2d_21_l_out_ch_format_const_layer_format_type, conv2d_21_t_out_0_ptr_s8, 1, 1792, conv2d_21_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, {(stai_ptr) conv2d_21_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_21 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_22 */
  {
      const ai_i8* conv2d_22_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1920);
    const ai_i8* conv2d_22_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[42] + 0);
    const ai_i32* conv2d_22_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[43] + 0);
    ai_i8* conv2d_22_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_22_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2048);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(22, 1, {(stai_ptr) conv2d_22_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_22_t_in_0_ptr_const_s8, conv2d_22_t_in_0_shape_w_const_u16, conv2d_22_t_in_0_shape_h_const_u16, conv2d_22_t_in_0_shape_ch_const_u16, conv2d_22_t_weight_0_ptr_const_s8, conv2d_22_t_weight_0_shape_w_const_u16, conv2d_22_t_weight_0_shape_h_const_u16, conv2d_22_l_pad_W_0_const_s32, conv2d_22_l_pad_H_0_const_s32, conv2d_22_l_stride_1_const_u16, conv2d_22_l_stride_0_const_u16, conv2d_22_t_weight_1_ptr_const_s32, conv2d_22_t_in_0_fmt_zero_const_s8, conv2d_22_t_out_0_fmt_zero_const_s8, conv2d_22_t_in_0_fmt_scale_const_f32, conv2d_22_t_out_0_fmt_scale_const_f32, conv2d_22_t_weight_0_fmt_scale_const_f32, conv2d_22_t_out_0_ptr_s8, conv2d_22_t_out_0_shape_w_const_u16, conv2d_22_t_out_0_shape_h_const_u16, 0, 4737, conv2d_22_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(22, 1, {(stai_ptr) conv2d_22_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_22 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_23 */
  {
    
  forward_lite_conv2d_23(net_ctx);
  }
  /* LITE_KERNEL_SECTION END conv2d_23 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_29 */
  {
      const ai_i8* conv2d_29_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 3456);
    const ai_i8* conv2d_29_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[46] + 0);
    const ai_i32* conv2d_29_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[47] + 0);
    ai_i8* conv2d_29_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1124);
    ai_i16* conv2d_29_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(29, 1, {(stai_ptr) conv2d_29_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_29_t_in_0_ptr_const_s8, conv2d_29_t_in_0_shape_w_const_u16, conv2d_29_t_in_0_shape_h_const_u16, conv2d_29_l_stride_1_const_u16, conv2d_29_l_stride_0_const_u16, conv2d_29_t_in_0_shape_ch_const_u16, conv2d_29_t_weight_0_ptr_const_s8, conv2d_29_t_out_0_shape_ch_const_u16, conv2d_29_t_weight_1_ptr_const_s32, conv2d_29_t_in_0_fmt_zero_const_s8, conv2d_29_t_out_0_fmt_zero_const_s8, conv2d_29_t_in_0_fmt_scale_const_f32, conv2d_29_t_out_0_fmt_scale_const_f32, conv2d_29_t_weight_0_fmt_scale_const_f32, conv2d_29_l_out_ch_format_const_layer_format_type, conv2d_29_t_out_0_ptr_s8, 1, 1124, conv2d_29_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(29, 1, {(stai_ptr) conv2d_29_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_29 */
  /* LITE_KERNEL_SECTION BEGIN nl_30 */
  {
      ai_i8* nl_30_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 496);
    const ai_i8* nl_30_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1124);
    ai_i32* nl_30_t_scratch_0_ptr_s32 = (ai_i32*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(30, 1, {(stai_ptr) nl_30_t_in_0_ptr_const_s8});
    
  forward_lite_nl_softmax_is8os8(nl_30_t_out_0_ptr_s8, nl_30_t_in_0_ptr_const_s8, nl_30_t_in_0_shape_ch_h_w_prod_const_u32, 1, 10, 1775826944, 24, -124, nl_30_t_scratch_0_ptr_s32);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(30, 1, {(stai_ptr) nl_30_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END nl_30 */
  /* LITE_KERNEL_SECTION BEGIN conversion_35 */
  {
      const ai_i8* conversion_35_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 496);
    ai_float* conversion_35_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_outputs[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(35, 1, {(stai_ptr) conversion_35_t_in_0_ptr_const_s8});
    
  forward_lite_node_convert_integer_is8of32(conversion_35_t_in_0_ptr_const_s8, conversion_35_t_out_0_ptr_f32, conversion_35_t_out_0_shape_h_w_ch_d_prod_const_u32, conversion_35_t_in_0_fmt_scale_const_f32, conversion_35_t_in_0_fmt_zero_const_s8);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(35, 1, {(stai_ptr) conversion_35_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END conversion_35 */
  return net_ctx->_return_code;
}

/*****************************************************************************/
/*  Getters APIs Section  */
STAI_API_ENTRY
stai_size stai_network_get_context_size()
{
  return (stai_size)STAI_NETWORK_CONTEXT_SIZE;
}

#if defined(HAVE_NETWORK_INFO)
STAI_API_ENTRY
stai_return_code stai_network_get_info(
  stai_network* network,
  stai_network_info* info)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, info==NULL, STAI_ERROR_NETWORK_INVALID_INFO, net_ctx->_return_code)

  // Copy of network info struct
  *info = g_network_info;

  return STAI_SUCCESS;
}
#endif


STAI_API_ENTRY
stai_return_code stai_network_get_activations(
  stai_network* network, stai_ptr* activations, stai_size* n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, !n_activations, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_activations = STAI_NETWORK_ACTIVATIONS_NUM;
for (stai_size idx=0; activations && (idx<STAI_NETWORK_ACTIVATIONS_NUM); idx++) {
    // get address of the activations buffers
    activations[idx] = net_ctx->_activations[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_weights(
  stai_network* network, stai_ptr* weights, stai_size* n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_weights, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_weights = STAI_NETWORK_WEIGHTS_NUM;
for (stai_size idx=0; weights && (idx<STAI_NETWORK_WEIGHTS_NUM); idx++) {
    // get address of the weights buffers
    weights[idx] = net_ctx->_weights[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_inputs(
  stai_network* network, stai_ptr* inputs, stai_size* n_inputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_inputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_inputs = STAI_NETWORK_IN_NUM;
  for (stai_size idx=0; inputs && (idx<STAI_NETWORK_IN_NUM); idx++) {
    inputs[idx] = net_ctx->_inputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_outputs(
  stai_network* network, stai_ptr* outputs, stai_size* n_outputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_outputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_outputs = STAI_NETWORK_OUT_NUM;
  for (stai_size idx=0; outputs && (idx<STAI_NETWORK_OUT_NUM); idx++) {
    outputs[idx] = net_ctx->_outputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_error(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /* return 1st generated error or STAI_SUCCESS if no errors so far */
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_states(
  stai_network* network, stai_ptr* states, stai_size* n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_states, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  /* get the number of internals states (supporting multi-heap also for internal states) */
  *n_states = STAI_NETWORK_STATES_NUM;

  STAI_UNUSED(states)
return net_ctx->_return_code;
}


/*****************************************************************************/
/*  Setters APIs Section  */

STAI_API_ENTRY
stai_return_code stai_network_set_activations(
  stai_network* network,
  const stai_ptr* activations,
  const stai_size n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _activations_alignment[] = STAI_NETWORK_ACTIVATIONS_ALIGNMENTS;
  STAI_PRINT("  [stai_network_set_activations] network(%p) activations[%d]: %p\n\n", net_ctx, n_activations, activations)
  _STAI_SET_ERROR(net_ctx, !activations,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_activations!=STAI_NETWORK_ACTIVATIONS_NUM,
                  STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_NUM, net_ctx->_return_code)

  for (stai_size idx=0; activations && idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    STAI_PRINT("  activation[%d]: %p\n", idx, activations[idx])
    _STAI_SET_ERROR(net_ctx, activations[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)activations[idx]) & (_activations_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_activations[idx] = activations[idx];
  }
  net_ctx->_inputs[0] = activations[0] + 0;

  net_ctx->_outputs[0] = activations[0] + 0;
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_weights(
  stai_network* network,
  const stai_ptr* weights,
  const stai_size n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _weights_alignment[] = STAI_NETWORK_WEIGHTS_ALIGNMENTS;
  _STAI_SET_ERROR(net_ctx, !weights,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_weights!=STAI_NETWORK_WEIGHTS_NUM,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_NUM, net_ctx->_return_code)
  for (stai_size idx=0; weights && idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    STAI_PRINT("  weight[%d]: %p\n", idx, weights[idx])
    _STAI_SET_ERROR(net_ctx, weights[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)weights[idx]) & (_weights_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_weights[idx] = weights[idx];
  }_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_inputs(
  stai_network* network,
  const stai_ptr* inputs,
  const stai_size n_inputs)
{
  const uintptr_t _inputs_alignment[] = STAI_NETWORK_IN_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !inputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_inputs!=STAI_NETWORK_IN_NUM,
                  STAI_ERROR_NETWORK_INVALID_IN_NUM, net_ctx->_return_code)

  for (stai_size idx=0; inputs && idx<STAI_NETWORK_IN_NUM; idx++) {
    STAI_PRINT("  input[%d]: %p\n", idx, inputs[idx])
    _STAI_SET_ERROR(net_ctx, inputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)inputs[idx]) & (_inputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_inputs[idx] = inputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_outputs(
  stai_network* network,
  const stai_ptr* outputs,
  const stai_size n_outputs)
{
  const uintptr_t _outputs_alignment[] = STAI_NETWORK_OUT_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !outputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_outputs!=STAI_NETWORK_OUT_NUM,
                  STAI_ERROR_NETWORK_INVALID_OUT_NUM, net_ctx->_return_code)

  for (stai_size idx=0; outputs && idx<n_outputs; idx++) {
    STAI_PRINT("  output[%d]: %p\n", idx, outputs[idx])
    _STAI_SET_ERROR(net_ctx, outputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)outputs[idx]) & (_outputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_outputs[idx] = outputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_states(
  stai_network* network,
  const stai_ptr* states,
  const stai_size n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  STAI_UNUSED(states)
  STAI_UNUSED(n_states)
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}

STAI_API_ENTRY
stai_return_code stai_network_set_callback(
  stai_network* network, const stai_event_cb cb, void* cb_cookie)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  STAI_PRINT("  set_callback %p cb %p cookie %p\n", net_ctx, cb, cb_cookie)
  // _STAI_SET_ERROR(net_ctx, cb==NULL, STAI_ERROR_NETWORK_INVALID_CALLBACK, net_ctx->_return_code)
  net_ctx->_callback = cb;
  net_ctx->_callback_cookie = cb_cookie;
  return net_ctx->_return_code;
}

#undef _STAI_SET_ERROR
#undef _STAI_CONTEXT_ALIGNMENT
#undef _STAI_CONTEXT_ACQUIRE
#undef _STAI_NETWORK_EVENT_NODE_START_CB
#undef _STAI_NETWORK_EVENT_NODE_STOP_CB
#undef _STAI_NETWORK_MODEL_SIGNATURE
#undef _STAI_NETWORK_DATETIME
#undef _STAI_NETWORK_COMPILE_DATETIME

