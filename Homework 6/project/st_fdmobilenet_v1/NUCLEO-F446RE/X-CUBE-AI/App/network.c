/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2025-12-31T12:38:51+0000
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
#define _STAI_NETWORK_MODEL_SIGNATURE     "0x6ae6379491dd57665b5d1728f5145be8"
#define _STAI_NETWORK_DATETIME            "2025-12-31T12:38:51+0000"
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
      STAI_DECLARE_ARRAY(int32_t, 1, 432),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_2_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_2_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_3_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_3_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 144),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_4_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_4_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 64),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_5_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_5_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 768),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_6_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_6_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 192),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_7_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_7_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 432),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_8_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_8_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 192),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_9_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_9_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 3840),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_10_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_10_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 320),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_11_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_11_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 720),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_12_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_12_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 320),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_13_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_13_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 5120),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_14_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_14_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_15_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_15_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 576),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_16_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_16_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 256),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_17_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_17_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 6144),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_18_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_18_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_19_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_19_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 864),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_20_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_20_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_21_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_21_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 6144),
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
    AI_PACK_INTQ_SCALE(0.006158170755952597f),
    AI_PACK_INTQ_ZP(50)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_23_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.025286104530096054f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_23_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 256,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.01567741297185421f, 0.01778411492705345f, 0.01340076606720686f, 0.020010432228446007f, 0.014859097078442574f, 0.014757970348000526f, 0.022401701658964157f, 0.017015649005770683f, 0.01517435535788536f, 0.01481655053794384f, 0.01582260988652706f, 0.01947639137506485f, 0.01431360188871622f, 0.01607598550617695f, 0.01573300175368786f, 0.014884659089148045f, 0.018693996593356133f, 0.016586342826485634f, 0.012817095965147018f, 0.021113386377692223f, 0.0204814113676548f, 0.016860518604516983f, 0.02545466646552086f, 0.014745033346116543f, 0.017915286123752594f, 0.01643882878124714f, 0.020845701918005943f, 0.019350819289684296f, 0.013208524323999882f, 0.017526375129818916f, 0.014811690896749496f, 0.014108623377978802f, 0.017383038997650146f, 0.014579708687961102f, 0.01890482008457184f, 0.015838447958230972f, 0.014757957309484482f, 0.019444137811660767f, 0.015012141317129135f, 0.014206405729055405f, 0.017142081633210182f, 0.020210299640893936f, 0.014665106311440468f, 0.015835674479603767f, 0.013481228612363338f, 0.022476037964224815f, 0.0173479150980711f, 0.015938110649585724f, 0.015666360035538673f, 0.013265923596918583f, 0.014439131133258343f, 0.01227035466581583f, 0.02203979901969433f, 0.018949633464217186f, 0.017589299008250237f, 0.015493634156882763f, 0.013386378064751625f, 0.013034122996032238f, 0.01874277926981449f, 0.012385781854391098f, 0.014136172831058502f, 0.014468819834291935f, 0.012270917184650898f, 0.015051677823066711f, 0.016078753396868706f, 0.01811078190803528f, 0.01693836972117424f, 0.01585838571190834f, 0.018895072862505913f, 0.015550995245575905f, 0.0124442083761096f, 0.01569209434092045f, 0.013506432995200157f, 0.01660175621509552f, 0.01714363321661949f, 0.018182678148150444f, 0.016166502609848976f, 0.018655749037861824f, 0.013083243742585182f, 0.01674165576696396f, 0.015636252239346504f, 0.015354064293205738f, 0.01552227046340704f, 0.013880344107747078f, 0.013646150939166546f, 0.016438527032732964f, 0.01764834299683571f, 0.012906842865049839f, 0.013823728077113628f, 0.015898779034614563f, 0.015890078619122505f, 0.015100477263331413f, 0.016314202919602394f, 0.013103022240102291f, 0.0173040758818388f, 0.01249806396663189f, 0.017103636637330055f, 0.020997626706957817f, 0.014390967786312103f, 0.01895342580974102f, 0.020977621898055077f, 0.01814204268157482f, 0.019605213776230812f, 0.019387003034353256f, 0.014195632189512253f, 0.016776148229837418f, 0.013838817365467548f, 0.01726537011563778f, 0.022318972274661064f, 0.01958433911204338f, 0.012226097285747528f, 0.012860872782766819f, 0.013750511221587658f, 0.018097417429089546f, 0.023562217131257057f, 0.015862498432397842f, 0.01335990708321333f, 0.018976431339979172f, 0.011677087284624577f, 0.016461379826068878f, 0.013030298985540867f, 0.017758291214704514f, 0.01381661742925644f, 0.013913686387240887f, 0.015351939015090466f, 0.021760547533631325f, 0.019450491294264793f, 0.014117510989308357f, 0.013321935199201107f, 0.01737498864531517f, 0.01414113026112318f, 0.015940628945827484f, 0.014628644101321697f, 0.015226013958454132f, 0.014650917612016201f, 0.01740274205803871f, 0.014648670330643654f, 0.017445780336856842f, 0.0173209086060524f, 0.017137348651885986f, 0.013344145379960537f, 0.012951742857694626f, 0.012251721695065498f, 0.015027373097836971f, 0.013259118422865868f, 0.017087409272789955f, 0.01300628948956728f, 0.019007554277777672f, 0.01800563372671604f, 0.02295219711959362f, 0.012484255246818066f, 0.016491200774908066f, 0.015124128200113773f, 0.015073631890118122f, 0.014356343075633049f, 0.015307161957025528f, 0.013987583108246326f, 0.016052620485424995f, 0.015226965770125389f, 0.014471885748207569f, 0.01621493697166443f, 0.014866934157907963f, 0.01504211500287056f, 0.018466271460056305f, 0.012715226970613003f, 0.015802577137947083f, 0.013212204910814762f, 0.018644999712705612f, 0.013326660729944706f, 0.01961871050298214f, 0.012616204097867012f, 0.014301935210824013f, 0.020350381731987f, 0.013818083330988884f, 0.014488640241324902f, 0.01929941028356552f, 0.012999171391129494f, 0.013944569043815136f, 0.014104641042649746f, 0.020378749817609787f, 0.022031808272004128f, 0.017571687698364258f, 0.014909487217664719f, 0.019050536677241325f, 0.016018886119127274f, 0.0165680143982172f, 0.017112351953983307f, 0.018195031210780144f, 0.015725238248705864f, 0.01361051481217146f, 0.012670640833675861f, 0.014491872861981392f, 0.015180390328168869f, 0.01730458252131939f, 0.017010880634188652f, 0.016279835253953934f, 0.011782939545810223f, 0.015688220039010048f, 0.01625688746571541f, 0.01776716485619545f, 0.015406451188027859f, 0.014342406764626503f, 0.017267361283302307f, 0.015890711918473244f, 0.01821877621114254f, 0.01179588120430708f, 0.013767837546765804f, 0.01445890311151743f, 0.016742218285799026f, 0.015366875566542149f, 0.018034426495432854f, 0.019499292597174644f, 0.01789802871644497f, 0.01314315851777792f, 0.017845312133431435f, 0.015466578304767609f, 0.014474794268608093f, 0.01572941429913044f, 0.013297410681843758f, 0.011610898189246655f, 0.019618071615695953f, 0.011447686702013016f, 0.015799785032868385f, 0.016110096126794815f, 0.014891893602907658f, 0.023013439029455185f, 0.015816643834114075f, 0.015354407951235771f, 0.016389600932598114f, 0.019463416188955307f, 0.01370922476053238f, 0.015388292260468006f, 0.015552124008536339f, 0.013587893918156624f, 0.013067178428173065f, 0.01650223508477211f, 0.02193925902247429f, 0.016241367906332016f, 0.014870677143335342f, 0.02144484408199787f, 0.016375862061977386f, 0.012739399448037148f, 0.014171747490763664f, 0.018703630194067955f, 0.02160804159939289f, 0.012391054071485996f, 0.012802577577531338f, 0.01273279171437025f, 0.01989804580807686f, 0.01157724391669035f, 0.015949981287121773f, 0.01487068459391594f, 0.01528529729694128f, 0.01754571683704853f, 0.012688178569078445f, 0.017938392236828804f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_23_scratch1_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.025286104530096054f),
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
static const ai_u16 conv2d_1_t_out_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_1_t_weight_0_shape_w_const_u16 = 3;
static const ai_i32 conv2d_1_l_pad_W_0_const_s32 = 0;
static const ai_u16 conv2d_1_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_1_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_1_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32 = 0.003921568859368563f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.019122522324323654f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009279884397983551f, 0.004976786673069f, 0.007554265670478344f, 0.005906743463128805f, 0.005225418601185083f, 0.012882004491984844f, 0.008672245778143406f, 0.009221887215971947f, 0.0053591299802064896f, 0.012522861361503601f, 0.004150932654738426f, 0.008563474752008915f, 0.004513331223279238f, 0.0038824963849037886f, 0.008531829342246056f, 0.004767866339534521f);
static const ai_layer_format_type conv2d_1_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 16;

static const ai_u16 conv2d_2_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_2_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_2_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_2_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_2_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_2_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_2_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_2_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_2_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_2_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_2_t_out_0_fmt_zero_const_s8 = -14;
static const ai_float conv2d_2_t_in_0_fmt_scale_const_f32 = 0.019122522324323654f;
static const ai_float conv2d_2_t_out_0_fmt_scale_const_f32 = 0.018843552097678185f;
static const ai_float conv2d_2_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0026929464656859636f, 0.002319450955837965f, 0.0014644830953329802f, 0.0019243656424805522f, 0.0019290696363896132f, 0.002960881916806102f, 0.0020030909217894077f, 0.0019799352157860994f, 0.0017227695789188147f, 0.001654862193390727f, 0.00208746874704957f, 0.0018324400298297405f, 0.0019550009164959192f, 0.002649919595569372f, 0.0011994285741820931f, 0.002085855696350336f);
static const ai_u16 conv2d_2_t_out_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_2_t_out_0_shape_h_const_u16 = 8;

static const ai_u16 conv2d_3_t_in_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_3_t_in_0_shape_h_const_u16 = 8;
static const ai_u16 conv2d_3_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_3_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_3_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_3_t_out_0_shape_ch_const_u16 = 48;
static const ai_i8 conv2d_3_t_in_0_fmt_zero_const_s8 = -14;
static const ai_i8 conv2d_3_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_3_t_in_0_fmt_scale_const_f32 = 0.018843552097678185f;
static const ai_float conv2d_3_t_out_0_fmt_scale_const_f32 = 0.02303696796298027f;
static const ai_float conv2d_3_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009209677577018738f, 0.011587176471948624f, 0.01272729504853487f, 0.012164182029664516f, 0.008445455692708492f, 0.017546774819493294f, 0.01621243543922901f, 0.01326650008559227f, 0.009079121053218842f, 0.016764508560299873f, 0.010135937482118607f, 0.015883471816778183f, 0.012417004443705082f, 0.017042648047208786f, 0.01501714251935482f, 0.01060060877352953f, 0.011553018353879452f, 0.012699613347649574f, 0.007886834442615509f, 0.006959649734199047f, 0.012860432267189026f, 0.011641357094049454f, 0.011976739391684532f, 0.013632729649543762f, 0.01114705204963684f, 0.009222324937582016f, 0.014205488376319408f, 0.012715916149318218f, 0.01055640447884798f, 0.008908098563551903f, 0.012663072906434536f, 0.008998040109872818f, 0.012931362725794315f, 0.01849488727748394f, 0.010032635182142258f, 0.01141892559826374f, 0.010295836254954338f, 0.015629863366484642f, 0.007511300500482321f, 0.012586517259478569f, 0.01679147593677044f, 0.013129329308867455f, 0.016677720472216606f, 0.01584002934396267f, 0.009423254989087582f, 0.01097114011645317f, 0.010149302892386913f, 0.010712234303355217f);
static const ai_layer_format_type conv2d_3_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_4_t_in_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_4_t_in_0_shape_h_const_u16 = 8;
static const ai_u16 conv2d_4_t_in_0_shape_ch_const_u16 = 48;
static const ai_u16 conv2d_4_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_4_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_4_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_4_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_4_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_4_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_4_t_out_0_fmt_zero_const_s8 = 15;
static const ai_float conv2d_4_t_in_0_fmt_scale_const_f32 = 0.02303696796298027f;
static const ai_float conv2d_4_t_out_0_fmt_scale_const_f32 = 0.01784505695104599f;
static const ai_float conv2d_4_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0014504551654681563f, 0.0014662882313132286f, 0.001959471730515361f, 0.0014623238239437342f, 0.0012113420525565743f, 0.001962267793715f, 0.00147643918171525f, 0.0013972921296954155f, 0.0012312546605244279f, 0.0017668802756816149f, 0.001600958639755845f, 0.0018755835480988026f, 0.0016953236190602183f, 0.0020150067284703255f, 0.0013380300952121615f, 0.0012906847987324f, 0.0015038507990539074f, 0.0015330200549215078f, 0.0012390578631311655f, 0.0014606713084504008f, 0.002123198937624693f, 0.0018120568711310625f, 0.001428584335371852f, 0.0012012466322630644f, 0.0013861559564247727f, 0.0015390017069876194f, 0.0013295760145410895f, 0.0018703608075156808f, 0.001243561739102006f, 0.001395310740917921f, 0.0014743689680472016f, 0.001052861218340695f, 0.001381739042699337f, 0.0014998088590800762f, 0.0009608211694285274f, 0.0018843428697437048f, 0.0011027660220861435f, 0.0017375805182382464f, 0.0013979069190099835f, 0.0019410401582717896f, 0.0015402365243062377f, 0.0011302492348477244f, 0.0015907621709629893f, 0.0016221955884248018f, 0.0011546058813109994f, 0.0011102157877758145f, 0.0012197182513773441f, 0.0012470863293856382f);
static const ai_u16 conv2d_4_t_out_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_4_t_out_0_shape_h_const_u16 = 4;

static const ai_u16 conv2d_5_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_5_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_5_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_5_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_5_t_in_0_shape_ch_const_u16 = 48;
static const ai_u16 conv2d_5_t_out_0_shape_ch_const_u16 = 80;
static const ai_i8 conv2d_5_t_in_0_fmt_zero_const_s8 = 15;
static const ai_i8 conv2d_5_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_5_t_in_0_fmt_scale_const_f32 = 0.01784505695104599f;
static const ai_float conv2d_5_t_out_0_fmt_scale_const_f32 = 0.022118549793958664f;
static const ai_float conv2d_5_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009899085387587547f, 0.008495235815644264f, 0.008868479169905186f, 0.006551900412887335f, 0.00867350772023201f, 0.00741857523098588f, 0.009725288487970829f, 0.00789372343569994f, 0.00827604066580534f, 0.008912107907235622f, 0.00781252421438694f, 0.011856671422719955f, 0.010067661292850971f, 0.00926484540104866f, 0.009464666247367859f, 0.009464609436690807f, 0.007411205675452948f, 0.007272735703736544f, 0.005993823520839214f, 0.006733101326972246f, 0.008242448791861534f, 0.008810846135020256f, 0.009186943992972374f, 0.006875323597341776f, 0.006933155003935099f, 0.007637560833245516f, 0.006991091184318066f, 0.007888449355959892f, 0.0064367144368588924f, 0.008403368294239044f, 0.005808871239423752f, 0.008096584118902683f, 0.010473241098225117f, 0.009571445174515247f, 0.008778778836131096f, 0.007795966230332851f, 0.005925110075622797f, 0.006226109340786934f, 0.010633179917931557f, 0.007213398814201355f, 0.008676591329276562f, 0.006842330098152161f, 0.013628046028316021f, 0.009782298468053341f, 0.009810962714254856f, 0.007612261921167374f, 0.009975322522222996f, 0.009344699792563915f, 0.006379741709679365f, 0.009609194472432137f, 0.008285527117550373f, 0.0069325948134064674f, 0.008885692805051804f, 0.010627750307321548f, 0.008403445594012737f, 0.009313070215284824f, 0.010475683957338333f, 0.006423344369977713f, 0.007824950851500034f, 0.009484507143497467f, 0.008588956668972969f, 0.012073769234120846f, 0.009392169304192066f, 0.007994483225047588f, 0.010891194455325603f, 0.00789274275302887f, 0.009779687970876694f, 0.00832225102931261f, 0.00842755101621151f, 0.012740311212837696f, 0.010740098543465137f, 0.00991553533822298f, 0.00889558345079422f, 0.009495334699749947f, 0.008084231056272984f, 0.009185525588691235f, 0.009314569644629955f, 0.0087122255936265f, 0.007191832177340984f, 0.010286745615303516f);
static const ai_layer_format_type conv2d_5_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_6_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_6_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_6_t_in_0_shape_ch_const_u16 = 80;
static const ai_u16 conv2d_6_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_6_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_6_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_6_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_6_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_6_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_6_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_6_t_out_0_fmt_zero_const_s8 = -20;
static const ai_float conv2d_6_t_in_0_fmt_scale_const_f32 = 0.022118549793958664f;
static const ai_float conv2d_6_t_out_0_fmt_scale_const_f32 = 0.012010117992758751f;
static const ai_float conv2d_6_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0012696539051830769f, 0.002198573900386691f, 0.0016416726866737008f, 0.0013872628333047032f, 0.0010925859678536654f, 0.001205403241328895f, 0.0015456476248800755f, 0.0012098910519853234f, 0.001613952568732202f, 0.0016252623172476888f, 0.0012487549101933837f, 0.0013571185991168022f, 0.0018136311555281281f, 0.0017914909403771162f, 0.0013910058187320828f, 0.0013061435893177986f, 0.001575756585225463f, 0.0014518905663862824f, 0.0013340642908588052f, 0.0014165114844217896f, 0.0013631945475935936f, 0.001401726738549769f, 0.0011379299685359001f, 0.001366928219795227f, 0.0016644283896312118f, 0.0009845142485573888f, 0.0015273959143087268f, 0.0010030849371105433f, 0.001612426363863051f, 0.0014930005418136716f, 0.0013923635706305504f, 0.0014226913917809725f, 0.002082215389236808f, 0.0014048500452190638f, 0.0017275373684242368f, 0.0009308800799772143f, 0.001280702999792993f, 0.0011570972856134176f, 0.0015013616066426039f, 0.001178168342448771f, 0.0015753208426758647f, 0.001129287644289434f, 0.0015653271693736315f, 0.0012996880104765296f, 0.0011061298428103328f, 0.0012646738905459642f, 0.001674806117080152f, 0.0012306150747463107f, 0.000959551427513361f, 0.001121091772802174f, 0.00189900619443506f, 0.0011914849746972322f, 0.0014403124805539846f, 0.0012166575761511922f, 0.0016450779512524605f, 0.0016276993555948138f, 0.0012615221785381436f, 0.0010675477096810937f, 0.0014098059618845582f, 0.0013142608804628253f, 0.0014969639014452696f, 0.0009837985271587968f, 0.0016988512361422181f, 0.0017346274107694626f, 0.0012430943315848708f, 0.0014829594874754548f, 0.0013075125170871615f, 0.001866801525466144f, 0.0013657803647220135f, 0.001510112197138369f, 0.0017554243095219135f, 0.0014582118019461632f, 0.000920023419894278f, 0.0013976602349430323f, 0.0018844426376745105f, 0.0019142113160341978f, 0.0012124700006097555f, 0.0011880462989211082f, 0.0010359573643654585f, 0.001528001157566905f);
static const ai_u16 conv2d_6_t_out_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_6_t_out_0_shape_h_const_u16 = 4;

static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_7_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_7_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16 = 80;
static const ai_u16 conv2d_7_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8 = -20;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32 = 0.012010117992758751f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32 = 0.023622021079063416f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.011957679875195026f, 0.008970660157501698f, 0.013722723349928856f, 0.009325241670012474f, 0.01127774640917778f, 0.007700646296143532f, 0.01126782689243555f, 0.010174663737416267f, 0.007731154095381498f, 0.013215669430792332f, 0.008734479546546936f, 0.010225186124444008f, 0.011908324435353279f, 0.009724358096718788f, 0.009861988015472889f, 0.0118757588788867f, 0.01231211144477129f, 0.009384503588080406f, 0.011231756769120693f, 0.010323040187358856f, 0.011003581807017326f, 0.010025466792285442f, 0.013086363673210144f, 0.008610520511865616f, 0.012214120477437973f, 0.012053251266479492f, 0.011254321783781052f, 0.010527793318033218f, 0.012339357286691666f, 0.010455216281116009f, 0.00981536228209734f, 0.009779322892427444f, 0.015289991162717342f, 0.012078672647476196f, 0.010859969072043896f, 0.011206700466573238f, 0.010505177080631256f, 0.011545059271156788f, 0.009863743558526039f, 0.00940290279686451f, 0.011911299079656601f, 0.011618571355938911f, 0.011672320775687695f, 0.010115954093635082f, 0.009569727815687656f, 0.010107760317623615f, 0.012865987606346607f, 0.00840715877711773f, 0.009334061294794083f, 0.011936589144170284f, 0.01012786291539669f, 0.011549068614840508f, 0.009582688100636005f, 0.010698397643864155f, 0.009545853361487389f, 0.009431642480194569f, 0.008951576426625252f, 0.008858638815581799f, 0.009547804482281208f, 0.009571938775479794f, 0.010322167538106441f, 0.00880361907184124f, 0.00968177616596222f, 0.010551974177360535f);
static const ai_layer_format_type conv2d_7_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_8_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_8_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_8_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_8_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_8_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_8_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8 = -5;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32 = 0.023622021079063416f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32 = 0.009520532563328743f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002102791564539075f, 0.001313817105256021f, 0.0012103358749300241f, 0.0012586716329678893f, 0.001252664253115654f, 0.0013796851271763444f, 0.001608164981007576f, 0.001439155195839703f, 0.0014627641066908836f, 0.0015212181024253368f, 0.0016852718545123935f, 0.0016841772012412548f, 0.0014434114564210176f, 0.0017146450700238347f, 0.0012016997206956148f, 0.0014150638598948717f, 0.0013373714173212647f, 0.0015725493431091309f, 0.0018487712368369102f, 0.0014997405232861638f, 0.0013714353553950787f, 0.0016194804338738322f, 0.0009980130707845092f, 0.001249185879714787f, 0.0015789475291967392f, 0.0011624176986515522f, 0.001344397896900773f, 0.0013836434809491038f, 0.0011224134359508753f, 0.0019581241067498922f, 0.0010847499361261725f, 0.0010115656768903136f, 0.0011077356757596135f, 0.0017356370808556676f, 0.0012578474124893546f, 0.0013731883373111486f, 0.0010275942040607333f, 0.0017045127460733056f, 0.001436132937669754f, 0.001254432718269527f, 0.0014252765104174614f, 0.001601517666131258f, 0.0015647388063371181f, 0.0013501574285328388f, 0.0009668157435953617f, 0.0016275706002488732f, 0.0016694102669134736f, 0.0015486711636185646f, 0.0009319881210103631f, 0.0013824563939124346f, 0.0020372821018099785f, 0.0011521718697622418f, 0.0013392983237281442f, 0.0015439849812537432f, 0.0013750405050814152f, 0.0013645004946738482f, 0.001208584988489747f, 0.0015331433387473226f, 0.001453905482776463f, 0.0012214106973260641f, 0.00109065230935812f, 0.0012657304760068655f, 0.0013081042561680079f, 0.0014608013443648815f);
static const ai_u16 conv2d_8_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_8_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_9_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_9_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_9_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_9_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_9_t_out_0_shape_ch_const_u16 = 96;
static const ai_i8 conv2d_9_t_in_0_fmt_zero_const_s8 = -5;
static const ai_i8 conv2d_9_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_9_t_in_0_fmt_scale_const_f32 = 0.009520532563328743f;
static const ai_float conv2d_9_t_out_0_fmt_scale_const_f32 = 0.02021421119570732f;
static const ai_float conv2d_9_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.013999557122588158f, 0.016020821407437325f, 0.01617616042494774f, 0.012144222855567932f, 0.011485611088573933f, 0.01160335261374712f, 0.01194753497838974f, 0.011337775737047195f, 0.012313750572502613f, 0.012294684536755085f, 0.011973832733929157f, 0.01223771832883358f, 0.01089848205447197f, 0.009841647930443287f, 0.01363796554505825f, 0.01842305436730385f, 0.008051740936934948f, 0.010669417679309845f, 0.012621498666703701f, 0.011806230992078781f, 0.014017839916050434f, 0.012526711449027061f, 0.012982466258108616f, 0.010223820805549622f, 0.01045973040163517f, 0.015492185018956661f, 0.010887276381254196f, 0.01090982649475336f, 0.009677039459347725f, 0.01037291344255209f, 0.00804702565073967f, 0.011200618930161f, 0.010063167661428452f, 0.012677036225795746f, 0.012004191055893898f, 0.015415916219353676f, 0.012336990796029568f, 0.009659954346716404f, 0.011296902783215046f, 0.011518380604684353f, 0.016157470643520355f, 0.012799067422747612f, 0.012123174034059048f, 0.01261542085558176f, 0.013277123682200909f, 0.013943995349109173f, 0.009368953295052052f, 0.009825305081903934f, 0.01211890485137701f, 0.010935262776911259f, 0.011549601331353188f, 0.01298542134463787f, 0.014214928261935711f, 0.01141839288175106f, 0.01250796765089035f, 0.011329030618071556f, 0.009457314386963844f, 0.014699362218379974f, 0.010613451711833477f, 0.01267666183412075f, 0.009768090210855007f, 0.011108742095530033f, 0.01055052224546671f, 0.01153402216732502f, 0.015129604376852512f, 0.01299983635544777f, 0.01134785357862711f, 0.009507608599960804f, 0.011649897322058678f, 0.012880985625088215f, 0.01324810367077589f, 0.010393484495580196f, 0.009508886374533176f, 0.011545597575604916f, 0.010653374716639519f, 0.009024140425026417f, 0.01085616834461689f, 0.008834056556224823f, 0.009556084871292114f, 0.016709744930267334f, 0.009159360080957413f, 0.01039295457303524f, 0.014735045842826366f, 0.007189307827502489f, 0.01130636502057314f, 0.009328311309218407f, 0.012735236436128616f, 0.013565720990300179f, 0.013111716136336327f, 0.016026601195335388f, 0.014493118040263653f, 0.01315183937549591f, 0.01120823249220848f, 0.012809811159968376f, 0.010648325085639954f, 0.017472803592681885f);
static const ai_layer_format_type conv2d_9_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_10_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_10_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_10_t_in_0_shape_ch_const_u16 = 96;
static const ai_u16 conv2d_10_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_10_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_10_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_10_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_10_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_10_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_10_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_10_t_out_0_fmt_zero_const_s8 = 13;
static const ai_float conv2d_10_t_in_0_fmt_scale_const_f32 = 0.02021421119570732f;
static const ai_float conv2d_10_t_out_0_fmt_scale_const_f32 = 0.00880748312920332f;
static const ai_float conv2d_10_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0014798552729189396f, 0.0010755283292382956f, 0.0009029687498696148f, 0.0013933482114225626f, 0.0014204384060576558f, 0.0015242287190631032f, 0.001926420838572085f, 0.0014390754513442516f, 0.0010558958165347576f, 0.0009616492316126823f, 0.0010012299753725529f, 0.0015275164041668177f, 0.0013395065907388926f, 0.0011587579501792789f, 0.0012268199352547526f, 0.0012570552062243223f, 0.000802840106189251f, 0.0014052089536562562f, 0.0012266946723684669f, 0.0014321146300062537f, 0.0014112811768427491f, 0.0009027612395584583f, 0.0018791044130921364f, 0.001402714173309505f, 0.0010408652015030384f, 0.0015877350233495235f, 0.0009380077244713902f, 0.001972238766029477f, 0.0016210523899644613f, 0.0014396829064935446f, 0.001447012647986412f, 0.0017401897348463535f, 0.001018805312924087f, 0.0009166739764623344f, 0.001152258599177003f, 0.0011336278403177857f, 0.0016558007337152958f, 0.0010493657318875194f, 0.001331663690507412f, 0.001052161562256515f, 0.0014433792093768716f, 0.0013197787338867784f, 0.0014418821083381772f, 0.0009316877694800496f, 0.0012481090379878879f, 0.001079175970517099f, 0.0011756728636100888f, 0.0017510164761915803f, 0.0011661782627925277f, 0.0013469780096784234f, 0.0014351712306961417f, 0.0011995850363746285f, 0.0014850915176793933f, 0.0013031252892687917f, 0.0013098245253786445f, 0.00087972596520558f, 0.0011389892315492034f, 0.00150300282984972f, 0.0010391896357759833f, 0.0005678327288478613f, 0.0011961476411670446f, 0.0015118903247639537f, 0.0012277000350877643f, 0.0011474561179056764f, 0.0008774244925007224f, 0.0012653530575335026f, 0.0007232032949104905f, 0.0011341475183144212f, 0.0012438028352335095f, 0.0012848610058426857f, 0.0012764494167640805f, 0.0016885633813217282f, 0.0012874098028987646f, 0.0016043398063629866f, 0.0012183953076601028f, 0.0015424324665218592f, 0.001457839272916317f, 0.001349353464320302f, 0.001587263192050159f, 0.0009244967950507998f, 0.0009921687887981534f, 0.0009666744153946638f, 0.0017319961916655302f, 0.0018396038794890046f, 0.0015255932230502367f, 0.0011550507042557001f, 0.0010130625450983644f, 0.0013870879774913192f, 0.0010698604164645076f, 0.001981856534257531f, 0.0012531662359833717f, 0.0008266199147328734f, 0.0013959795469418168f, 0.0015490147052332759f, 0.0015266346745193005f, 0.001195278367958963f);
static const ai_u16 conv2d_10_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_10_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_11_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_11_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 96;
static const ai_u16 conv2d_11_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = 13;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.00880748312920332f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.017682315781712532f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.012560740113258362f, 0.01314158458262682f, 0.010626564733684063f, 0.011260783299803734f, 0.011502002365887165f, 0.009437868371605873f, 0.013010204769670963f, 0.008779156021773815f, 0.009073765948414803f, 0.012105352245271206f, 0.012980596162378788f, 0.009181017056107521f, 0.011155996471643448f, 0.011306805536150932f, 0.009680359624326229f, 0.01284029707312584f, 0.010076957754790783f, 0.010573961772024632f, 0.009165825322270393f, 0.011280606500804424f, 0.010152014903724194f, 0.010854631662368774f, 0.008734151721000671f, 0.01112887542694807f, 0.011263168416917324f, 0.011921252124011517f, 0.014275703579187393f, 0.014869860373437405f, 0.009756136685609818f, 0.012241342104971409f, 0.01278745662420988f, 0.01171488780528307f, 0.01027143094688654f, 0.00856859888881445f, 0.015049219131469727f, 0.014698714949190617f, 0.013695450499653816f, 0.01084976363927126f, 0.013054921291768551f, 0.013241538777947426f, 0.012468683533370495f, 0.01077959593385458f, 0.012400266714394093f, 0.016818713396787643f, 0.009743575938045979f, 0.012739661149680614f, 0.012298043817281723f, 0.011503885500133038f, 0.010733197443187237f, 0.011652127839624882f, 0.010523924604058266f, 0.014648542739450932f, 0.012972342781722546f, 0.008850658312439919f, 0.014329005964100361f, 0.014720422215759754f, 0.007156196050345898f, 0.010480204597115517f, 0.009704068303108215f, 0.021872591227293015f, 0.013900676742196083f, 0.013515211641788483f, 0.01376031618565321f, 0.011121181771159172f);
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
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.017682315781712532f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.007414449471980333f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0008263306226581335f, 0.0013639428652822971f, 0.0012590715195983648f, 0.001129054930061102f, 0.0011005887063220143f, 0.0009780250256881118f, 0.001520476769655943f, 0.0007734447717666626f, 0.001059180242009461f, 0.0010698041878640652f, 0.0010492918081581593f, 0.0007485236856155097f, 0.0013350890949368477f, 0.0009290702291764319f, 0.001321225892752409f, 0.0012928383657708764f, 0.0015954904956743121f, 0.0017149007180705667f, 0.0010390630923211575f, 0.001247122883796692f, 0.0010673075448721647f, 0.0008494267240166664f, 0.0008839275105856359f, 0.0005570583161897957f, 0.0008389336871914566f, 0.0007343468023464084f, 0.0011015507625415921f, 0.0015139421448111534f, 0.0010482468642294407f, 0.001224901876412332f, 0.0010879194596782327f, 0.0009110304526984692f, 0.0009154428262263536f, 0.000811960780993104f, 0.0011177501874044538f, 0.0011702595511451364f, 0.000990209635347128f, 0.0011280670296400785f, 0.0009627309627830982f, 0.0011296772863715887f, 0.00169852445833385f, 0.0010375548154115677f, 0.0011615147814154625f, 0.0009332802146673203f, 0.0010099976789206266f, 0.000656886026263237f, 0.0009497990249656141f, 0.0008142281440086663f, 0.001624354044906795f, 0.0009765620343387127f, 0.001050859224051237f, 0.0009074872359633446f, 0.0014706901274621487f, 0.00158061389811337f, 0.0012217898620292544f, 0.0013741153525188565f, 0.0007220857660286129f, 0.0011881543323397636f, 0.001027907826937735f, 0.0011868156725540757f, 0.0011109061306342483f, 0.0010869980324059725f, 0.0006948459194973111f, 0.0013240787666290998f);
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
static const ai_float conv2d_13_t_in_0_fmt_scale_const_f32 = 0.007414449471980333f;
static const ai_float conv2d_13_t_out_0_fmt_scale_const_f32 = 0.01550769992172718f;
static const ai_float conv2d_13_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.00973683875054121f, 0.011392883956432343f, 0.009400582872331142f, 0.008850159123539925f, 0.011206208728253841f, 0.009454852901399136f, 0.01187539380043745f, 0.009503021836280823f, 0.010723561979830265f, 0.011436361819505692f, 0.007736918982118368f, 0.011083363555371761f, 0.0070435283705592155f, 0.007161558140069246f, 0.010646974667906761f, 0.009567124769091606f, 0.011220327578485012f, 0.010160883888602257f, 0.009565343149006367f, 0.008344263769686222f, 0.009242985397577286f, 0.009712683036923409f, 0.007669292390346527f, 0.011558626778423786f, 0.011288605630397797f, 0.009603439830243587f, 0.008189079351723194f, 0.009035988710820675f, 0.009523386135697365f, 0.008225474506616592f, 0.011483877897262573f, 0.010481784120202065f, 0.009516456164419651f, 0.011738892644643784f, 0.010620814748108387f, 0.009941741824150085f, 0.01119336485862732f, 0.010208376683294773f, 0.010443726554512978f, 0.008420038037002087f, 0.008737444877624512f, 0.00713057117536664f, 0.010042822919785976f, 0.009174684062600136f, 0.010606899857521057f, 0.011442366987466812f, 0.008739701472222805f, 0.009751428849995136f, 0.01364393625408411f, 0.009912937879562378f, 0.014635642990469933f, 0.010069942101836205f, 0.010906752198934555f, 0.00857529230415821f, 0.010421568527817726f, 0.01298611517995596f, 0.0077245417051017284f, 0.01176858227699995f, 0.01359174121171236f, 0.010645706206560135f, 0.012066339142620564f, 0.010112645104527473f, 0.010022293776273727f, 0.009764330461621284f, 0.010356280952692032f, 0.010738171637058258f, 0.008610596880316734f, 0.01118994876742363f, 0.011713135056197643f, 0.008164780214428902f, 0.005164581350982189f, 0.010551250539720058f, 0.007173129823058844f, 0.008778293617069721f, 0.00710046011954546f, 0.008201638236641884f, 0.010013436898589134f, 0.010585096664726734f, 0.008892398327589035f, 0.006930679548531771f, 0.010358565486967564f, 0.006769699044525623f, 0.01420842856168747f, 0.00982947088778019f, 0.009523875080049038f, 0.009876672178506851f, 0.011005189269781113f, 0.013799169100821018f, 0.008044860325753689f, 0.008295778185129166f, 0.010033432394266129f, 0.006902046501636505f, 0.00917746964842081f, 0.01273350790143013f, 0.010819690302014351f, 0.010412299074232578f, 0.009605024941265583f, 0.009452404454350471f, 0.010827398858964443f, 0.0105107631534338f, 0.00916166789829731f, 0.009478988125920296f, 0.00927950069308281f, 0.009249919094145298f, 0.006265620235353708f, 0.011153750121593475f, 0.00856166984885931f, 0.007779743988066912f, 0.009149816818535328f, 0.013515396043658257f, 0.011672483757138252f, 0.011112648993730545f, 0.010116423480212688f, 0.010949838906526566f, 0.009924028068780899f, 0.010144087485969067f, 0.00826354417949915f, 0.011230597272515297f, 0.008743446320295334f, 0.006573124323040247f, 0.011322194710373878f, 0.012154944241046906f, 0.00858180783689022f, 0.007914948277175426f, 0.011093971319496632f, 0.008264318108558655f, 0.009526096284389496f, 0.0072518037632107735f);
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
static const ai_i8 conv2d_14_t_out_0_fmt_zero_const_s8 = 4;
static const ai_float conv2d_14_t_in_0_fmt_scale_const_f32 = 0.01550769992172718f;
static const ai_float conv2d_14_t_out_0_fmt_scale_const_f32 = 0.0033287093974649906f;
static const ai_float conv2d_14_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001118904328905046f, 0.0008737894240766764f, 0.0008920226828195155f, 0.000898354162927717f, 0.0005188803188502789f, 0.00046778106479905546f, 0.0005964164156466722f, 0.0005877165822312236f, 0.000857959792483598f, 0.0005940335104241967f, 0.000557219609618187f, 0.0004857218300458044f, 0.0005199493607506156f, 0.0007198661915026605f, 0.0005396655760705471f, 0.000507557881064713f, 0.0006611146964132786f, 0.0012654369929805398f, 0.0005556982941925526f, 0.0004963219980709255f, 0.0007477852632291615f, 0.0009647980332374573f, 0.0010055506136268377f, 0.0005318729672580957f, 0.0006232773885130882f, 0.0008823841926641762f, 0.0010733341332525015f, 0.0004971118178218603f, 0.00047396690933965147f, 0.0005687129450961947f, 0.0009946981444954872f, 0.00048286220408044755f, 0.0005724948714487255f, 0.0005063209682703018f, 0.0005070794140920043f, 0.0005523520521819592f, 0.0010848159436136484f, 0.0008896189974620938f, 0.0007112519233487546f, 0.0004640730912797153f, 0.0005275009316392243f, 0.00047745840856805444f, 0.00029846609686501324f, 0.0010228867176920176f, 0.0004699253768194467f, 0.0008925262372940779f, 0.0007030050037428737f, 0.0005292792338877916f, 0.0005808343412354589f, 0.0005535149830393493f, 0.0005341202486306429f, 0.0006808564066886902f, 0.0005624396726489067f, 0.00047291364171542227f, 0.00046279167872853577f, 0.0003568708198145032f, 0.0009467057534493506f, 0.0005081433337181807f, 0.000542827183380723f, 0.000581738306209445f, 0.0008741756901144981f, 0.0005050969775766134f, 0.0005215288256295025f, 0.0009653206798247993f, 0.0009591024718247354f, 0.0006316374056041241f, 0.0005644491175189614f, 0.0006853224476799369f, 0.0005344006349332631f, 0.0007938837516121566f, 0.00049465341726318f, 0.0005657490692101419f, 0.0005121690919622779f, 0.0010038807522505522f, 0.0005590899963863194f, 0.0005365030956454575f, 0.0009533489355817437f, 0.0005419577355496585f, 0.000441886717453599f, 0.0004889286938123405f, 0.0009725208510644734f, 0.0005649520317092538f, 0.0007938512717373669f, 0.001088433782570064f, 0.0010033940197899938f, 0.000999873736873269f, 0.0008992261136882007f, 0.000747135141864419f, 0.0004005946684628725f, 0.0007161409594118595f, 0.0009933046530932188f, 0.0004931707517243922f, 0.0008250261307694018f, 0.0011088778264820576f, 0.000856029218994081f, 0.0005330688436515629f, 0.0009726062417030334f, 0.0004540146328508854f, 0.000632165523711592f, 0.0007964146207086742f, 0.0007574548362754285f, 0.00054804643150419f, 0.0006772791384719312f, 0.0005458953091874719f, 0.0004474062588997185f, 0.000552995188627392f, 0.0005422236281447113f, 0.0003842120640911162f, 0.0005657279398292303f, 0.0006227442645467818f, 0.0005432241014204919f, 0.0005251443944871426f, 0.0009093732223846018f, 0.0005601992597803473f, 0.0008106777677312493f, 0.0004957899218425155f, 0.0007513358141295612f, 0.0005277655436657369f, 0.0005413427134044468f, 0.0007303478196263313f, 0.0007375112036243081f, 0.0008617212297394872f, 0.00047549715964123607f, 0.0004965602420270443f, 0.0005591181688942015f, 0.0005896508228033781f, 0.0008985085296444595f, 0.0005944432923570275f);
static const ai_u16 conv2d_14_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_14_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_15_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_15_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_15_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_15_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_15_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_15_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_15_t_in_0_fmt_zero_const_s8 = 4;
static const ai_i8 conv2d_15_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_15_t_in_0_fmt_scale_const_f32 = 0.0033287093974649906f;
static const ai_float conv2d_15_t_out_0_fmt_scale_const_f32 = 0.012423967942595482f;
static const ai_float conv2d_15_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.012917162850499153f, 0.02200820855796337f, 0.01494211982935667f, 0.01705400086939335f, 0.01804688200354576f, 0.013957678340375423f, 0.02069772034883499f, 0.02154419757425785f, 0.015498986467719078f, 0.018883641809225082f, 0.019429795444011688f, 0.017291352152824402f, 0.014758848585188389f, 0.019388798624277115f, 0.0191882885992527f, 0.015923358500003815f, 0.01792066916823387f, 0.015822747722268105f, 0.016208434477448463f, 0.01848732680082321f, 0.016252323985099792f, 0.013743367046117783f, 0.02397901378571987f, 0.016075141727924347f, 0.018494946882128716f, 0.012798001989722252f, 0.019492337480187416f, 0.019584979861974716f, 0.016442976891994476f, 0.015885302796959877f, 0.017148669809103012f, 0.011767367832362652f, 0.022269131615757942f, 0.01828709989786148f, 0.01915554888546467f, 0.01821659505367279f, 0.021959509700536728f, 0.01372174546122551f, 0.013933215290307999f, 0.014751032926142216f, 0.017249315977096558f, 0.014887342229485512f, 0.022482536733150482f, 0.018905315548181534f, 0.016943873837590218f, 0.017669042572379112f, 0.020863240584731102f, 0.016005339100956917f, 0.018295183777809143f, 0.023222627118229866f, 0.015541922301054f, 0.016422709450125694f, 0.021327778697013855f, 0.015095019713044167f, 0.01182496827095747f, 0.015082551166415215f, 0.014897741377353668f, 0.017820872366428375f, 0.010638341307640076f, 0.013598989695310593f, 0.011710909195244312f, 0.018354469910264015f, 0.01659853383898735f, 0.014246354810893536f, 0.012350705452263355f, 0.022389035671949387f, 0.014965194277465343f, 0.015825267881155014f, 0.016556624323129654f, 0.015184784308075905f, 0.01573939248919487f, 0.018930811434984207f, 0.02071954868733883f, 0.01707175374031067f, 0.02350904606282711f, 0.017466602846980095f, 0.013950993306934834f, 0.017264341935515404f, 0.017918800935149193f, 0.015299499966204166f, 0.01359887421131134f, 0.016465038061141968f, 0.025213608518242836f, 0.01698180101811886f, 0.01854199543595314f, 0.01658417470753193f, 0.015719374641776085f, 0.020546697080135345f, 0.016159961000084877f, 0.01374875195324421f, 0.02053133398294449f, 0.01900656148791313f, 0.01661347970366478f, 0.02188762091100216f, 0.013460700400173664f, 0.02091659978032112f, 0.01432422362267971f, 0.012066598050296307f, 0.021529478952288628f, 0.01512908935546875f, 0.018781282007694244f, 0.016130736097693443f, 0.01317654550075531f, 0.019185958430171013f, 0.015572675503790379f, 0.013438090682029724f, 0.025165775790810585f, 0.016901465132832527f, 0.013386854901909828f, 0.017235631123185158f, 0.0171960536390543f, 0.013366065919399261f, 0.014734937809407711f, 0.012083402834832668f, 0.020116478204727173f, 0.01531781442463398f, 0.01479558926075697f, 0.016127556562423706f, 0.016975678503513336f, 0.013522247783839703f, 0.02048421837389469f, 0.014453675597906113f, 0.020438602194190025f, 0.014628452248871326f, 0.014743543229997158f, 0.01649361290037632f, 0.012585795484483242f, 0.015701618045568466f);
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
static const ai_i8 conv2d_16_t_out_0_fmt_zero_const_s8 = 5;
static const ai_float conv2d_16_t_in_0_fmt_scale_const_f32 = 0.012423967942595482f;
static const ai_float conv2d_16_t_out_0_fmt_scale_const_f32 = 0.0035066951531916857f;
static const ai_float conv2d_16_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.000722356780897826f, 0.0005746815004386008f, 0.00046728263259865344f, 0.0005648613441735506f, 0.0005439114756882191f, 0.0013792305253446102f, 0.00044900079956278205f, 0.0005658867885358632f, 0.0005556708201766014f, 0.0005389327998273075f, 0.0009227602859027684f, 0.0005330780986696482f, 0.0007707983604632318f, 0.0005600983276963234f, 0.0006633016164414585f, 0.0007284771418198943f, 0.0008046712027862668f, 0.0006617612671107054f, 0.0006891555385664105f, 0.0006701311795040965f, 0.00047743774484843016f, 0.000443946395535022f, 0.0005061461124569178f, 0.001038954360410571f, 0.0005560740828514099f, 0.00125217589084059f, 0.0006784449797123671f, 0.0009968685917556286f, 0.0005020361277274787f, 0.0012583736097440124f, 0.000526344170793891f, 0.0005140086868777871f, 0.00047869986156001687f, 0.0005174707039259374f, 0.0008143605082295835f, 0.0007555499905720353f, 0.0007043189252726734f, 0.000552543206140399f, 0.0003700038942042738f, 0.0005069812177680433f, 0.0005364501848816872f, 0.0007228752365335822f, 0.00042059042607434094f, 0.000560517655685544f, 0.000672186492010951f, 0.0005363968666642904f, 0.0005606230697594583f, 0.0007292103837244213f, 0.00071623275289312f, 0.0008776445174589753f, 0.0006173023721203208f, 0.0006444097380153835f, 0.0011218246072530746f, 0.0006081448518671095f, 0.0003424203023314476f, 0.0009065766935236752f, 0.0004206322773825377f, 0.0005585833569057286f, 0.000739143812097609f, 0.00062210374744609f, 0.0005487700691446662f, 0.0012985765933990479f, 0.000961296318564564f, 0.00048295172746293247f, 0.0011736805317923427f, 0.0005652596591971815f, 0.0005642386386170983f, 0.0007717178668826818f, 0.0004698311968240887f, 0.00055858981795609f, 0.0005826898268423975f, 0.00048322687507607043f, 0.0005408524884842336f, 0.000559329753741622f, 0.0004851754056289792f, 0.0009011709480546415f, 0.0005179311265237629f, 0.0010849442332983017f, 0.0004627240705303848f, 0.0006162356585264206f, 0.0005555201787501574f, 0.00107509212102741f, 0.0008367426926270127f, 0.0005354860913939774f, 0.0005647088983096182f, 0.00044028149568475783f, 0.0006421256111934781f, 0.0004412459675222635f, 0.0009855199605226517f, 0.0005161495064385235f, 0.0007199646206572652f, 0.00048060293192975223f, 0.0005621317541226745f, 0.0005774013116024435f, 0.0009035341208800673f, 0.000512081605847925f, 0.0004594420606736094f, 0.0005350402207113802f, 0.0007790972595103085f, 0.0004706367035396397f, 0.0007778135477565229f, 0.0006261715316213667f, 0.0009033657843247056f, 0.0007169846212491393f, 0.000555861450266093f, 0.0005178782739676535f, 0.0005025853752158582f, 0.0006236248300410807f, 0.0005484612192958593f, 0.0004214500659145415f, 0.00048178809811361134f, 0.0004627410671673715f, 0.0006418721168302f, 0.0005075258668512106f, 0.0006434303359128535f, 0.0005243128980509937f, 0.0006723468541167676f, 0.0007974978652782738f, 0.0007261812570504844f, 0.0005244059721007943f, 0.0004576766805257648f, 0.0005502098938450217f, 0.0006841241847723722f, 0.0005004930426366627f, 0.0010234423680230975f, 0.0006831271457485855f, 0.0011191362282261252f, 0.0004399198805913329f);
static const ai_u16 conv2d_16_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_16_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_17_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_17_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_17_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_17_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_17_t_in_0_fmt_zero_const_s8 = 5;
static const ai_i8 conv2d_17_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_17_t_in_0_fmt_scale_const_f32 = 0.0035066951531916857f;
static const ai_float conv2d_17_t_out_0_fmt_scale_const_f32 = 0.014520758762955666f;
static const ai_float conv2d_17_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.017385318875312805f, 0.016705939546227455f, 0.0184637364000082f, 0.012560193426907063f, 0.013132798485457897f, 0.01897307299077511f, 0.018297862261533737f, 0.023256178945302963f, 0.01579935848712921f, 0.01640906371176243f, 0.013921662233769894f, 0.018435221165418625f, 0.0159158855676651f, 0.01903938502073288f, 0.019932730123400688f, 0.0180828720331192f, 0.01194683089852333f, 0.01594972051680088f, 0.022621478885412216f, 0.015396966598927975f, 0.01526902336627245f, 0.01804734766483307f, 0.015104359947144985f, 0.015689650550484657f, 0.025033673271536827f, 0.013491406105458736f, 0.018993841484189034f, 0.01490760687738657f, 0.016408376395702362f, 0.0138241620734334f, 0.014409467577934265f, 0.017743568867444992f, 0.01683017797768116f, 0.012595463544130325f, 0.018491290509700775f, 0.02319970913231373f, 0.015737950801849365f, 0.015098138712346554f, 0.01810126192867756f, 0.017882028594613075f, 0.017399795353412628f, 0.01478696521371603f, 0.018395937979221344f, 0.018031707033514977f, 0.017118439078330994f, 0.013639941811561584f, 0.01484884973615408f, 0.012925970368087292f, 0.020123163238167763f, 0.01660095900297165f, 0.016427313908934593f, 0.015656379982829094f, 0.018038270995020866f, 0.017834791913628578f, 0.015359845012426376f, 0.020256225019693375f, 0.016750726848840714f, 0.015531748533248901f, 0.01860361546278f, 0.015641631558537483f, 0.013294988311827183f, 0.013399646617472172f, 0.01579136960208416f, 0.01977877877652645f, 0.015385868027806282f, 0.012539240531623363f, 0.01834711618721485f, 0.01677844487130642f, 0.018668901175260544f, 0.017658939585089684f, 0.016288109123706818f, 0.014661171473562717f, 0.01970859430730343f, 0.016722293570637703f, 0.0124378502368927f, 0.012853543274104595f, 0.016894949600100517f, 0.018031608313322067f, 0.017793575301766396f, 0.017009349539875984f, 0.0176826324313879f, 0.018360435962677002f, 0.015524039044976234f, 0.015147021040320396f, 0.015984712168574333f, 0.03212427720427513f, 0.015740038827061653f, 0.014376981183886528f, 0.01640077866613865f, 0.020753197371959686f, 0.012050194665789604f, 0.017677119001746178f, 0.01664135418832302f, 0.018621129915118217f, 0.018604887649416924f, 0.01828066073358059f, 0.016354870051145554f, 0.017044739797711372f, 0.016101203858852386f, 0.019343366846442223f, 0.013578330166637897f, 0.019347691908478737f, 0.019245091825723648f, 0.015219000168144703f, 0.01381403487175703f, 0.01766154356300831f, 0.016712136566638947f, 0.019944101572036743f, 0.012765279039740562f, 0.01442750170826912f, 0.01599026285111904f, 0.018920691683888435f, 0.018219100311398506f, 0.015789980068802834f, 0.02028590813279152f, 0.016259554773569107f, 0.018442638218402863f, 0.017958061769604683f, 0.015457864850759506f, 0.016238780692219734f, 0.014598303474485874f, 0.013860814273357391f, 0.02053280919790268f, 0.021175270900130272f, 0.016414903104305267f, 0.017436422407627106f, 0.01935260184109211f, 0.016735471785068512f);
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
static const ai_i8 conv2d_18_t_out_0_fmt_zero_const_s8 = -10;
static const ai_float conv2d_18_t_in_0_fmt_scale_const_f32 = 0.014520758762955666f;
static const ai_float conv2d_18_t_out_0_fmt_scale_const_f32 = 0.003833226626738906f;
static const ai_float conv2d_18_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0011191536905243993f, 0.0005532507202588022f, 0.0004510947037488222f, 0.00048028313904069364f, 0.0003762537962757051f, 0.0006327751907519996f, 0.00040518169407732785f, 0.00048537825932726264f, 0.0005550224450416863f, 0.001095649553462863f, 0.0007957661291584373f, 0.0005493712378665805f, 0.0005593408131971955f, 0.0007308521308004856f, 0.0005291741690598428f, 0.0009552172850817442f, 0.0005355994217097759f, 0.0006376574747264385f, 0.0004893606528639793f, 0.0005527231842279434f, 0.0005625008489005268f, 0.001000342657789588f, 0.0005586394108831882f, 0.0008454874041490257f, 0.0009638063493184745f, 0.0005603089812211692f, 0.000535647151991725f, 0.0015840482665225863f, 0.000557516235858202f, 0.0007235300727188587f, 0.0007482344517484307f, 0.0005562943406403065f, 0.0005361132207326591f, 0.0005475113284774125f, 0.0009352007764391601f, 0.0005408304859884083f, 0.0005333919543772936f, 0.00045190981472842395f, 0.0005054677021689713f, 0.00084140949184075f, 0.0005524727166630328f, 0.0005837929784320295f, 0.0006547544035129249f, 0.0003728860756382346f, 0.000656390970107168f, 0.00040732641355134547f, 0.001095248619094491f, 0.0014759607147425413f, 0.00039913534419611096f, 0.0005372644518502057f, 0.0005212618270888925f, 0.0012477997224777937f, 0.0007189995376393199f, 0.0005376291228458285f, 0.00040013089892454445f, 0.0007616035873070359f, 0.0007972369785420597f, 0.0010092598386108875f, 0.0005777357728220522f, 0.0006535458378493786f, 0.0005214664270170033f, 0.0011188734788447618f, 0.0005231921677477658f, 0.0005778135964646935f, 0.000651865266263485f, 0.0004563109250739217f, 0.0008139366982504725f, 0.0005149369244463742f, 0.0005478810053318739f, 0.0005184525507502258f, 0.0004893351579084992f, 0.0008966346504166722f, 0.0005392808234319091f, 0.00067991140531376f, 0.0005219945451244712f, 0.0005397000350058079f, 0.000725728808902204f, 0.0005576489493250847f, 0.0006082138861529529f, 0.0005524051375687122f, 0.00048305370728485286f, 0.0003893552639055997f, 0.0007438574102707207f, 0.0005482484120875597f, 0.0004333437536843121f, 0.0013187186559662223f, 0.0008078604005277157f, 0.0008597912383265793f, 0.00044439133489504457f, 0.000866892107296735f, 0.0004337781574577093f, 0.0007290993817150593f, 0.0003482660686131567f, 0.0005963206640444696f, 0.0004545639967545867f, 0.0009091469109989703f, 0.001018939889036119f, 0.0004384251369629055f, 0.0004920572391711175f, 0.0005490090698003769f, 0.0005801266524940729f, 0.0005651258979924023f, 0.0005336845642887056f, 0.0008637356804683805f, 0.0008175033144652843f, 0.0008201351156458259f, 0.0005243299528956413f, 0.0009225097601301968f, 0.0009932764805853367f, 0.0005303247598931193f, 0.0008015984203666449f, 0.0005292160203680396f, 0.0004388539236970246f, 0.0005528230685740709f, 0.0005580571014434099f, 0.000619277881924063f, 0.0005537705146707594f, 0.000744565564673394f, 0.0005014134803786874f, 0.0003849711501970887f, 0.0005644378834404051f, 0.0005156539846211672f, 0.001072108861990273f, 0.0005525663727894425f, 0.0014818558702245355f, 0.0008603204623796046f, 0.0008247819496318698f, 0.0005233035190030932f);
static const ai_u16 conv2d_18_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_18_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_19_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_19_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_19_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_19_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_19_t_in_0_fmt_zero_const_s8 = -10;
static const ai_i8 conv2d_19_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_19_t_in_0_fmt_scale_const_f32 = 0.003833226626738906f;
static const ai_float conv2d_19_t_out_0_fmt_scale_const_f32 = 0.018168434500694275f;
static const ai_float conv2d_19_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.01879330538213253f, 0.01829090341925621f, 0.014939533546566963f, 0.025785338133573532f, 0.01976785622537136f, 0.025374500080943108f, 0.012944316491484642f, 0.018935056403279305f, 0.014691227115690708f, 0.01676120050251484f, 0.018449032679200172f, 0.01820867881178856f, 0.016049083322286606f, 0.011931231245398521f, 0.02059321478009224f, 0.015670977532863617f, 0.015078569762408733f, 0.013908609747886658f, 0.0164233036339283f, 0.015587474219501019f, 0.018380746245384216f, 0.013986076228320599f, 0.013960464857518673f, 0.015426859259605408f, 0.01802959106862545f, 0.015704963356256485f, 0.015065332874655724f, 0.016195325180888176f, 0.01926581561565399f, 0.013326200656592846f, 0.019113054499030113f, 0.013453153893351555f, 0.018324391916394234f, 0.019010094925761223f, 0.015727698802947998f, 0.01626729778945446f, 0.016941579058766365f, 0.015547748655080795f, 0.014470509253442287f, 0.019615251570940018f, 0.015268301591277122f, 0.015802783891558647f, 0.02063237689435482f, 0.015328435227274895f, 0.014330280013382435f, 0.017074082046747208f, 0.01926959492266178f, 0.014925534836947918f, 0.01111594494432211f, 0.017071884125471115f, 0.02708612196147442f, 0.01597946137189865f, 0.017896318808197975f, 0.013827239163219929f, 0.01612025313079357f, 0.016467468813061714f, 0.015353609807789326f, 0.01513172872364521f, 0.014337603002786636f, 0.013554484583437443f, 0.014387624338269234f, 0.013790812343358994f, 0.014858199283480644f, 0.018252376466989517f, 0.014789310283958912f, 0.02062259055674076f, 0.020448148250579834f, 0.01718795858323574f, 0.01502601895481348f, 0.016280625015497208f, 0.011971497908234596f, 0.016116216778755188f, 0.01743404194712639f, 0.016337502747774124f, 0.015202202834188938f, 0.016528135165572166f, 0.014681472443044186f, 0.01705615222454071f, 0.016222897917032242f, 0.018313879147171974f, 0.014330849051475525f, 0.015161564573645592f, 0.01378429401665926f, 0.015874478965997696f, 0.01665889471769333f, 0.016807734966278076f, 0.020783931016921997f, 0.014620354399085045f, 0.027211936190724373f, 0.02559814043343067f, 0.017498865723609924f, 0.017672667279839516f, 0.019663559272885323f, 0.012954280711710453f, 0.013064775615930557f, 0.021951371803879738f, 0.01587611995637417f, 0.016558635979890823f, 0.017675070092082024f, 0.012256459333002567f, 0.016099976375699043f, 0.015259938314557076f, 0.017155131325125694f, 0.018138499930500984f, 0.01705644093453884f, 0.02473420463502407f, 0.023811567574739456f, 0.019208457320928574f, 0.01585797220468521f, 0.016659101471304893f, 0.01548018679022789f, 0.01988581009209156f, 0.018735574558377266f, 0.016562528908252716f, 0.016006147488951683f, 0.015534228645265102f, 0.013892176561057568f, 0.014384889043867588f, 0.014387066476047039f, 0.01609227806329727f, 0.023130055516958237f, 0.016427623108029366f, 0.014472022652626038f, 0.013279261067509651f, 0.012499341741204262f, 0.013140660710632801f, 0.014650769531726837f, 0.016732782125473022f);
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
static const ai_i8 conv2d_20_t_out_0_fmt_zero_const_s8 = 23;
static const ai_float conv2d_20_t_in_0_fmt_scale_const_f32 = 0.018168434500694275f;
static const ai_float conv2d_20_t_out_0_fmt_scale_const_f32 = 0.005018808878958225f;
static const ai_float conv2d_20_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0006811750936321914f, 0.0007580167730338871f, 0.0007111663580872118f, 0.0005644730990752578f, 0.0005782214575447142f, 0.0005180693115107715f, 0.0005263666389510036f, 0.0004437883326318115f, 0.0005706418305635452f, 0.0006518865702673793f, 0.0011201357701793313f, 0.0006469981162808836f, 0.0008478600066155195f, 0.0008403039537370205f, 0.0005335223977454007f, 0.0004664442967623472f, 0.0010176076320931315f, 0.0003766489098779857f, 0.0005320149357430637f, 0.0005351259605959058f, 0.0004849378892686218f, 0.0007522765663452446f, 0.0006912791868671775f, 0.0007415054133161902f, 0.0004247688048053533f, 0.000544280803296715f, 0.0005310604465194046f, 0.0010290450882166624f, 0.0005285029183141887f, 0.0007023205398581922f, 0.0006034566322341561f, 0.0007831996772438288f, 0.0006192714208737016f, 0.001289502833969891f, 0.000533934507984668f, 0.0006581087363883853f, 0.0005258209421299398f, 0.0005180748412385583f, 0.0005728541873395443f, 0.0005073248757980764f, 0.0003879469877574593f, 0.0005578470299951732f, 0.0005377016495913267f, 0.0013573572505265474f, 0.0012235353933647275f, 0.0008635559352114797f, 0.0005057362141087651f, 0.0005653976695612073f, 0.0009078230941668153f, 0.0006608567200601101f, 0.0010088487761095166f, 0.00044701964361593127f, 0.0004793910193257034f, 0.0007590231252834201f, 0.00048247002996504307f, 0.0008007191936485469f, 0.0009394217049703002f, 0.0012824113946408033f, 0.001201252220198512f, 0.0011346458923071623f, 0.0010583302937448025f, 0.0011914820643141866f, 0.0007979705696925521f, 0.0005633996916003525f, 0.00030919371056370437f, 0.00042250746628269553f, 0.0005584865575656295f, 0.0007224646979011595f, 0.000508524477481842f, 0.0005388579447753727f, 0.0004331069940235466f, 0.000937350036110729f, 0.0005584137979894876f, 0.0005626548081636429f, 0.0004827558877877891f, 0.0004944458487443626f, 0.0005277988966554403f, 0.0005562481819652021f, 0.000648126529995352f, 0.00045474120997823775f, 0.0006151564884930849f, 0.0005715786246582866f, 0.000901977124158293f, 0.0006465528858825564f, 0.0005177570856176317f, 0.0007748480420559645f, 0.0006721639656461775f, 0.0005553963710553944f, 0.0004914580495096743f, 0.0004970313748344779f, 0.0008853135514073074f, 0.0008582990267314017f, 0.0007662704447284341f, 0.0009220915380865335f, 0.0008752885041758418f, 0.0011046506697311997f, 0.0008669249364174902f, 0.0005669841193594038f, 0.000507257180288434f, 0.00041956992936320603f, 0.0007642589043825865f, 0.0005587558262050152f, 0.001008324441500008f, 0.0005346395191736519f, 0.000909081194549799f, 0.0005653004627674818f, 0.00047706576879136264f, 0.0004257742257323116f, 0.0007983369287103415f, 0.0005117175751365721f, 0.0006465333863161504f, 0.0010205288417637348f, 0.0003984462528023869f, 0.0006234780885279179f, 0.001292240689508617f, 0.0004989571170881391f, 0.000397178198909387f, 0.0006134605500847101f, 0.000826273753773421f, 0.0005567952757701278f, 0.00048673321725800633f, 0.0005388641729950905f, 0.0005446876166388392f, 0.00048571350635029376f, 0.0006587550160475075f, 0.0005420746165327728f, 0.0009944670600816607f, 0.0009283545659855008f);
static const ai_u16 conv2d_20_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_20_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_21_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_21_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_21_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_21_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_21_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_21_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_21_t_in_0_fmt_zero_const_s8 = 23;
static const ai_i8 conv2d_21_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_21_t_in_0_fmt_scale_const_f32 = 0.005018808878958225f;
static const ai_float conv2d_21_t_out_0_fmt_scale_const_f32 = 0.021219203248620033f;
static const ai_float conv2d_21_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.015700021758675575f, 0.01773208938539028f, 0.015696745365858078f, 0.010763437487185001f, 0.01690112240612507f, 0.01623537391424179f, 0.013556094840168953f, 0.015771551057696342f, 0.013012216426432133f, 0.015929538756608963f, 0.02381083369255066f, 0.017201267182826996f, 0.015125282108783722f, 0.016154080629348755f, 0.014950511045753956f, 0.014037534594535828f, 0.01455687079578638f, 0.015303627587854862f, 0.01659197360277176f, 0.012483114376664162f, 0.014415403828024864f, 0.01281809713691473f, 0.01824118010699749f, 0.02532361075282097f, 0.012470001354813576f, 0.014046424999833107f, 0.016899388283491135f, 0.011519658379256725f, 0.01378064788877964f, 0.011328816413879395f, 0.013904057443141937f, 0.017355050891637802f, 0.011482682079076767f, 0.01594868302345276f, 0.013506880961358547f, 0.017698556184768677f, 0.01657557487487793f, 0.01921776868402958f, 0.02463265135884285f, 0.016512883827090263f, 0.01612403616309166f, 0.013814828358590603f, 0.015429637394845486f, 0.017274323850870132f, 0.01737499237060547f, 0.014611752703785896f, 0.014135867357254028f, 0.01519332267343998f, 0.016435865312814713f, 0.01926511526107788f, 0.015605471096932888f, 0.015006480738520622f, 0.02058325707912445f, 0.015641938894987106f, 0.013364139012992382f, 0.018043339252471924f, 0.015444853343069553f, 0.013496506959199905f, 0.017673159018158913f, 0.01497772615402937f, 0.013977275229990482f, 0.01599474623799324f, 0.014080461114645004f, 0.014330478385090828f, 0.018451346084475517f, 0.015259620733559132f, 0.014001722447574139f, 0.013362743891775608f, 0.012608557939529419f, 0.012480659410357475f, 0.022167909890413284f, 0.011903774924576283f, 0.014569858089089394f, 0.01411930751055479f, 0.01623043231666088f, 0.016877466812729836f, 0.014907917939126492f, 0.012891124002635479f, 0.015773557126522064f, 0.016782457008957863f, 0.014191183261573315f, 0.016776051372289658f, 0.014293287880718708f, 0.015289836563169956f, 0.015747422352433205f, 0.014264865778386593f, 0.012188229709863663f, 0.011672621592879295f, 0.01619878038764f, 0.0165922362357378f, 0.014926363714039326f, 0.01809980347752571f, 0.017129996791481972f, 0.015878431499004364f, 0.013118009082973003f, 0.015524497255682945f, 0.01518165785819292f, 0.013392973691225052f, 0.015017416328191757f, 0.01683913730084896f, 0.013940508477389812f, 0.019672950729727745f, 0.01668565534055233f, 0.01748882792890072f, 0.01696305349469185f, 0.02821781300008297f, 0.019593095406889915f, 0.020820902660489082f, 0.015844274312257767f, 0.017389779910445213f, 0.014045852236449718f, 0.017531761899590492f, 0.020801080390810966f, 0.01199399121105671f, 0.013468049466609955f, 0.014249294064939022f, 0.012178636156022549f, 0.01570146158337593f, 0.012526879087090492f, 0.015410158783197403f, 0.016545765101909637f, 0.014155757613480091f, 0.017555009573698044f, 0.013805600814521313f, 0.017037685960531235f, 0.017422998324036598f, 0.015500141307711601f, 0.014868702739477158f);
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
static const ai_i8 conv2d_22_t_out_0_fmt_zero_const_s8 = 50;
static const ai_float conv2d_22_t_in_0_fmt_scale_const_f32 = 0.021219203248620033f;
static const ai_float conv2d_22_t_out_0_fmt_scale_const_f32 = 0.006158170755952597f;
static const ai_float conv2d_22_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0009258431382477283f, 0.0005658453446812928f, 0.00044780600001104176f, 0.000549760356079787f, 0.0004857141466345638f, 0.0006323365378193557f, 0.000472926942165941f, 0.0005403176764957607f, 0.0005085935117676854f, 0.0005294078728184104f, 0.0005621990421786904f, 0.0007211212068796158f, 0.0003035163099411875f, 0.0006701352540403605f, 0.001137242536060512f, 0.000895169039722532f, 0.0007974650361575186f, 0.0008027643780224025f, 0.000459246919490397f, 0.0007133831386454403f, 0.0012684508692473173f, 0.0005633608088828623f, 0.0006819689297117293f, 0.001585494726896286f, 0.000736083195079118f, 0.0006892955861985683f, 0.0005286587402224541f, 0.0011873024050146341f, 0.000963506055995822f, 0.0005310098640620708f, 0.0009022072772495449f, 0.001251261099241674f, 0.0008003805996850133f, 0.0009574537980370224f, 0.000951089255977422f, 0.0006968932575546205f, 0.0006998890312388539f, 0.0010151644237339497f, 0.0005411041784100235f, 0.0005637278663925827f, 0.0005262995255179703f, 0.00047986386925913393f, 0.0011741755297407508f, 0.0013577785575762391f, 0.0004388155648484826f, 0.0005463582929223776f, 0.0003672445600386709f, 0.0007578616496175528f, 0.0006735911592841148f, 0.0006271993624977767f, 0.0005275732255540788f, 0.0007795683923177421f, 0.0004511809383984655f, 0.000944289960898459f, 0.0005373982712626457f, 0.0006691868184134364f, 0.001060031121596694f, 0.0005546865868382156f, 0.0005017502699047327f, 0.0004996393690817058f, 0.0005617113201878965f, 0.0005684167263098061f, 0.0004865018418058753f, 0.0007674014777876437f, 0.0006728447624482214f, 0.0008712265407666564f, 0.0008026648429222405f, 0.0007325633778236806f, 0.0005617872229777277f, 0.000995833077467978f, 0.00035130884498357773f, 0.000826982141006738f, 0.0014783485094085336f, 0.001185560249723494f, 0.000586475885938853f, 0.0004945116234011948f, 0.0005211752140894532f, 0.0005318907788023353f, 0.0005642633768729866f, 0.0004827928787562996f, 0.0008624606998637319f, 0.0005618276773020625f, 0.0005301256896927953f, 0.0007617427036166191f, 0.0005503693828359246f, 0.0009730836027301848f, 0.0011718608438968658f, 0.0005549592897295952f, 0.00037764705484732985f, 0.0005545128369703889f, 0.0008140653953887522f, 0.0005149575299583375f, 0.0010006495285779238f, 0.0004408153472468257f, 0.00045515349484048784f, 0.0005014390335418284f, 0.0010556498309597373f, 0.001159963896498084f, 0.0004918695776723325f, 0.0006010354263707995f, 0.0007022434147074819f, 0.0004514966858550906f, 0.0005644358461722732f, 0.0010221345582976937f, 0.0004123532271478325f, 0.00043994662701152265f, 0.00044065286056138575f, 0.0005488567985594273f, 0.0005819790530949831f, 0.0011850902810692787f, 0.0005639860173687339f, 0.00044242432340979576f, 0.0005344796809367836f, 0.0010965822730213404f, 0.0006151520647108555f, 0.0008949923212639987f, 0.0009122299961745739f, 0.0009492185199633241f, 0.0005352304433472455f, 0.0006616574828512967f, 0.0005153070669621229f, 0.0005611710948869586f, 0.0009827754693105817f, 0.0009897204581648111f, 0.001077725668437779f, 0.0005170812364667654f, 0.0004787355428561568f, 0.0005486852605827153f);
static const ai_u16 conv2d_22_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_22_t_out_0_shape_h_const_u16 = 1;


static const ai_u16 conv2d_29_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_29_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_29_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_29_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_29_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_29_t_out_0_shape_ch_const_u16 = 10;
static const ai_i8 conv2d_29_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_29_t_out_0_fmt_zero_const_s8 = 44;
static const ai_float conv2d_29_t_in_0_fmt_scale_const_f32 = 0.025286104530096054f;
static const ai_float conv2d_29_t_out_0_fmt_scale_const_f32 = 0.25218451023101807f;
static const ai_float conv2d_29_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0036915333475917578f, 0.0035455217584967613f, 0.0038557001389563084f, 0.002713906578719616f, 0.0034928249660879374f, 0.00315299304202199f, 0.004658110439777374f, 0.003297960851341486f, 0.003426368348300457f, 0.002719560405239463f);
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
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 4272);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 3076);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(1, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_rgb_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_t_out_0_shape_ch_const_u16, conv2d_1_t_weight_0_shape_w_const_u16, conv2d_1_l_pad_W_0_const_s32, conv2d_1_l_stride_0_const_u16, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_l_out_ch_format_const_layer_format_type, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, 1196, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(1, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_2 */
  {
      const ai_i8* conv2d_2_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 4272);
    const ai_i8* conv2d_2_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[2] + 0);
    const ai_i32* conv2d_2_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[3] + 0);
    ai_i8* conv2d_2_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 596);
    ai_i16* conv2d_2_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) conv2d_2_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_2_t_in_0_ptr_const_s8, conv2d_2_t_in_0_shape_w_const_u16, conv2d_2_t_in_0_shape_h_const_u16, conv2d_2_t_in_0_shape_ch_const_u16, conv2d_2_t_weight_0_ptr_const_s8, conv2d_2_t_weight_0_shape_w_const_u16, conv2d_2_t_weight_0_shape_h_const_u16, conv2d_2_l_pad_W_0_const_s32, conv2d_2_l_pad_H_0_const_s32, conv2d_2_l_stride_1_const_u16, conv2d_2_l_stride_0_const_u16, conv2d_2_t_weight_1_ptr_const_s32, conv2d_2_t_in_0_fmt_zero_const_s8, conv2d_2_t_out_0_fmt_zero_const_s8, conv2d_2_t_in_0_fmt_scale_const_f32, conv2d_2_t_out_0_fmt_scale_const_f32, conv2d_2_t_weight_0_fmt_scale_const_f32, conv2d_2_t_out_0_ptr_s8, conv2d_2_t_out_0_shape_w_const_u16, conv2d_2_t_out_0_shape_h_const_u16, 0, 593, conv2d_2_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) conv2d_2_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_2 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_3 */
  {
      const ai_i8* conv2d_3_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 596);
    const ai_i8* conv2d_3_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[4] + 0);
    const ai_i32* conv2d_3_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[5] + 0);
    ai_i8* conv2d_3_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1620);
    ai_i16* conv2d_3_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr) conv2d_3_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_3_t_in_0_ptr_const_s8, conv2d_3_t_in_0_shape_w_const_u16, conv2d_3_t_in_0_shape_h_const_u16, conv2d_3_l_stride_1_const_u16, conv2d_3_l_stride_0_const_u16, conv2d_3_t_in_0_shape_ch_const_u16, conv2d_3_t_weight_0_ptr_const_s8, conv2d_3_t_out_0_shape_ch_const_u16, conv2d_3_t_weight_1_ptr_const_s32, conv2d_3_t_in_0_fmt_zero_const_s8, conv2d_3_t_out_0_fmt_zero_const_s8, conv2d_3_t_in_0_fmt_scale_const_f32, conv2d_3_t_out_0_fmt_scale_const_f32, conv2d_3_t_weight_0_fmt_scale_const_f32, conv2d_3_l_out_ch_format_const_layer_format_type, conv2d_3_t_out_0_ptr_s8, 1, 544, conv2d_3_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr) conv2d_3_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_3 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_4 */
  {
      const ai_i8* conv2d_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1620);
    const ai_i8* conv2d_4_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[6] + 0);
    const ai_i32* conv2d_4_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[7] + 0);
    ai_i8* conv2d_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_4_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 4692);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) conv2d_4_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_4_t_in_0_ptr_const_s8, conv2d_4_t_in_0_shape_w_const_u16, conv2d_4_t_in_0_shape_h_const_u16, conv2d_4_t_in_0_shape_ch_const_u16, conv2d_4_t_weight_0_ptr_const_s8, conv2d_4_t_weight_0_shape_w_const_u16, conv2d_4_t_weight_0_shape_h_const_u16, conv2d_4_l_pad_W_0_const_s32, conv2d_4_l_pad_H_0_const_s32, conv2d_4_l_stride_1_const_u16, conv2d_4_l_stride_0_const_u16, conv2d_4_t_weight_1_ptr_const_s32, conv2d_4_t_in_0_fmt_zero_const_s8, conv2d_4_t_out_0_fmt_zero_const_s8, conv2d_4_t_in_0_fmt_scale_const_f32, conv2d_4_t_out_0_fmt_scale_const_f32, conv2d_4_t_weight_0_fmt_scale_const_f32, conv2d_4_t_out_0_ptr_s8, conv2d_4_t_out_0_shape_w_const_u16, conv2d_4_t_out_0_shape_h_const_u16, 0, 1777, conv2d_4_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) conv2d_4_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_4 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5 */
  {
      const ai_i8* conv2d_5_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_5_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[8] + 0);
    const ai_i32* conv2d_5_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[9] + 0);
    ai_i8* conv2d_5_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1760);
    ai_i16* conv2d_5_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 768);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_5_t_in_0_ptr_const_s8, conv2d_5_t_in_0_shape_w_const_u16, conv2d_5_t_in_0_shape_h_const_u16, conv2d_5_l_stride_1_const_u16, conv2d_5_l_stride_0_const_u16, conv2d_5_t_in_0_shape_ch_const_u16, conv2d_5_t_weight_0_ptr_const_s8, conv2d_5_t_out_0_shape_ch_const_u16, conv2d_5_t_weight_1_ptr_const_s32, conv2d_5_t_in_0_fmt_zero_const_s8, conv2d_5_t_out_0_fmt_zero_const_s8, conv2d_5_t_in_0_fmt_scale_const_f32, conv2d_5_t_out_0_fmt_scale_const_f32, conv2d_5_t_weight_0_fmt_scale_const_f32, conv2d_5_l_out_ch_format_const_layer_format_type, conv2d_5_t_out_0_ptr_s8, 1, 992, conv2d_5_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_5 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_6 */
  {
      const ai_i8* conv2d_6_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1760);
    const ai_i8* conv2d_6_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[10] + 0);
    const ai_i32* conv2d_6_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[11] + 0);
    ai_i8* conv2d_6_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_6_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 3040);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 1, {(stai_ptr) conv2d_6_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_6_t_in_0_ptr_const_s8, conv2d_6_t_in_0_shape_w_const_u16, conv2d_6_t_in_0_shape_h_const_u16, conv2d_6_t_in_0_shape_ch_const_u16, conv2d_6_t_weight_0_ptr_const_s8, conv2d_6_t_weight_0_shape_w_const_u16, conv2d_6_t_weight_0_shape_h_const_u16, conv2d_6_l_pad_W_0_const_s32, conv2d_6_l_pad_H_0_const_s32, conv2d_6_l_stride_1_const_u16, conv2d_6_l_stride_0_const_u16, conv2d_6_t_weight_1_ptr_const_s32, conv2d_6_t_in_0_fmt_zero_const_s8, conv2d_6_t_out_0_fmt_zero_const_s8, conv2d_6_t_in_0_fmt_scale_const_f32, conv2d_6_t_out_0_fmt_scale_const_f32, conv2d_6_t_weight_0_fmt_scale_const_f32, conv2d_6_t_out_0_ptr_s8, conv2d_6_t_out_0_shape_w_const_u16, conv2d_6_t_out_0_shape_h_const_u16, 0, 2961, conv2d_6_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, {(stai_ptr) conv2d_6_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
  {
      const ai_i8* conv2d_7_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_7_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[12] + 0);
    const ai_i32* conv2d_7_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[13] + 0);
    ai_i8* conv2d_7_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2240);
    ai_i16* conv2d_7_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1280);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_7_t_in_0_ptr_const_s8, conv2d_7_t_in_0_shape_w_const_u16, conv2d_7_t_in_0_shape_h_const_u16, conv2d_7_l_stride_1_const_u16, conv2d_7_l_stride_0_const_u16, conv2d_7_t_in_0_shape_ch_const_u16, conv2d_7_t_weight_0_ptr_const_s8, conv2d_7_t_out_0_shape_ch_const_u16, conv2d_7_t_weight_1_ptr_const_s32, conv2d_7_t_in_0_fmt_zero_const_s8, conv2d_7_t_out_0_fmt_zero_const_s8, conv2d_7_t_in_0_fmt_scale_const_f32, conv2d_7_t_out_0_fmt_scale_const_f32, conv2d_7_t_weight_0_fmt_scale_const_f32, conv2d_7_l_out_ch_format_const_layer_format_type, conv2d_7_t_out_0_ptr_s8, 1, 960, conv2d_7_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_7 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_8 */
  {
      const ai_i8* conv2d_8_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2240);
    const ai_i8* conv2d_8_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[14] + 0);
    const ai_i32* conv2d_8_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[15] + 0);
    ai_i8* conv2d_8_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_8_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 3264);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16, conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_t_in_0_shape_ch_const_u16, conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_weight_0_shape_w_const_u16, conv2d_8_t_weight_0_shape_h_const_u16, conv2d_8_l_pad_W_0_const_s32, conv2d_8_l_pad_H_0_const_s32, conv2d_8_l_stride_1_const_u16, conv2d_8_l_stride_0_const_u16, conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8, conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32, conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32, conv2d_8_t_out_0_ptr_s8, conv2d_8_t_out_0_shape_w_const_u16, conv2d_8_t_out_0_shape_h_const_u16, 0, 2369, conv2d_8_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_8 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_9 */
  {
      const ai_i8* conv2d_9_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_9_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[16] + 0);
    const ai_i32* conv2d_9_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[17] + 0);
    ai_i8* conv2d_9_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1472);
    ai_i16* conv2d_9_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 256);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_9_t_in_0_ptr_const_s8, conv2d_9_t_in_0_shape_w_const_u16, conv2d_9_t_in_0_shape_h_const_u16, conv2d_9_l_stride_1_const_u16, conv2d_9_l_stride_0_const_u16, conv2d_9_t_in_0_shape_ch_const_u16, conv2d_9_t_weight_0_ptr_const_s8, conv2d_9_t_out_0_shape_ch_const_u16, conv2d_9_t_weight_1_ptr_const_s32, conv2d_9_t_in_0_fmt_zero_const_s8, conv2d_9_t_out_0_fmt_zero_const_s8, conv2d_9_t_in_0_fmt_scale_const_f32, conv2d_9_t_out_0_fmt_scale_const_f32, conv2d_9_t_weight_0_fmt_scale_const_f32, conv2d_9_l_out_ch_format_const_layer_format_type, conv2d_9_t_out_0_ptr_s8, 1, 1216, conv2d_9_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_9 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_10 */
  {
      const ai_i8* conv2d_10_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1472);
    const ai_i8* conv2d_10_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[18] + 0);
    const ai_i32* conv2d_10_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[19] + 0);
    ai_i8* conv2d_10_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_10_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1856);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(10, 1, {(stai_ptr) conv2d_10_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_10_t_in_0_ptr_const_s8, conv2d_10_t_in_0_shape_w_const_u16, conv2d_10_t_in_0_shape_h_const_u16, conv2d_10_t_in_0_shape_ch_const_u16, conv2d_10_t_weight_0_ptr_const_s8, conv2d_10_t_weight_0_shape_w_const_u16, conv2d_10_t_weight_0_shape_h_const_u16, conv2d_10_l_pad_W_0_const_s32, conv2d_10_l_pad_H_0_const_s32, conv2d_10_l_stride_1_const_u16, conv2d_10_l_stride_0_const_u16, conv2d_10_t_weight_1_ptr_const_s32, conv2d_10_t_in_0_fmt_zero_const_s8, conv2d_10_t_out_0_fmt_zero_const_s8, conv2d_10_t_in_0_fmt_scale_const_f32, conv2d_10_t_out_0_fmt_scale_const_f32, conv2d_10_t_weight_0_fmt_scale_const_f32, conv2d_10_t_out_0_ptr_s8, conv2d_10_t_out_0_shape_w_const_u16, conv2d_10_t_out_0_shape_h_const_u16, 0, 3553, conv2d_10_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, {(stai_ptr) conv2d_10_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_10 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[20] + 0);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[21] + 0);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1408);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 384);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_l_stride_1_const_u16, conv2d_11_l_stride_0_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_t_out_0_shape_ch_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_l_out_ch_format_const_layer_format_type, conv2d_11_t_out_0_ptr_s8, 1, 1024, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1408);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[22] + 0);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[23] + 0);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1664);
  
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
      ai_i8* nl_30_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 248);
    const ai_i8* nl_30_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1124);
    ai_i32* nl_30_t_scratch_0_ptr_s32 = (ai_i32*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(30, 1, {(stai_ptr) nl_30_t_in_0_ptr_const_s8});
    
  forward_lite_nl_softmax_is8os8(nl_30_t_out_0_ptr_s8, nl_30_t_in_0_ptr_const_s8, nl_30_t_in_0_shape_ch_h_w_prod_const_u32, 1, 10, 1083124224, 25, -62, nl_30_t_scratch_0_ptr_s32);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(30, 1, {(stai_ptr) nl_30_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END nl_30 */
  /* LITE_KERNEL_SECTION BEGIN conversion_35 */
  {
      const ai_i8* conversion_35_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 248);
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

