/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2025-12-31T12:37:37+0000
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

#include "ai_lite_inspect.h"

#include "lite_operators.h"
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
#define _STAI_NETWORK_MODEL_SIGNATURE     "0xb6696d41992d73688b38079560ab0338"
#define _STAI_NETWORK_DATETIME            "2025-12-31T12:37:37+0000"
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
      STAI_DECLARE_ARRAY(int32_t, 1, 16384),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_46_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_46_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_47_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_47_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1152),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_48_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_48_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 512),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_49_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_49_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 32768),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_50_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_50_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1024),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_51_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_51_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2304),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_52_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_52_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1024),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_53_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_53_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 65536),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_54_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_54_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 1024),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_55_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_55_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 2560),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_56_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_56_SIZE_BYTES,
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
      (stai_ptr)g_network_conv2d_1_weights_array,(stai_ptr)g_network_conv2d_1_bias_array,(stai_ptr)g_network_conv2d_2_weights_array,(stai_ptr)g_network_conv2d_2_bias_array,(stai_ptr)g_network_conv2d_3_weights_array,(stai_ptr)g_network_conv2d_3_bias_array,(stai_ptr)g_network_conv2d_4_weights_array,(stai_ptr)g_network_conv2d_4_bias_array,(stai_ptr)g_network_conv2d_5_weights_array,(stai_ptr)g_network_conv2d_5_bias_array,(stai_ptr)g_network_conv2d_6_weights_array,(stai_ptr)g_network_conv2d_6_bias_array,(stai_ptr)g_network_conv2d_7_weights_array,(stai_ptr)g_network_conv2d_7_bias_array,(stai_ptr)g_network_conv2d_8_weights_array,(stai_ptr)g_network_conv2d_8_bias_array,(stai_ptr)g_network_conv2d_9_weights_array,(stai_ptr)g_network_conv2d_9_bias_array,(stai_ptr)g_network_conv2d_10_weights_array,(stai_ptr)g_network_conv2d_10_bias_array,(stai_ptr)g_network_conv2d_11_weights_array,(stai_ptr)g_network_conv2d_11_bias_array,(stai_ptr)g_network_conv2d_12_weights_array,(stai_ptr)g_network_conv2d_12_bias_array,(stai_ptr)g_network_conv2d_13_weights_array,(stai_ptr)g_network_conv2d_13_bias_array,(stai_ptr)g_network_conv2d_14_weights_array,(stai_ptr)g_network_conv2d_14_bias_array,(stai_ptr)g_network_conv2d_15_weights_array,(stai_ptr)g_network_conv2d_15_bias_array,(stai_ptr)g_network_conv2d_16_weights_array,(stai_ptr)g_network_conv2d_16_bias_array,(stai_ptr)g_network_conv2d_17_weights_array,(stai_ptr)g_network_conv2d_17_bias_array,(stai_ptr)g_network_conv2d_18_weights_array,(stai_ptr)g_network_conv2d_18_bias_array,(stai_ptr)g_network_conv2d_19_weights_array,(stai_ptr)g_network_conv2d_19_bias_array,(stai_ptr)g_network_conv2d_20_weights_array,(stai_ptr)g_network_conv2d_20_bias_array,(stai_ptr)g_network_conv2d_21_weights_array,(stai_ptr)g_network_conv2d_21_bias_array,(stai_ptr)g_network_conv2d_22_weights_array,(stai_ptr)g_network_conv2d_22_bias_array,(stai_ptr)g_network_conv2d_23_weights_array,(stai_ptr)g_network_conv2d_23_bias_array,(stai_ptr)g_network_conv2d_24_weights_array,(stai_ptr)g_network_conv2d_24_bias_array,(stai_ptr)g_network_conv2d_25_weights_array,(stai_ptr)g_network_conv2d_25_bias_array,(stai_ptr)g_network_conv2d_26_weights_array,(stai_ptr)g_network_conv2d_26_bias_array,(stai_ptr)g_network_conv2d_27_weights_array,(stai_ptr)g_network_conv2d_27_bias_array,(stai_ptr)g_network_gemm_29_weights_array,(stai_ptr)g_network_gemm_29_bias_array
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
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_26_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.011937644332647324f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_27_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.015968982130289078f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_27_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 256,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0018034717068076134f, 0.0015569273382425308f, 0.0020242936443537474f, 0.0019305609166622162f, 0.0021078281570225954f, 0.0018259456846863031f, 0.002161627169698477f, 0.002763219876214862f, 0.0019318016711622477f, 0.0018553317058831453f, 0.0017877109348773956f, 0.0020368611440062523f, 0.0017899400554597378f, 0.002154346788302064f, 0.0014584382297471166f, 0.0019154301844537258f, 0.0017288514645770192f, 0.0019297577673569322f, 0.0017384253442287445f, 0.0017441755626350641f, 0.0016478830948472023f, 0.0023067882284522057f, 0.0020487550646066666f, 0.0022046675439924f, 0.002214440144598484f, 0.0015991535037755966f, 0.0018994879210367799f, 0.0017326807137578726f, 0.0015628638211637735f, 0.0024039563722908497f, 0.0025625783018767834f, 0.001804082072339952f, 0.0014871404273435473f, 0.0021832750644534826f, 0.0020324522629380226f, 0.0031069384422153234f, 0.0021094921976327896f, 0.0020982339046895504f, 0.0021087657660245895f, 0.0017608911730349064f, 0.0019619413651525974f, 0.0022030577529221773f, 0.00161398621276021f, 0.0013791073579341173f, 0.002102810423821211f, 0.0018590455874800682f, 0.0016209118766710162f, 0.001980959204956889f, 0.0018474232638254762f, 0.001705680275335908f, 0.0026111495681107044f, 0.002199573675170541f, 0.0020993936341255903f, 0.00163308868650347f, 0.0021444193553179502f, 0.0018449357012286782f, 0.0031318562105298042f, 0.0016152471071109176f, 0.0019430750980973244f, 0.0017800498753786087f, 0.0020492258481681347f, 0.0021529647056013346f, 0.002285892842337489f, 0.001977739855647087f, 0.0016605129931122065f, 0.001631200429983437f, 0.00154790875967592f, 0.0020949889440089464f, 0.0016967073315754533f, 0.0018395998049527407f, 0.002391785616055131f, 0.0024297237396240234f, 0.0019422060577198863f, 0.001681844936683774f, 0.0020721538458019495f, 0.0015853584045544267f, 0.0021892846561968327f, 0.0019440327305346727f, 0.001761527033522725f, 0.0018240241333842278f, 0.001934443018399179f, 0.0020566200837492943f, 0.0018197267781943083f, 0.002296966966241598f, 0.0019148685969412327f, 0.0026224914472550154f, 0.0023722699843347073f, 0.0014337729662656784f, 0.001954485196620226f, 0.0020013281609863043f, 0.0018081037560477853f, 0.001709328149445355f, 0.001558271935209632f, 0.001697740750387311f, 0.0020940862596035004f, 0.0018271023873239756f, 0.0019162138924002647f, 0.0017322967760264874f, 0.0023149685002863407f, 0.0020052373874932528f, 0.0018259122734889388f, 0.0020629826467484236f, 0.0017774063162505627f, 0.0016133503522723913f, 0.001633147243410349f, 0.0014274995774030685f, 0.0018439646810293198f, 0.0020310510881245136f, 0.0019650624599307775f, 0.001837888266891241f, 0.0017587958136573434f, 0.001851928303949535f, 0.0015098836738616228f, 0.0019368694629520178f, 0.0018616417655721307f, 0.0021120761521160603f, 0.0015290859155356884f, 0.0018084770999848843f, 0.0018178456230089068f, 0.0020943377166986465f, 0.0023634755052626133f, 0.0019676978699862957f, 0.0020020348019897938f, 0.0014577555702999234f, 0.0017609744099900126f, 0.001802298123948276f, 0.0014881411334499717f, 0.0019193132175132632f, 0.001589141204021871f, 0.002073508920148015f, 0.001449253992177546f, 0.0020877753850072622f, 0.002393282949924469f, 0.0019406299106776714f, 0.0021883537992835045f, 0.0016918957699090242f, 0.0017837834311649203f, 0.0020754863508045673f, 0.0017918047960847616f, 0.0018983731279149652f, 0.002042231149971485f, 0.0018926436314359307f, 0.0017887871945276856f, 0.0019004078349098563f, 0.0017685623606666923f, 0.00209544925019145f, 0.0015507952775806189f, 0.0024570643436163664f, 0.0014621566515415907f, 0.0014979527331888676f, 0.0019501777132973075f, 0.0016563431126996875f, 0.002269931137561798f, 0.0017206998309120536f, 0.0019805848132818937f, 0.00228977226652205f, 0.0021316700149327517f, 0.0020507504232227802f, 0.0020270205568522215f, 0.0022020393516868353f, 0.0025691031478345394f, 0.0020411901641637087f, 0.0021049242932349443f, 0.0023808262776583433f, 0.0017009266884997487f, 0.001979849301278591f, 0.002348351525142789f, 0.0018131184624508023f, 0.001785009284503758f, 0.002046355977654457f, 0.002094960305839777f, 0.0025597941130399704f, 0.0018005584133788943f, 0.0017733152490109205f, 0.002168054925277829f, 0.0012768907472491264f, 0.0017995978705585003f, 0.0022953462321311235f, 0.0019647583831101656f, 0.0017861735541373491f, 0.0022038770839571953f, 0.0021403327118605375f, 0.0021384048741310835f, 0.0016120010986924171f, 0.0016127413837239146f, 0.00196009106002748f, 0.0014261212199926376f, 0.0018305170815438032f, 0.0016394074773415923f, 0.0019766411278396845f, 0.0025533942971378565f, 0.0016179856611415744f, 0.0017177457921206951f, 0.0015567472437396646f, 0.0016203193226829171f, 0.0015678263735026121f, 0.001990523422136903f, 0.001719446387141943f, 0.0017908501904457808f, 0.0015562641201540828f, 0.0017426225822418928f, 0.0019016485894098878f, 0.0025526918470859528f, 0.0013219822430983186f, 0.0016202529659494758f, 0.001925083459354937f, 0.0019004439236596227f, 0.001755366800352931f, 0.0015631058486178517f, 0.0017043622210621834f, 0.0026914915069937706f, 0.0019853312987834215f, 0.0018576219445094466f, 0.0016685575246810913f, 0.001601892989128828f, 0.0014415221521630883f, 0.0017621621955186129f, 0.0017110741464421153f, 0.001364176976494491f, 0.001892229774966836f, 0.0014977562241256237f, 0.001772832009010017f, 0.0017373452428728342f, 0.002085563028231263f, 0.0019547438714653254f, 0.0019887504167854786f, 0.002358750207349658f, 0.002575118327513337f, 0.0022088123951107264f, 0.00237033748999238f, 0.0020438325591385365f, 0.001926088472828269f, 0.001992067787796259f, 0.0024135347921401262f, 0.0023933362681418657f, 0.0015539525775238872f, 0.0026114587672054768f, 0.0019671455956995487f, 0.0018805685685947537f, 0.001346562523394823f, 0.0017958146054297686f, 0.0018677106127142906f, 0.002212576800957322f, 0.0017330022528767586f, 0.00221367203630507f, 0.0019104599487036467f, 0.0018866034224629402f, 0.0021556788124144077f, 0.0021245405077934265f, 0.001668966026045382f, 0.002034037606790662f, 0.002154764486476779f, 0.0019907199312001467f, 0.0019912533462047577f, 0.002017643302679062f, 0.0017350877169519663f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_27_scratch1_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.015968982130289078f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_29_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.12688586115837097f),
    AI_PACK_INTQ_ZP(12)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(gemm_29_weights_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 10,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.002913424512371421f, 0.002942608203738928f, 0.0027364154811948538f, 0.0023744546342641115f, 0.0031937190797179937f, 0.002580640371888876f, 0.0037313925568014383f, 0.002926301211118698f, 0.002520178211852908f, 0.0025283510331064463f),
    AI_PACK_INTQ_ZP(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_26_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 256, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_27_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 256, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_27_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 65536, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_27_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 256, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_27_scratch0_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 3584, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_27_scratch1_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 256, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  gemm_29_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 10, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  gemm_29_weights_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 2560, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  gemm_29_bias_array, AI_ARRAY_FORMAT_S32,
  NULL, NULL, 10, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  gemm_29_scratch0_array, AI_ARRAY_FORMAT_S16,
  NULL, NULL, 306, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_26_output, AI_STATIC,
  69, 0x1,
  AI_SHAPE_INIT(4, 1, 256, 1, 1), AI_STRIDE_INIT(4, 1, 1, 256, 256),
  1, &conv2d_26_output_array, &conv2d_26_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_27_bias, AI_STATIC,
  72, 0x0,
  AI_SHAPE_INIT(4, 1, 256, 1, 1), AI_STRIDE_INIT(4, 4, 4, 1024, 1024),
  1, &conv2d_27_bias_array, NULL)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_27_output, AI_STATIC,
  73, 0x1,
  AI_SHAPE_INIT(4, 1, 256, 1, 1), AI_STRIDE_INIT(4, 1, 1, 256, 256),
  1, &conv2d_27_output_array, &conv2d_27_output_array_intq)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_27_scratch0, AI_STATIC,
  74, 0x0,
  AI_SHAPE_INIT(4, 1, 3584, 1, 1), AI_STRIDE_INIT(4, 1, 1, 3584, 3584),
  1, &conv2d_27_scratch0_array, NULL)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_27_scratch1, AI_STATIC,
  75, 0x1,
  AI_SHAPE_INIT(4, 1, 256, 1, 1), AI_STRIDE_INIT(4, 1, 1, 256, 256),
  1, &conv2d_27_scratch1_array, &conv2d_27_scratch1_array_intq)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_27_weights, AI_STATIC,
  76, 0x1,
  AI_SHAPE_INIT(4, 256, 1, 1, 256), AI_STRIDE_INIT(4, 1, 256, 65536, 65536),
  1, &conv2d_27_weights_array, &conv2d_27_weights_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  gemm_29_bias, AI_STATIC,
  111, 0x0,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 4, 4, 40, 40),
  1, &gemm_29_bias_array, NULL)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  gemm_29_output, AI_STATIC,
  112, 0x1,
  AI_SHAPE_INIT(4, 1, 10, 1, 1), AI_STRIDE_INIT(4, 1, 1, 10, 10),
  1, &gemm_29_output_array, &gemm_29_output_array_intq)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  gemm_29_scratch0, AI_STATIC,
  113, 0x0,
  AI_SHAPE_INIT(4, 1, 306, 1, 1), AI_STRIDE_INIT(4, 2, 2, 612, 612),
  1, &gemm_29_scratch0_array, NULL)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  gemm_29_weights, AI_STATIC,
  114, 0x1,
  AI_SHAPE_INIT(4, 256, 10, 1, 1), AI_STRIDE_INIT(4, 1, 256, 2560, 2560),
  1, &gemm_29_weights_array, &gemm_29_weights_array_intq)


AI_TENSOR_CHAIN_OBJ_DECLARE(
  conv2d_27_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_26_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_27_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &conv2d_27_weights, &conv2d_27_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_27_scratch0, &conv2d_27_scratch1)
)

AI_LAYER_OBJ_DECLARE(
  conv2d_27_layer, 28,
  OPTIMIZED_CONV2D_TYPE, 0x0, NULL,
  conv2d_nl_pool, forward_pw_sssa8_ch_nl_pool,
  &conv2d_27_chain,
  NULL, &conv2d_27_layer, AI_STATIC, 
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

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gemm_29_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_27_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_29_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gemm_29_weights, &gemm_29_bias),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gemm_29_scratch0)
)

AI_LAYER_OBJ_DECLARE(
  gemm_29_layer, 29,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense_integer_SSSA_ch,
  &gemm_29_chain,
  NULL, &gemm_29_layer, AI_STATIC, 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_conv2d_27(_stai_network_context* net_ctx)
{
  conv2d_26_output_array.data = AI_PTR(net_ctx->_activations[0] + 9476);
  conv2d_26_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 9476);
  conv2d_27_weights_array.data = AI_PTR(net_ctx->_weights[52] + 0);
  conv2d_27_weights_array.data_start = AI_PTR(net_ctx->_weights[52] + 0);
  conv2d_27_bias_array.data = AI_PTR(net_ctx->_weights[53] + 0);
  conv2d_27_bias_array.data_start = AI_PTR(net_ctx->_weights[53] + 0);
  conv2d_27_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_27_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  conv2d_27_scratch1_array.data = AI_PTR(net_ctx->_activations[0] + 3584);
  conv2d_27_scratch1_array.data_start = AI_PTR(net_ctx->_activations[0] + 3584);
  conv2d_27_output_array.data = AI_PTR(net_ctx->_activations[0] + 3840);
  conv2d_27_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 3840);
  _STAI_NETWORK_EVENT_NODE_START_CB(28, 1, { conv2d_26_output.data->data});
  forward_pw_sssa8_ch_nl_pool(&conv2d_27_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(28, 1, { conv2d_27_output.data->data});
}
void forward_lite_gemm_29(_stai_network_context* net_ctx)
{
  conv2d_27_output_array.data = AI_PTR(net_ctx->_activations[0] + 3840);
  conv2d_27_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 3840);
  gemm_29_weights_array.data = AI_PTR(net_ctx->_weights[54] + 0);
  gemm_29_weights_array.data_start = AI_PTR(net_ctx->_weights[54] + 0);
  gemm_29_bias_array.data = AI_PTR(net_ctx->_weights[55] + 0);
  gemm_29_bias_array.data_start = AI_PTR(net_ctx->_weights[55] + 0);
  gemm_29_scratch0_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  gemm_29_scratch0_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  gemm_29_output_array.data = AI_PTR(net_ctx->_activations[0] + 612);
  gemm_29_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 612);
  _STAI_NETWORK_EVENT_NODE_START_CB(29, 1, { conv2d_27_output.data->data});
  forward_dense_integer_SSSA_ch(&gemm_29_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(29, 1, { gemm_29_output.data->data});
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
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.022468851879239082f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.006852544844150543f, 0.013327695429325104f, 0.008107395842671394f, 0.004613250028342009f, 0.015639806166291237f, 0.004644596017897129f, 0.0031855187844485044f, 0.007543571759015322f);
static const ai_layer_format_type conv2d_1_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 16;

static const ai_u16 conv2d_2_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_2_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_2_t_in_0_shape_ch_const_u16 = 8;
static const ai_u16 conv2d_2_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_2_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_2_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_2_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_2_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_2_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_2_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_2_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_2_t_in_0_fmt_scale_const_f32 = 0.022468851879239082f;
static const ai_float conv2d_2_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_2_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.013815972954034805f, 0.00452336436137557f, 0.009833009913563728f, 0.009330111555755138f, 0.0071484423242509365f, 0.023411910980939865f, 0.007785026449710131f, 0.007347944192588329f);
static const ai_u16 conv2d_2_t_out_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_2_t_out_0_shape_h_const_u16 = 16;

static const ai_u16 conv2d_3_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_3_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_3_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_3_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_3_t_in_0_shape_ch_const_u16 = 8;
static const ai_u16 conv2d_3_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_3_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_3_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_3_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_3_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_3_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0043729133903980255f, 0.006598993670195341f, 0.005569597240537405f, 0.005066356621682644f, 0.005550425965338945f, 0.003948900382965803f, 0.005314261186867952f, 0.004275760613381863f, 0.00526988971978426f, 0.006052085664123297f, 0.008151544257998466f, 0.004693594295531511f, 0.005302724428474903f, 0.006637840531766415f, 0.005311862099915743f, 0.005321362987160683f);
static const ai_layer_format_type conv2d_3_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_4_t_in_0_shape_w_const_u16 = 16;
static const ai_u16 conv2d_4_t_in_0_shape_h_const_u16 = 16;
static const ai_u16 conv2d_4_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_4_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_4_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_4_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_4_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_4_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_4_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_4_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_4_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_4_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_4_t_out_0_fmt_scale_const_f32 = 0.023247789591550827f;
static const ai_float conv2d_4_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004820187110453844f, 0.0048456680960953236f, 0.003069063415750861f, 0.01225936971604824f, 0.0069414968602359295f, 0.01269654743373394f, 0.005359001457691193f, 0.004590670578181744f, 0.0036667408421635628f, 0.003244168823584914f, 0.006216831039637327f, 0.008520593866705894f, 0.008257630281150341f, 0.003614499233663082f, 0.011391675099730492f, 0.0047081876546144485f);
static const ai_u16 conv2d_4_t_out_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_4_t_out_0_shape_h_const_u16 = 8;

static const ai_u16 conv2d_5_t_in_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_5_t_in_0_shape_h_const_u16 = 8;
static const ai_u16 conv2d_5_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_5_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_5_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_5_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_5_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_5_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_5_t_in_0_fmt_scale_const_f32 = 0.023247789591550827f;
static const ai_float conv2d_5_t_out_0_fmt_scale_const_f32 = 0.021480420604348183f;
static const ai_float conv2d_5_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.006972797680646181f, 0.005168210715055466f, 0.00737987132743001f, 0.005957953631877899f, 0.004220485687255859f, 0.006013579200953245f, 0.004766607191413641f, 0.0057290284894406796f, 0.004984266124665737f, 0.004601767286658287f, 0.0053434898145496845f, 0.004456047900021076f, 0.003552018664777279f, 0.005109453573822975f, 0.00792715698480606f, 0.006592164281755686f, 0.005317806266248226f, 0.005087228491902351f, 0.005087732803076506f, 0.005215429700911045f, 0.004781372379511595f, 0.003501621540635824f, 0.004349634051322937f, 0.006369994021952152f, 0.004264618270099163f, 0.003920221235603094f, 0.007141692563891411f, 0.005341107025742531f, 0.005718909669667482f, 0.006105957552790642f, 0.004391053691506386f, 0.005012068897485733f);
static const ai_layer_format_type conv2d_5_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_6_t_in_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_6_t_in_0_shape_h_const_u16 = 8;
static const ai_u16 conv2d_6_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_6_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_6_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_6_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_6_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_6_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_6_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_6_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_6_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_6_t_in_0_fmt_scale_const_f32 = 0.021480420604348183f;
static const ai_float conv2d_6_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_6_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0052336896769702435f, 0.008010043762624264f, 0.008241700939834118f, 0.0049856300465762615f, 0.004533789120614529f, 0.0063702999614179134f, 0.006318368017673492f, 0.007298333570361137f, 0.005908602848649025f, 0.007051325403153896f, 0.006485970225185156f, 0.006999670527875423f, 0.011068466119468212f, 0.007257409859448671f, 0.009020200930535793f, 0.004910928197205067f, 0.009142875671386719f, 0.009477195329964161f, 0.006111476570367813f, 0.006278622895479202f, 0.00867126602679491f, 0.004413452930748463f, 0.004696102812886238f, 0.0068086981773376465f, 0.006332342512905598f, 0.004393964074552059f, 0.0064421165734529495f, 0.005770512390881777f, 0.005561402067542076f, 0.0060984170995652676f, 0.00861423835158348f, 0.0052202059887349606f);
static const ai_u16 conv2d_6_t_out_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_6_t_out_0_shape_h_const_u16 = 8;

static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16 = 8;
static const ai_u16 conv2d_7_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_7_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_7_t_out_0_shape_ch_const_u16 = 32;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32 = 0.020983759313821793f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0028132470324635506f, 0.003458032151684165f, 0.003200532402843237f, 0.00247932062484324f, 0.003186797257512808f, 0.0039730980060994625f, 0.0036175206769257784f, 0.004264238756150007f, 0.003370319027453661f, 0.0033710079733282328f, 0.004531014710664749f, 0.003698607673868537f, 0.0036202678456902504f, 0.0028734563384205103f, 0.0046317195519804955f, 0.00442239735275507f, 0.003665936877951026f, 0.0036448517348617315f, 0.0026428273413330317f, 0.004209382925182581f, 0.002959024626761675f, 0.004777807742357254f, 0.004669602960348129f, 0.0035325740464031696f, 0.004367089830338955f, 0.003518366953358054f, 0.0025723769795149565f, 0.003994711674749851f, 0.0029979220125824213f, 0.0058785914443433285f, 0.004320070147514343f, 0.004922391381114721f);
static const ai_layer_format_type conv2d_7_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16 = 8;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16 = 8;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_8_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_8_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_8_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_8_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_8_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_8_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32 = 0.020983759313821793f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009803001768887043f, 0.006516054272651672f, 0.007366386242210865f, 0.0064140609465539455f, 0.005745260510593653f, 0.006436311174184084f, 0.006965232081711292f, 0.006076007150113583f, 0.009423390962183475f, 0.007973779924213886f, 0.004108888562768698f, 0.008950741030275822f, 0.0055341701954603195f, 0.007035817019641399f, 0.004579434636980295f, 0.010392528027296066f, 0.0054696365259587765f, 0.008456953801214695f, 0.01151605136692524f, 0.00447230227291584f, 0.006825179792940617f, 0.005835077725350857f, 0.007689045276492834f, 0.004934442229568958f, 0.00482507748529315f, 0.0061986809596419334f, 0.007336312439292669f, 0.005441781599074602f, 0.00629199855029583f, 0.00890559982508421f, 0.007656681817024946f, 0.011392605490982533f);
static const ai_u16 conv2d_8_t_out_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_8_t_out_0_shape_h_const_u16 = 4;

static const ai_u16 conv2d_9_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_9_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_9_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_9_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_9_t_in_0_shape_ch_const_u16 = 32;
static const ai_u16 conv2d_9_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_9_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_9_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_9_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_9_t_out_0_fmt_scale_const_f32 = 0.019045090302824974f;
static const ai_float conv2d_9_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0029911964666098356f, 0.0029037552885711193f, 0.003821908263489604f, 0.0032334276475012302f, 0.004279991611838341f, 0.0044357250444591045f, 0.004683046136051416f, 0.0041074249893426895f, 0.0026362368371337652f, 0.005028947256505489f, 0.003896022215485573f, 0.002955363132059574f, 0.0037885995116084814f, 0.004442840348929167f, 0.0035861025098711252f, 0.004192082677036524f, 0.004521027207374573f, 0.003238904755562544f, 0.004051440395414829f, 0.0042398651130497456f, 0.004865291528403759f, 0.003924214746803045f, 0.0032633815426379442f, 0.0024577449075877666f, 0.0027449498884379864f, 0.0025371862575411797f, 0.0032482536043971777f, 0.0027198302559554577f, 0.0036960721481591463f, 0.003939399961382151f, 0.004022601060569286f, 0.003650486236438155f, 0.0033528932835906744f, 0.002642926760017872f, 0.00409993389621377f, 0.002774266293272376f, 0.0045987339690327644f, 0.0033429046161472797f, 0.0028477413579821587f, 0.003977267071604729f, 0.004659267142415047f, 0.004347591195255518f, 0.004287577699869871f, 0.0042811790481209755f, 0.00382928061299026f, 0.0030431258492171764f, 0.0024406379088759422f, 0.004656482487916946f, 0.004068220965564251f, 0.0037529300898313522f, 0.005031042732298374f, 0.004294616170227528f, 0.0026159370318055153f, 0.003377542831003666f, 0.0031866496428847313f, 0.0038610680494457483f, 0.0033702319487929344f, 0.0030356678180396557f, 0.0036869016475975513f, 0.0030228423420339823f, 0.0026217305567115545f, 0.0033946966286748648f, 0.003095383755862713f, 0.0038998457603156567f);
static const ai_layer_format_type conv2d_9_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_10_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_10_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_10_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_10_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_10_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_10_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_10_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_10_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_10_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_10_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_10_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_10_t_in_0_fmt_scale_const_f32 = 0.019045090302824974f;
static const ai_float conv2d_10_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_10_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.006792466156184673f, 0.014080098830163479f, 0.010292332619428635f, 0.006519433576613665f, 0.009930628351867199f, 0.008922144770622253f, 0.009007519111037254f, 0.009496350772678852f, 0.008181552402675152f, 0.009817011654376984f, 0.006657812744379044f, 0.0126880444586277f, 0.010482946410775185f, 0.007620131131261587f, 0.009596800431609154f, 0.0096022579818964f, 0.009044681675732136f, 0.011510216630995274f, 0.008689172565937042f, 0.007782903965562582f, 0.009398270398378372f, 0.007687114644795656f, 0.007286050822585821f, 0.005718224681913853f, 0.011729988269507885f, 0.009908254258334637f, 0.010115842334926128f, 0.009736264124512672f, 0.0075262184254825115f, 0.007103821262717247f, 0.007911293767392635f, 0.008331024087965488f, 0.008835477754473686f, 0.009225682355463505f, 0.006615620106458664f, 0.012987959198653698f, 0.008294668979942799f, 0.00722349202260375f, 0.010670638643205166f, 0.00743120675906539f, 0.006153736263513565f, 0.007386095821857452f, 0.009680382907390594f, 0.006933793891221285f, 0.006643383298069239f, 0.007864133454859257f, 0.006816928740590811f, 0.0074629709124565125f, 0.00780468387529254f, 0.007509545888751745f, 0.008205276913940907f, 0.00673064636066556f, 0.00835674349218607f, 0.006827663630247116f, 0.007838154211640358f, 0.011097070761024952f, 0.011428328230977058f, 0.006271550431847572f, 0.009761420078575611f, 0.006866957526654005f, 0.010878747329115868f, 0.008046545088291168f, 0.005306047387421131f, 0.009302686899900436f);
static const ai_u16 conv2d_10_t_out_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_10_t_out_0_shape_h_const_u16 = 4;

static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_11_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_11_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_11_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.020144855603575706f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0022809351794421673f, 0.002691201865673065f, 0.003887386526912451f, 0.0035569167230278254f, 0.0040665664710104465f, 0.0028737736865878105f, 0.004308436065912247f, 0.004216258879750967f, 0.0025199763476848602f, 0.0031392904929816723f, 0.0030690189450979233f, 0.0026723858900368214f, 0.00339530105702579f, 0.0038128471933305264f, 0.0025786312762647867f, 0.003110866528004408f, 0.0024718500208109617f, 0.003051690524443984f, 0.0022591061424463987f, 0.003522467100992799f, 0.0024931617081165314f, 0.0036708242259919643f, 0.002383370418101549f, 0.0018918667919933796f, 0.003080853493884206f, 0.0032216613180935383f, 0.0025218839291483164f, 0.0028585728723555803f, 0.0034290591720491648f, 0.0034269702155143023f, 0.0026709269732236862f, 0.002750077750533819f, 0.0035764130298048258f, 0.00332855642773211f, 0.003111409954726696f, 0.003178591374307871f, 0.004182867705821991f, 0.0037388841155916452f, 0.003413628553971648f, 0.0032023447565734386f, 0.0020295886788517237f, 0.002377662807703018f, 0.0029816150199621916f, 0.0028890729881823063f, 0.0031690949108451605f, 0.0031699759420007467f, 0.0032625130843371153f, 0.0024014278315007687f, 0.0023874498438090086f, 0.002971659181639552f, 0.0032594697549939156f, 0.003627857891842723f, 0.00406687194481492f, 0.0028741771820932627f, 0.0030649772379547358f, 0.002376149408519268f, 0.003155057318508625f, 0.0021366875153034925f, 0.0031293334905058146f, 0.0032296013087034225f, 0.003245631232857704f, 0.0027090960647910833f, 0.0025738668628036976f, 0.0025469998363405466f);
static const ai_layer_format_type conv2d_11_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_12_t_in_0_shape_w_const_u16 = 4;
static const ai_u16 conv2d_12_t_in_0_shape_h_const_u16 = 4;
static const ai_u16 conv2d_12_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_12_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_12_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_12_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_12_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_12_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_12_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_12_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.020144855603575706f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.020962368696928024f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0066525558941066265f, 0.00880616158246994f, 0.009401276707649231f, 0.007889129221439362f, 0.007826474495232105f, 0.00900974404066801f, 0.008564294315874577f, 0.007784189190715551f, 0.006911479402333498f, 0.006448017433285713f, 0.006068129558116198f, 0.00817833747714758f, 0.008939709514379501f, 0.008091123774647713f, 0.00836976245045662f, 0.00922569539397955f, 0.009813285432755947f, 0.006177371833473444f, 0.011557760648429394f, 0.007799103390425444f, 0.008001689799129963f, 0.006512122228741646f, 0.009474854916334152f, 0.011796095408499241f, 0.006850811652839184f, 0.008879723027348518f, 0.012220405973494053f, 0.005557647440582514f, 0.01028916984796524f, 0.008297746069729328f, 0.00670802453532815f, 0.008245320990681648f, 0.008307932876050472f, 0.005716593004763126f, 0.006525692064315081f, 0.007663360331207514f, 0.006684357300400734f, 0.007967385463416576f, 0.006384190637618303f, 0.007913211360573769f, 0.009607456624507904f, 0.008357370272278786f, 0.009737968444824219f, 0.008032171986997128f, 0.007199869956821203f, 0.004795841872692108f, 0.006862373556941748f, 0.00631835637614131f, 0.009157332591712475f, 0.01081241574138403f, 0.00726171163842082f, 0.0075866649858653545f, 0.008243889547884464f, 0.009049778804183006f, 0.007608476560562849f, 0.011410393752157688f, 0.007441373076289892f, 0.01079505030065775f, 0.00677658012136817f, 0.010105421766638756f, 0.009098177775740623f, 0.006198422517627478f, 0.007861068472266197f, 0.00947615411132574f);
static const ai_u16 conv2d_12_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_12_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_13_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_13_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_13_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_13_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_13_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_13_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_13_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_13_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_13_t_in_0_fmt_scale_const_f32 = 0.020962368696928024f;
static const ai_float conv2d_13_t_out_0_fmt_scale_const_f32 = 0.017926963046193123f;
static const ai_float conv2d_13_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0022630514577031136f, 0.002358559053391218f, 0.0031529124826192856f, 0.002986437873914838f, 0.0027107896748930216f, 0.0034987370017915964f, 0.003749085357412696f, 0.003240615362301469f, 0.0036440661642700434f, 0.0029298632871359587f, 0.002218097448348999f, 0.002593166660517454f, 0.002913254778832197f, 0.0031039349269121885f, 0.002740727737545967f, 0.004019174259155989f, 0.003297356655821204f, 0.003565335413441062f, 0.002165486104786396f, 0.0027653214056044817f, 0.0031113605946302414f, 0.0035244119353592396f, 0.00341171957552433f, 0.003251961898058653f, 0.002726231701672077f, 0.0024939333088696003f, 0.0029669387731701136f, 0.003048836952075362f, 0.003733766032382846f, 0.00270068203099072f, 0.004062561318278313f, 0.0034055893775075674f, 0.0024529751390218735f, 0.002960957121104002f, 0.0037537531461566687f, 0.0025361503940075636f, 0.002957769902423024f, 0.0031919293105602264f, 0.002936100121587515f, 0.002973027992993593f, 0.0031632515601813793f, 0.003083600429818034f, 0.00402559619396925f, 0.0028725459706038237f, 0.003363820491358638f, 0.0025585112161934376f, 0.002573717851191759f, 0.002734292298555374f, 0.004317686893045902f, 0.003672902937978506f, 0.003224788699299097f, 0.004042292013764381f, 0.0032605042215436697f, 0.0031369926873594522f, 0.0028933619614690542f, 0.0029050204902887344f, 0.004601662512868643f, 0.002960558282211423f, 0.0021832336205989122f, 0.0023248265497386456f, 0.003048939863219857f, 0.003007354913279414f, 0.002895411802455783f, 0.003294931026175618f, 0.003099809167906642f, 0.0028026457875967026f, 0.0021732624154537916f, 0.002685034414753318f, 0.00228360528126359f, 0.0027523415628820658f, 0.0032954802736639977f, 0.004027150571346283f, 0.0022806450724601746f, 0.00409667519852519f, 0.002590992720797658f, 0.003324149874970317f, 0.0031670546159148216f, 0.0029892672318965197f, 0.0036871389020234346f, 0.0028922257479280233f, 0.002738368697464466f, 0.0033556739799678326f, 0.002865392714738846f, 0.00261216820217669f, 0.00392841687425971f, 0.0032682926394045353f, 0.002534292172640562f, 0.003434765385463834f, 0.0027560016606003046f, 0.003136461367830634f, 0.0029313054401427507f, 0.003106559393927455f, 0.0032184054143726826f, 0.004700347315520048f, 0.002682093530893326f, 0.003330295206978917f, 0.0026680107694119215f, 0.0027435666415840387f, 0.0032086733262985945f, 0.00321777886711061f, 0.0019160566153004766f, 0.0030803424306213856f, 0.0030086797196418047f, 0.002924287458881736f, 0.002974179806187749f, 0.0033879221882671118f, 0.002602604916319251f, 0.002152610570192337f, 0.0036827705334872007f, 0.003925017546862364f, 0.0023413507733494043f, 0.0028242403641343117f, 0.003782338695600629f, 0.0030724159441888332f, 0.0032411538995802402f, 0.0025108878035098314f, 0.0031106239184737206f, 0.0037687583826482296f, 0.0022739137057214975f, 0.0035594224464148283f, 0.0019768124911934137f, 0.0034292147029191256f, 0.00309857283718884f, 0.0027761030942201614f, 0.0031178127974271774f, 0.0038105985149741173f, 0.003185716224834323f, 0.0031572096049785614f);
static const ai_layer_format_type conv2d_13_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_14_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_14_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_14_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_14_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_14_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_14_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_14_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_14_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_14_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_14_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_14_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_14_t_in_0_fmt_scale_const_f32 = 0.017926963046193123f;
static const ai_float conv2d_14_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_14_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.01222996599972248f, 0.01211050059646368f, 0.012436667457222939f, 0.008070535957813263f, 0.00863801222294569f, 0.008601817302405834f, 0.010926236398518085f, 0.012340869754552841f, 0.016188520938158035f, 0.008231453597545624f, 0.011589174158871174f, 0.009482569992542267f, 0.013851281255483627f, 0.008251420222222805f, 0.010855570435523987f, 0.008704745210707188f, 0.012670875526964664f, 0.01026687677949667f, 0.00944938138127327f, 0.010584830306470394f, 0.011671184562146664f, 0.008650518022477627f, 0.010267040692269802f, 0.00823693536221981f, 0.00923658162355423f, 0.010970021598041058f, 0.008266115561127663f, 0.007604156155139208f, 0.010952342301607132f, 0.01343434490263462f, 0.01009879820048809f, 0.009405715391039848f, 0.005587967578321695f, 0.00785075593739748f, 0.006851872894912958f, 0.012376658618450165f, 0.009819159284234047f, 0.00925258919596672f, 0.010311098769307137f, 0.010596252977848053f, 0.011562081053853035f, 0.009568979032337666f, 0.010298406705260277f, 0.01033875159919262f, 0.010220522992312908f, 0.016709012910723686f, 0.011006559245288372f, 0.012224839068949223f, 0.011067650280892849f, 0.01034737378358841f, 0.014967708848416805f, 0.00869961827993393f, 0.009179797023534775f, 0.01115721371024847f, 0.008137218654155731f, 0.01354874949902296f, 0.011938848532736301f, 0.011371118016541004f, 0.009985441341996193f, 0.011359230615198612f, 0.013115812093019485f, 0.010679647326469421f, 0.013133810833096504f, 0.010102350264787674f, 0.007906477898359299f, 0.01484775822609663f, 0.011686036363244057f, 0.008012698963284492f, 0.011141872964799404f, 0.010600607842206955f, 0.017140259966254234f, 0.014326312579214573f, 0.006744537968188524f, 0.011158812791109085f, 0.007347743958234787f, 0.011146772652864456f, 0.007459222339093685f, 0.00798887200653553f, 0.009752447716891766f, 0.007549183443188667f, 0.010164459235966206f, 0.007777652703225613f, 0.011421947740018368f, 0.008396524004638195f, 0.00698111392557621f, 0.005266119726002216f, 0.012676208280026913f, 0.007005833555012941f, 0.010435637086629868f, 0.010578636080026627f, 0.009303574450314045f, 0.012941427528858185f, 0.010045272298157215f, 0.010593491606414318f, 0.01281725987792015f, 0.012965983711183071f, 0.009104887954890728f, 0.006662596482783556f, 0.010138150304555893f, 0.011666924692690372f, 0.014489802531898022f, 0.009293245151638985f, 0.007521410007029772f, 0.008328861556947231f, 0.013970734551548958f, 0.0110841179266572f, 0.007602479308843613f, 0.011878143064677715f, 0.010398166254162788f, 0.00808602012693882f, 0.008111443370580673f, 0.008312787860631943f, 0.009509324096143246f, 0.012778053991496563f, 0.011223239824175835f, 0.011162425391376019f, 0.008827954530715942f, 0.0103217214345932f, 0.009696133434772491f, 0.009019262157380581f, 0.011972155421972275f, 0.009051494300365448f, 0.013102500699460506f, 0.00929504819214344f, 0.009133540093898773f, 0.008541389368474483f, 0.011287355795502663f, 0.010981901548802853f);
static const ai_u16 conv2d_14_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_14_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_15_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_15_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_15_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_15_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_15_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_15_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_15_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_15_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_15_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_15_t_out_0_fmt_scale_const_f32 = 0.017578106373548508f;
static const ai_float conv2d_15_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0011434437474235892f, 0.001623230753466487f, 0.0019180973758921027f, 0.0023693805560469627f, 0.0026886879932135344f, 0.0029732335824519396f, 0.0015205384697765112f, 0.0025391126982867718f, 0.0029320241883397102f, 0.0020370609126985073f, 0.0022572323214262724f, 0.0030923958402127028f, 0.0017912142211571336f, 0.002375481417402625f, 0.0018370335455983877f, 0.001562628080137074f, 0.0018719749059528112f, 0.002356620505452156f, 0.0018344130367040634f, 0.0018125063506886363f, 0.0020805997774004936f, 0.0018042585579678416f, 0.001354771084152162f, 0.0017725753132253885f, 0.0026027190033346415f, 0.0023246349301189184f, 0.0024867623578757048f, 0.0017155945533886552f, 0.001959904097020626f, 0.0018696514889597893f, 0.002665390959009528f, 0.0018560742028057575f, 0.0020234629046171904f, 0.0020114409271627665f, 0.0020799010526388884f, 0.002690238179638982f, 0.0019610573071986437f, 0.0032831013668328524f, 0.0023203289601951838f, 0.0021224000956863165f, 0.0026661392766982317f, 0.002553915837779641f, 0.0021277929190546274f, 0.002105819061398506f, 0.0021508841309696436f, 0.0020693379919975996f, 0.00215317215770483f, 0.0016957346815615892f, 0.0024497099220752716f, 0.002198750851675868f, 0.0019613769836723804f, 0.002018113387748599f, 0.0018642371287569404f, 0.0022244194988161325f, 0.002514258958399296f, 0.002035941695794463f, 0.0020328806713223457f, 0.002702385652810335f, 0.0017547495663166046f, 0.0021818424575030804f, 0.002541662659496069f, 0.0028082907665520906f, 0.0029962128028273582f, 0.0019150314619764686f, 0.0017260456224903464f, 0.0018249611603096128f, 0.0021780994720757008f, 0.002251123543828726f, 0.0015542112523689866f, 0.002022300846874714f, 0.002499108901247382f, 0.002600331325083971f, 0.002851285273209214f, 0.0022825172636657953f, 0.002347511239349842f, 0.002667391672730446f, 0.0024754779879003763f, 0.0016929738922044635f, 0.0027852219063788652f, 0.001944717369042337f, 0.0022555971518158913f, 0.003051835810765624f, 0.002964099869132042f, 0.0019047025125473738f, 0.0025237170048058033f, 0.0018420291598886251f, 0.0026091348845511675f, 0.0024006986059248447f, 0.002327003749087453f, 0.0027134683914482594f, 0.0021072253584861755f, 0.0017924733692780137f, 0.0023297411389648914f, 0.0022773193195462227f, 0.0018933112733066082f, 0.002556170802563429f, 0.0024403566494584084f, 0.002182974247261882f, 0.002169779734686017f, 0.0025395043194293976f, 0.0020632820669561625f, 0.0014788195258006454f, 0.001653392449952662f, 0.001772611984051764f, 0.0022142771631479263f, 0.002171817235648632f, 0.0020933966152369976f, 0.0017865009140223265f, 0.002148199826478958f, 0.001638151821680367f, 0.00233420985750854f, 0.0017095637740567327f, 0.0018770699389278889f, 0.0026163288857787848f, 0.0022388705983757973f, 0.001896610832773149f, 0.0016092510195448995f, 0.001893316046334803f, 0.002034752629697323f, 0.0017155123641714454f, 0.002394675510004163f, 0.002007537055760622f, 0.0024299316573888063f, 0.002261321758851409f, 0.0022112736478447914f, 0.002094675786793232f, 0.001625457894988358f, 0.0031393191311508417f);
static const ai_layer_format_type conv2d_15_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_16_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_16_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_16_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_16_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_16_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_16_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_16_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_16_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_16_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_16_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_16_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_16_t_in_0_fmt_scale_const_f32 = 0.017578106373548508f;
static const ai_float conv2d_16_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_16_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.010783378966152668f, 0.011588571593165398f, 0.008884914219379425f, 0.006594516336917877f, 0.010147450491786003f, 0.007485031150281429f, 0.010985863395035267f, 0.008505085483193398f, 0.010376489721238613f, 0.008044391870498657f, 0.006374678108841181f, 0.010599867440760136f, 0.010829285718500614f, 0.01202465407550335f, 0.010120516642928123f, 0.006931417156010866f, 0.008709200657904148f, 0.010702026076614857f, 0.007947898469865322f, 0.010707340203225613f, 0.012827309779822826f, 0.009384660050272942f, 0.010694397613406181f, 0.007328249514102936f, 0.009408628568053246f, 0.009205996058881283f, 0.006465844344347715f, 0.005585645791143179f, 0.008555992506444454f, 0.009577793069183826f, 0.008327985182404518f, 0.008696421980857849f, 0.010628646239638329f, 0.008248589001595974f, 0.011134885251522064f, 0.009605169296264648f, 0.009290157817304134f, 0.00849405862390995f, 0.010182219557464123f, 0.008484494872391224f, 0.011879660189151764f, 0.009095112793147564f, 0.00829721987247467f, 0.009088077582418919f, 0.009629582986235619f, 0.008821996860206127f, 0.011893443763256073f, 0.007378242444247007f, 0.009658451192080975f, 0.00994657538831234f, 0.011153881438076496f, 0.00959369819611311f, 0.007930354215204716f, 0.01100348774343729f, 0.009685525670647621f, 0.008167211897671223f, 0.008572561666369438f, 0.00818784348666668f, 0.01065556239336729f, 0.006909843999892473f, 0.010408987291157246f, 0.0126130236312747f, 0.01120485458523035f, 0.009943143464624882f, 0.008726955391466618f, 0.009846709668636322f, 0.008678866550326347f, 0.010338915511965752f, 0.01198960468173027f, 0.008150696754455566f, 0.00956040434539318f, 0.010664244182407856f, 0.01128657627850771f, 0.00819618534296751f, 0.010836447589099407f, 0.012742391787469387f, 0.008994176052510738f, 0.00988045148551464f, 0.009490583091974258f, 0.00788080133497715f, 0.008683056570589542f, 0.010093391872942448f, 0.009776596911251545f, 0.008782070130109787f, 0.008867540396749973f, 0.011736742220818996f, 0.008690323680639267f, 0.011292146518826485f, 0.009247633628547192f, 0.009349505417048931f, 0.00780515419319272f, 0.008450860157608986f, 0.009660671465098858f, 0.024196818470954895f, 0.0084955720230937f, 0.007669206243008375f, 0.009262504056096077f, 0.007181275635957718f, 0.011960848234593868f, 0.010518082417547703f, 0.009118352085351944f, 0.013208200223743916f, 0.010662758722901344f, 0.009083998389542103f, 0.008360872976481915f, 0.009175186045467854f, 0.008546614088118076f, 0.007957356981933117f, 0.010711018927395344f, 0.009442652575671673f, 0.010767211206257343f, 0.0115430299192667f, 0.008922303095459938f, 0.009321969002485275f, 0.01197568979114294f, 0.008182809688150883f, 0.007031781133264303f, 0.011107608675956726f, 0.01131963450461626f, 0.00921811442822218f, 0.008621258661150932f, 0.009563807398080826f, 0.009086745791137218f, 0.010303745046257973f, 0.009188110940158367f, 0.012846076861023903f, 0.007893726229667664f, 0.013460053130984306f);
static const ai_u16 conv2d_16_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_16_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_17_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_17_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_17_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_17_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_17_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_17_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_17_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_17_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_17_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_17_t_out_0_fmt_scale_const_f32 = 0.017774304375052452f;
static const ai_float conv2d_17_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001839947304688394f, 0.002519754460081458f, 0.0021369836758822203f, 0.0014644283801317215f, 0.002272253390401602f, 0.0026993886567652225f, 0.0017601118888705969f, 0.0013982674572616816f, 0.0021304369438439608f, 0.003328664693981409f, 0.0018024984747171402f, 0.0019225606229156256f, 0.0018036194378510118f, 0.002140279393643141f, 0.002566940151154995f, 0.0023076010402292013f, 0.0021020881831645966f, 0.002249709330499172f, 0.001840987941250205f, 0.0021396891679614782f, 0.0017907291185110807f, 0.002031063660979271f, 0.0017221184680238366f, 0.0016648772871121764f, 0.001792242517694831f, 0.0016733729280531406f, 0.0019182289252057672f, 0.002099293516948819f, 0.0015998495509847999f, 0.0025335291866213083f, 0.0017996610840782523f, 0.002139684511348605f, 0.0017454916378483176f, 0.00203084759414196f, 0.002067192457616329f, 0.0021212135907262564f, 0.002085136016830802f, 0.0028278122190386057f, 0.0022196362260729074f, 0.0017124791629612446f, 0.0018723978428170085f, 0.0013459110632538795f, 0.0011564736487343907f, 0.0017085220897570252f, 0.002367090666666627f, 0.0024376215878874063f, 0.0017247186042368412f, 0.002240604953840375f, 0.002597068203613162f, 0.002576279453933239f, 0.00279593956656754f, 0.0028626045677810907f, 0.002587702823802829f, 0.0017097701784223318f, 0.002063188934698701f, 0.00159071059897542f, 0.001836353912949562f, 0.0023249133955687284f, 0.0020009458530694246f, 0.0020135280210524797f, 0.0020381733775138855f, 0.001800446305423975f, 0.002086358843371272f, 0.0019667595624923706f, 0.0019037581514567137f, 0.001953531987965107f, 0.0020761124324053526f, 0.0021905479952692986f, 0.0020055535715073347f, 0.0022241841070353985f, 0.001990740420296788f, 0.0024201213382184505f, 0.0020728197414427996f, 0.0020348825491964817f, 0.002174709690734744f, 0.0018049710197374225f, 0.0020171855576336384f, 0.0021841106936335564f, 0.0021952036768198013f, 0.0018602142808958888f, 0.0024952501989901066f, 0.0018520784797146916f, 0.0029488541185855865f, 0.0018653826555237174f, 0.0017182918963953853f, 0.002527362434193492f, 0.002123722340911627f, 0.0018293812172487378f, 0.0020704041235148907f, 0.0020972422789782286f, 0.002488467376679182f, 0.002362543484196067f, 0.001507549430243671f, 0.0019168228609487414f, 0.0019877138547599316f, 0.0023044415283948183f, 0.001884309109300375f, 0.002094870898872614f, 0.002498605288565159f, 0.002862415974959731f, 0.0016094516031444073f, 0.002092512557283044f, 0.0028046045918017626f, 0.002458929782733321f, 0.002208566991612315f, 0.0023266354110091925f, 0.0016924188239499927f, 0.001990734599530697f, 0.0019698801916092634f, 0.0028858380392193794f, 0.0016559887444600463f, 0.002295405836775899f, 0.002129444619640708f, 0.0031996078323572874f, 0.0023373174481093884f, 0.001734138117171824f, 0.0022421209141612053f, 0.0017780672060325742f, 0.002295892219990492f, 0.0025430158711969852f, 0.001415179343894124f, 0.0019486676901578903f, 0.0025941235944628716f, 0.0026709504891186953f, 0.0022849664092063904f, 0.001725420937873423f, 0.0020159734413027763f, 0.0020036816131323576f);
static const ai_layer_format_type conv2d_17_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_18_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_18_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_18_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_18_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_18_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_18_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_18_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_18_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_18_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_18_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_18_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_18_t_in_0_fmt_scale_const_f32 = 0.017774304375052452f;
static const ai_float conv2d_18_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_18_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.006272327154874802f, 0.008540775626897812f, 0.010815532878041267f, 0.013852126896381378f, 0.011838654056191444f, 0.007517489604651928f, 0.007699511479586363f, 0.008020476438105106f, 0.009515187703073025f, 0.009327816776931286f, 0.008634395897388458f, 0.008576412685215473f, 0.010541724041104317f, 0.0084206722676754f, 0.010309494100511074f, 0.013423442840576172f, 0.007668286096304655f, 0.009134768508374691f, 0.0113324373960495f, 0.009593472816050053f, 0.007821476086974144f, 0.009347816929221153f, 0.007448473479598761f, 0.008573903702199459f, 0.011152288876473904f, 0.008101824671030045f, 0.0071416376158595085f, 0.00860594678670168f, 0.009318701922893524f, 0.008664094842970371f, 0.008861200883984566f, 0.009311851114034653f, 0.009567146189510822f, 0.010160131379961967f, 0.008844793774187565f, 0.010793671943247318f, 0.007312034256756306f, 0.009689537808299065f, 0.011841914616525173f, 0.01067188661545515f, 0.007188924588263035f, 0.011295042000710964f, 0.007578364107757807f, 0.011401102878153324f, 0.007995212450623512f, 0.007493721321225166f, 0.008583823218941689f, 0.00956758763641119f, 0.009181241504848003f, 0.009246809408068657f, 0.007250569760799408f, 0.014166291803121567f, 0.007706234697252512f, 0.008440744131803513f, 0.007773504126816988f, 0.007861120626330376f, 0.011629846878349781f, 0.007686292286962271f, 0.009393897838890553f, 0.007803879678249359f, 0.01002501230686903f, 0.010826285928487778f, 0.00774102658033371f, 0.010403976775705814f, 0.010324266739189625f, 0.007548708003014326f, 0.009549691341817379f, 0.00850666780024767f, 0.009077931754291058f, 0.009480578824877739f, 0.011025923304259777f, 0.010350245982408524f, 0.00864793173968792f, 0.009806143119931221f, 0.010109907947480679f, 0.011181904934346676f, 0.010035207495093346f, 0.007237308192998171f, 0.00894472561776638f, 0.010679514147341251f, 0.010973766446113586f, 0.00957367941737175f, 0.01098322868347168f, 0.009166311472654343f, 0.00900295190513134f, 0.007413254585117102f, 0.008552961982786655f, 0.008679097518324852f, 0.011217162013053894f, 0.00811892282217741f, 0.008596571162343025f, 0.011112716980278492f, 0.009997363202273846f, 0.011696168221533298f, 0.009876077063381672f, 0.007069321349263191f, 0.008882487192749977f, 0.005786341615021229f, 0.00765620730817318f, 0.010070637799799442f, 0.011313032358884811f, 0.013096798211336136f, 0.007771539036184549f, 0.008925477974116802f, 0.010797735303640366f, 0.010203773155808449f, 0.008639749139547348f, 0.00818171352148056f, 0.009506013244390488f, 0.007344071287661791f, 0.009394061751663685f, 0.008327484130859375f, 0.00964087713509798f, 0.010205778293311596f, 0.010278713889420033f, 0.010057327337563038f, 0.007526265922933817f, 0.007349144201725721f, 0.010786564089357853f, 0.009604447521269321f, 0.007180964574217796f, 0.009326769039034843f, 0.008045164868235588f, 0.011355068534612656f, 0.006937138736248016f, 0.009102328680455685f, 0.0075102006085217f, 0.007795145735144615f);
static const ai_u16 conv2d_18_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_18_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_19_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_19_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_19_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_19_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_19_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_19_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_19_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_19_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_19_t_out_0_fmt_scale_const_f32 = 0.018600087612867355f;
static const ai_float conv2d_19_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002154511632397771f, 0.0014027690049260855f, 0.0014111950295045972f, 0.0015719530638307333f, 0.0017721920739859343f, 0.0018368380842730403f, 0.001949690980836749f, 0.0017620202852413058f, 0.0021039394196122885f, 0.001413761987350881f, 0.0025574578903615475f, 0.001807944499887526f, 0.0019037341699004173f, 0.001536693423986435f, 0.002188227605074644f, 0.001217068056575954f, 0.0017321676714345813f, 0.002412986708804965f, 0.0017796121537685394f, 0.0014182786690071225f, 0.001517929369583726f, 0.001923112664371729f, 0.0020860875956714153f, 0.0017503071576356888f, 0.0017441200325265527f, 0.001834770548157394f, 0.0019598740618675947f, 0.0020212826784700155f, 0.0017615712713450193f, 0.0015078497817739844f, 0.0016796786803752184f, 0.002213483676314354f, 0.001442507142201066f, 0.002021604450419545f, 0.0015785237774252892f, 0.0021607077214866877f, 0.0020660164300352335f, 0.0020435580518096685f, 0.0020287055522203445f, 0.0018920403672382236f, 0.0018505211919546127f, 0.001972469501197338f, 0.002050934126600623f, 0.0014235656708478928f, 0.0020884256809949875f, 0.0020747084636241198f, 0.002180657582357526f, 0.0012925781775265932f, 0.0020180149003863335f, 0.0020336697343736887f, 0.0025819300208240747f, 0.002495790831744671f, 0.0012347415322437882f, 0.001563562429510057f, 0.001857772353105247f, 0.002023459644988179f, 0.0016119431238621473f, 0.001700623077340424f, 0.0019347623456269503f, 0.0019747260957956314f, 0.0026727730873972178f, 0.0019620798993855715f, 0.0016252555651590228f, 0.0016597567591816187f, 0.0016066362150013447f, 0.001611693762242794f, 0.0018534546252340078f, 0.0019716639071702957f, 0.001920256414450705f, 0.0014398484490811825f, 0.0023549278266727924f, 0.001964299473911524f, 0.0016710700001567602f, 0.0018463220912963152f, 0.001648218953050673f, 0.0015298734651878476f, 0.0022589738946408033f, 0.0013633263297379017f, 0.0022572134621441364f, 0.001872470835223794f, 0.0019165505655109882f, 0.0014184865867719054f, 0.00149380078073591f, 0.001889859908260405f, 0.0016212661284953356f, 0.0020262098405510187f, 0.00232697743922472f, 0.0021533011458814144f, 0.0019395767012611032f, 0.001614676322788f, 0.0019674724899232388f, 0.0019495306769385934f, 0.0018595822621136904f, 0.0022652626503258944f, 0.0025063124485313892f, 0.002121490892022848f, 0.0021503251045942307f, 0.0015437203692272305f, 0.0015626066597178578f, 0.0020093501079827547f, 0.001885666511952877f, 0.0014544992009177804f, 0.0019396890420466661f, 0.002109537133947015f, 0.00220889481715858f, 0.002162222983315587f, 0.001698243897408247f, 0.0018291554879397154f, 0.0014730331022292376f, 0.0015952344983816147f, 0.0023056259378790855f, 0.0017967144958674908f, 0.0019400842720642686f, 0.0014077414525672793f, 0.0017092862399294972f, 0.0022706405725330114f, 0.0016116672195494175f, 0.00205486873164773f, 0.0016914868028834462f, 0.0012418903643265367f, 0.0014481060206890106f, 0.002222982235252857f, 0.0016969757853075862f, 0.001744278590194881f, 0.0017401379300281405f, 0.0018219432095065713f, 0.0017695885617285967f, 0.0019250427139922976f);
static const ai_layer_format_type conv2d_19_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_20_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_20_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_20_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_20_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_20_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_20_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_20_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_20_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_20_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_20_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_20_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_20_t_in_0_fmt_scale_const_f32 = 0.018600087612867355f;
static const ai_float conv2d_20_t_out_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_20_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009136276319622993f, 0.007777720224112272f, 0.005594375543296337f, 0.009310014545917511f, 0.009156500920653343f, 0.007670718245208263f, 0.007304616738110781f, 0.00791044533252716f, 0.010948073118925095f, 0.007419242057949305f, 0.008464094251394272f, 0.00625604810193181f, 0.006564287468791008f, 0.00881878286600113f, 0.00740820774808526f, 0.007120938040316105f, 0.008749525994062424f, 0.007940488867461681f, 0.01576101966202259f, 0.010422338731586933f, 0.008782876655459404f, 0.008115625940263271f, 0.01099327765405178f, 0.0067352778278291225f, 0.00821574404835701f, 0.008193565532565117f, 0.008436485193669796f, 0.01129154022783041f, 0.0068399603478610516f, 0.00695987232029438f, 0.013550322502851486f, 0.007703639566898346f, 0.012436280958354473f, 0.008931751362979412f, 0.009825807996094227f, 0.009129033423960209f, 0.010631472803652287f, 0.009294578805565834f, 0.007808799389749765f, 0.00820658728480339f, 0.010704715736210346f, 0.007677060551941395f, 0.008454704657196999f, 0.008536207489669323f, 0.007045880891382694f, 0.00951447058469057f, 0.0065193478949368f, 0.006974066141992807f, 0.008465738967061043f, 0.009793980978429317f, 0.007458326872438192f, 0.007472481578588486f, 0.008730090223252773f, 0.006813577376306057f, 0.008035165257751942f, 0.011062171310186386f, 0.008172073401510715f, 0.00856717023998499f, 0.007940733805298805f, 0.007759017404168844f, 0.010300236754119396f, 0.008815915323793888f, 0.008205204270780087f, 0.010511131025850773f, 0.004740032833069563f, 0.0072030858136713505f, 0.008212646469473839f, 0.010495474562048912f, 0.008110038936138153f, 0.008384372107684612f, 0.007969498634338379f, 0.009435970336198807f, 0.010368578135967255f, 0.005778333637863398f, 0.006738458760082722f, 0.007611607201397419f, 0.006267197895795107f, 0.007885335944592953f, 0.010562763549387455f, 0.0094196368008852f, 0.00782522652298212f, 0.006121893413364887f, 0.011245327070355415f, 0.009661332704126835f, 0.008212504908442497f, 0.00892894621938467f, 0.009103900752961636f, 0.00817988533526659f, 0.009129462763667107f, 0.008534282445907593f, 0.010383114218711853f, 0.007398443296551704f, 0.008905967697501183f, 0.009496847167611122f, 0.009188028983771801f, 0.011821973137557507f, 0.010064634494483471f, 0.009056826122105122f, 0.006860018242150545f, 0.007074182853102684f, 0.006740565877407789f, 0.005489857401698828f, 0.010054975748062134f, 0.009636894799768925f, 0.010396789759397507f, 0.00925487745553255f, 0.011055093258619308f, 0.00667826971039176f, 0.00816138181835413f, 0.006507334765046835f, 0.009916025213897228f, 0.008274592459201813f, 0.009233026765286922f, 0.008244482800364494f, 0.009552082978188992f, 0.009185459464788437f, 0.009826441295444965f, 0.009972134605050087f, 0.009353000670671463f, 0.00856082048267126f, 0.008653517812490463f, 0.010041898116469383f, 0.007244549226015806f, 0.008349438197910786f, 0.0093331728130579f, 0.008859938941895962f, 0.012559146620333195f, 0.00812555942684412f);
static const ai_u16 conv2d_20_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_20_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_21_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_21_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_21_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_21_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_21_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_21_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_21_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_21_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_21_t_in_0_fmt_scale_const_f32 = 0.0235294122248888f;
static const ai_float conv2d_21_t_out_0_fmt_scale_const_f32 = 0.01698455587029457f;
static const ai_float conv2d_21_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0014129953924566507f, 0.0018939654109999537f, 0.0018301631789654493f, 0.0016722573200240731f, 0.0012431723298504949f, 0.002191326580941677f, 0.001344883581623435f, 0.0016134935431182384f, 0.001434330944903195f, 0.001053714076988399f, 0.0013854230055585504f, 0.0013625716092064977f, 0.0018464084714651108f, 0.0018595807487145066f, 0.0015075294068083167f, 0.002072748262435198f, 0.0016475486336275935f, 0.0016730421921238303f, 0.0015598877798765898f, 0.002104898216202855f, 0.0012162098428234458f, 0.0013101639924570918f, 0.0016482026549056172f, 0.0024392660707235336f, 0.0016006228979676962f, 0.0013185193529352546f, 0.0013710868079215288f, 0.0012561311013996601f, 0.0016475665615871549f, 0.001334290485829115f, 0.0017190857324749231f, 0.001335600740276277f, 0.0012047452619299293f, 0.0019917963072657585f, 0.0015820655971765518f, 0.0014574677916243672f, 0.0016540100332349539f, 0.001695640617981553f, 0.0015758515801280737f, 0.001557178096845746f, 0.001688872231170535f, 0.0012716202763840556f, 0.0013777964049950242f, 0.002277675084769726f, 0.0018579483730718493f, 0.0016273937653750181f, 0.0015993161359801888f, 0.0013246969319880009f, 0.0011265802895650268f, 0.0014644153416156769f, 0.0013349391520023346f, 0.0013439366593956947f, 0.0020799997728317976f, 0.001729847863316536f, 0.0011195311089977622f, 0.0018970996607095003f, 0.0013174314517527819f, 0.001874722889624536f, 0.001818313030526042f, 0.0010835438733920455f, 0.0014255434507504106f, 0.0023966419976204634f, 0.001797755598090589f, 0.0016860660398378968f, 0.0016173860058188438f, 0.001731647178530693f, 0.0013903251383453608f, 0.002061911392956972f, 0.0013063102960586548f, 0.002059041988104582f, 0.001694832113571465f, 0.0015015192329883575f, 0.0017797349719330668f, 0.0012719500809907913f, 0.0014418944483622909f, 0.001914908760227263f, 0.0013626684667542577f, 0.0012652071891352534f, 0.0015256558544933796f, 0.0012332533951848745f, 0.0018222311045974493f, 0.00116844498552382f, 0.0016932234866544604f, 0.0015147452941164374f, 0.0014666315400972962f, 0.0016756069380789995f, 0.0016343743773177266f, 0.002064711879938841f, 0.0015612614806741476f, 0.001578966504894197f, 0.0014790662098675966f, 0.0013092643348500133f, 0.0019129496067762375f, 0.0016787159256637096f, 0.0019148411229252815f, 0.000993824447505176f, 0.0014376257313415408f, 0.001777565572410822f, 0.001373416162095964f, 0.0015521734021604061f, 0.0010915757156908512f, 0.001623148680664599f, 0.0012510454980656505f, 0.0016699323896318674f, 0.001606289646588266f, 0.0018907585181295872f, 0.0012762389378622174f, 0.0016512571601197124f, 0.0014889902668073773f, 0.0012861053692176938f, 0.0016271776985377073f, 0.0012189332628622651f, 0.0016463701613247395f, 0.0019327208865433931f, 0.0017771811690181494f, 0.001607229933142662f, 0.002161324955523014f, 0.0015218185726553202f, 0.0010463070357218385f, 0.001622825744561851f, 0.0013105805264785886f, 0.0012265041004866362f, 0.0015430075582116842f, 0.0012342658592388034f, 0.0012248465791344643f, 0.0016513620503246784f, 0.0017380737699568272f, 0.0013864219654351473f);
static const ai_layer_format_type conv2d_21_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_22_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_22_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_22_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_22_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_22_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_22_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_22_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_22_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_22_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_22_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_22_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_22_t_in_0_fmt_scale_const_f32 = 0.01698455587029457f;
static const ai_float conv2d_22_t_out_0_fmt_scale_const_f32 = 0.02118281088769436f;
static const ai_float conv2d_22_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.008541169576346874f, 0.009808667004108429f, 0.007660550996661186f, 0.007690938655287027f, 0.0070144422352313995f, 0.008901051245629787f, 0.007517319638282061f, 0.010110837407410145f, 0.007255982141941786f, 0.008338527753949165f, 0.005769051611423492f, 0.009346426464617252f, 0.007946920581161976f, 0.009151726961135864f, 0.00950931292027235f, 0.011518296785652637f, 0.013986370526254177f, 0.008890758268535137f, 0.009193588979542255f, 0.009043670259416103f, 0.009213991463184357f, 0.009021610021591187f, 0.00758133502677083f, 0.00982374045997858f, 0.00988471694290638f, 0.009524469263851643f, 0.0062838951125741005f, 0.0075638871639966965f, 0.0062441714107990265f, 0.010060572996735573f, 0.0064429775811731815f, 0.009486747905611992f, 0.00660720095038414f, 0.008035792037844658f, 0.0074151502922177315f, 0.009050915949046612f, 0.0097099794074893f, 0.010087374597787857f, 0.007282606791704893f, 0.007788836490362883f, 0.009071510285139084f, 0.006946704816073179f, 0.005821308586746454f, 0.009914367459714413f, 0.011130000464618206f, 0.007818333804607391f, 0.00935885775834322f, 0.007922262884676456f, 0.008855700492858887f, 0.00858389213681221f, 0.00809375662356615f, 0.008592776954174042f, 0.009853416122496128f, 0.011061074212193489f, 0.010152037255465984f, 0.009415007196366787f, 0.011289163492619991f, 0.006459526252001524f, 0.007513848599046469f, 0.007448564749211073f, 0.005800215993076563f, 0.010418119840323925f, 0.008422348648309708f, 0.009286900982260704f, 0.008504071272909641f, 0.006528276484459639f, 0.00766719039529562f, 0.01009275857359171f, 0.00907802302390337f, 0.011838909238576889f, 0.0073923757299780846f, 0.008887697011232376f, 0.007126088719815016f, 0.008156447671353817f, 0.008555722422897816f, 0.006662819068878889f, 0.007761201821267605f, 0.007585714105516672f, 0.008639141917228699f, 0.008985248394310474f, 0.007685934659093618f, 0.008240973576903343f, 0.0093748290091753f, 0.005828667897731066f, 0.007786019239574671f, 0.006297428160905838f, 0.007984895259141922f, 0.009036418981850147f, 0.009364301338791847f, 0.008567248471081257f, 0.010169207118451595f, 0.005992593709379435f, 0.010015803389251232f, 0.010844146832823753f, 0.008362730965018272f, 0.015134748071432114f, 0.008534401655197144f, 0.00933346152305603f, 0.011931407265365124f, 0.010196163319051266f, 0.007791753858327866f, 0.0075699156150221825f, 0.009587645530700684f, 0.009654596447944641f, 0.007597371470183134f, 0.008517219685018063f, 0.010661383159458637f, 0.008809144608676434f, 0.009762373752892017f, 0.010647966526448727f, 0.010361774824559689f, 0.0076920147985219955f, 0.006808524951338768f, 0.007009991444647312f, 0.009015806019306183f, 0.007362246513366699f, 0.006487962324172258f, 0.006119520403444767f, 0.00974996667355299f, 0.008713114075362682f, 0.008384826593101025f, 0.010293868370354176f, 0.006120304111391306f, 0.005237339995801449f, 0.007449005264788866f, 0.008200567215681076f, 0.011561607010662556f, 0.006747850216925144f);
static const ai_u16 conv2d_22_t_out_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_22_t_out_0_shape_h_const_u16 = 2;

static const ai_u16 conv2d_23_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_23_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_23_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_23_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_23_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_23_t_out_0_shape_ch_const_u16 = 128;
static const ai_i8 conv2d_23_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_23_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_23_t_in_0_fmt_scale_const_f32 = 0.02118281088769436f;
static const ai_float conv2d_23_t_out_0_fmt_scale_const_f32 = 0.0186606515198946f;
static const ai_float conv2d_23_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0020269553642719984f, 0.0012760505778715014f, 0.0020841078367084265f, 0.001616003573872149f, 0.0015412109205499291f, 0.002140021650120616f, 0.0020615775138139725f, 0.0017895547207444906f, 0.001200172700919211f, 0.0021499262657016516f, 0.0011973385699093342f, 0.0014508343301713467f, 0.0017293166602030396f, 0.0013728296617045999f, 0.0013328862842172384f, 0.0016500531928613782f, 0.0010488219559192657f, 0.0012218764750286937f, 0.001075333566404879f, 0.0014505592407658696f, 0.0010496265022084117f, 0.0016083766240626574f, 0.001818906283006072f, 0.0016276268288493156f, 0.0010480659548193216f, 0.0010952868033200502f, 0.001532766968011856f, 0.0009370455518364906f, 0.0012114110868424177f, 0.0023955777287483215f, 0.0013406825019046664f, 0.00136176822707057f, 0.002019047737121582f, 0.001561628538183868f, 0.0008562710136175156f, 0.002129719126969576f, 0.0017353394068777561f, 0.0011442125542089343f, 0.001355415559373796f, 0.0021065296605229378f, 0.001506802043877542f, 0.00156301015522331f, 0.0015330006135627627f, 0.0010269485646858811f, 0.0011695565190166235f, 0.0011790238786488771f, 0.0013086739927530289f, 0.0016847512451931834f, 0.0009382703574374318f, 0.0013067257823422551f, 0.0018882513977587223f, 0.001438533770851791f, 0.001596859423443675f, 0.001382610178552568f, 0.0012113554403185844f, 0.0013078043702989817f, 0.001798907294869423f, 0.0013851457042619586f, 0.001254635164514184f, 0.0011809171410277486f, 0.0014614027459174395f, 0.0009979954920709133f, 0.0018920127768069506f, 0.0017053971532732248f, 0.0021506003104150295f, 0.0015193586004897952f, 0.0016177892684936523f, 0.0019504270749166608f, 0.0017452503088861704f, 0.0018979867454618216f, 0.0010001900373026729f, 0.0013995675835758448f, 0.002040631603449583f, 0.0015234769089147449f, 0.0011677206493914127f, 0.0011217229766771197f, 0.001178254489786923f, 0.0016934600425884128f, 0.0020279884338378906f, 0.0013103301171213388f, 0.0010496765607967973f, 0.0014348953263834119f, 0.001676298095844686f, 0.0009851865470409393f, 0.0015131636755540967f, 0.0023444518446922302f, 0.0011926352744922042f, 0.001432999619282782f, 0.001101472764275968f, 0.001890147919766605f, 0.0018150497926399112f, 0.0017525233561173081f, 0.001067768200300634f, 0.0012418560218065977f, 0.0015508146025240421f, 0.0015945476479828358f, 0.001089511439204216f, 0.0012876421678811312f, 0.0018096005078405142f, 0.00235627219080925f, 0.0015571687836199999f, 0.001800051424652338f, 0.0018989434465765953f, 0.0019382756436243653f, 0.001066315220668912f, 0.0016809928929433227f, 0.00160182174295187f, 0.0016697755781933665f, 0.0019304138841107488f, 0.0012866755714640021f, 0.001142716035246849f, 0.0012627403484657407f, 0.0019244843861088157f, 0.002014894038438797f, 0.0011466925498098135f, 0.0016537067713215947f, 0.0015778521774336696f, 0.0010478904005140066f, 0.001250625355169177f, 0.0010361034655943513f, 0.0016267988830804825f, 0.0016420732717961073f, 0.0011587832123041153f, 0.0010315054096281528f, 0.001707608811557293f, 0.0015536390710622072f, 0.0012439501006156206f, 0.0014411816373467445f);
static const ai_layer_format_type conv2d_23_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_24_t_in_0_shape_w_const_u16 = 2;
static const ai_u16 conv2d_24_t_in_0_shape_h_const_u16 = 2;
static const ai_u16 conv2d_24_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_24_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_24_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_24_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_24_l_pad_H_0_const_s32 = 0;
static const ai_u16 conv2d_24_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_24_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_24_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_24_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_24_t_in_0_fmt_scale_const_f32 = 0.0186606515198946f;
static const ai_float conv2d_24_t_out_0_fmt_scale_const_f32 = 0.012933306396007538f;
static const ai_float conv2d_24_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0060597495175898075f, 0.01058166939765215f, 0.005769914481788874f, 0.008035111241042614f, 0.006589884869754314f, 0.006958481855690479f, 0.006551348138600588f, 0.006656527519226074f, 0.007978860288858414f, 0.006296499632298946f, 0.008519751019775867f, 0.00802258588373661f, 0.006869744509458542f, 0.006376603618264198f, 0.0064476365223526955f, 0.009585968218743801f, 0.006988536566495895f, 0.00851499754935503f, 0.006875002756714821f, 0.008683402091264725f, 0.01014773454517126f, 0.008508647792041302f, 0.006729488726705313f, 0.006574575323611498f, 0.009531673043966293f, 0.008879557251930237f, 0.006593703757971525f, 0.013845372013747692f, 0.00862892996519804f, 0.006614202633500099f, 0.007761445362120867f, 0.00988094974309206f, 0.0069196876138448715f, 0.0065109399147331715f, 0.010513111017644405f, 0.006736071780323982f, 0.007501809857785702f, 0.005432419944554567f, 0.009049338288605213f, 0.007072113454341888f, 0.006994468159973621f, 0.006584742572158575f, 0.006967220455408096f, 0.006730422843247652f, 0.007819193415343761f, 0.011623840779066086f, 0.007787351496517658f, 0.007609488442540169f, 0.012090837582945824f, 0.007999141700565815f, 0.007396440487354994f, 0.00862390361726284f, 0.00540482671931386f, 0.008601210080087185f, 0.006821487098932266f, 0.006705381441861391f, 0.007067540660500526f, 0.006687935441732407f, 0.011950170621275902f, 0.009158956818282604f, 0.0076651801355183125f, 0.012108329683542252f, 0.00754563556984067f, 0.008483179844915867f, 0.008138179779052734f, 0.008607183583080769f, 0.0069764163345098495f, 0.00681933481246233f, 0.006311909761279821f, 0.009568225592374802f, 0.008793273009359837f, 0.007953997701406479f, 0.005997851025313139f, 0.011266034096479416f, 0.010790997184813023f, 0.01311578694730997f, 0.00868691224604845f, 0.00738394632935524f, 0.005795920733362436f, 0.006666804198175669f, 0.008745173923671246f, 0.006789936684072018f, 0.0068297795951366425f, 0.009695831686258316f, 0.00792266521602869f, 0.008116207085549831f, 0.009361561387777328f, 0.00884736143052578f, 0.008509206585586071f, 0.008342663757503033f, 0.00694238068535924f, 0.00765948137268424f, 0.00833838526159525f, 0.007330047897994518f, 0.006441665347665548f, 0.00835628155618906f, 0.010287173092365265f, 0.006944949273020029f, 0.00777782779186964f, 0.006805818993598223f, 0.006328204646706581f, 0.008065973408520222f, 0.00705817760899663f, 0.007219634018838406f, 0.007142419461160898f, 0.008630993776023388f, 0.007011423818767071f, 0.007883412763476372f, 0.005549958441406488f, 0.009048310108482838f, 0.008689012378454208f, 0.006258276756852865f, 0.0068542687222361565f, 0.007229838520288467f, 0.00711323507130146f, 0.006886023562401533f, 0.007706066593527794f, 0.00751333124935627f, 0.008819020353257656f, 0.014553901739418507f, 0.008627518080174923f, 0.013888062909245491f, 0.006533985026180744f, 0.008408005349338055f, 0.008189327083528042f, 0.008908006362617016f, 0.009005816653370857f, 0.008698388934135437f);
static const ai_u16 conv2d_24_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_24_t_out_0_shape_h_const_u16 = 1;

static const ai_u16 conv2d_25_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_25_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_25_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_25_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_25_t_in_0_shape_ch_const_u16 = 128;
static const ai_u16 conv2d_25_t_out_0_shape_ch_const_u16 = 256;
static const ai_i8 conv2d_25_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_25_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_25_t_in_0_fmt_scale_const_f32 = 0.012933306396007538f;
static const ai_float conv2d_25_t_out_0_fmt_scale_const_f32 = 0.016210265457630157f;
static const ai_float conv2d_25_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0014809818239882588f, 0.0012633055448532104f, 0.0010863543720915914f, 0.0012172837741672993f, 0.001150469877757132f, 0.0012710770824924111f, 0.002155160531401634f, 0.0017920681275427341f, 0.0014529903419315815f, 0.0028683790005743504f, 0.001293333014473319f, 0.0012389729963615537f, 0.0010387124493718147f, 0.002325079869478941f, 0.001709850155748427f, 0.0015035626711323857f, 0.0012456257827579975f, 0.0012443576706573367f, 0.0011445115087553859f, 0.001792309107258916f, 0.0009536563884466887f, 0.0012063410831615329f, 0.0029542928095906973f, 0.0015850476920604706f, 0.0011230043601244688f, 0.001093488885089755f, 0.0010891990968957543f, 0.0009896450210362673f, 0.0012893121456727386f, 0.0011451010359451175f, 0.0017838635249063373f, 0.0013051594141870737f, 0.001090259407646954f, 0.0018520299345254898f, 0.0012761533726006746f, 0.0022457309532910585f, 0.0010983555112034082f, 0.0017187363700941205f, 0.0013690731720998883f, 0.0012856157263740897f, 0.0011946350568905473f, 0.001134911086410284f, 0.0009984213393181562f, 0.0013082106597721577f, 0.003677008207887411f, 0.001053282176144421f, 0.0010750951478257775f, 0.0016546882688999176f, 0.001404997194185853f, 0.001375315710902214f, 0.0013344207545742393f, 0.0018454364035278559f, 0.0015097042778506875f, 0.0033560427837073803f, 0.0017043969128280878f, 0.001065972843207419f, 0.0013006058288738132f, 0.0015123648336157203f, 0.001010462874546647f, 0.0013892771676182747f, 0.0021729578729718924f, 0.0013624350540339947f, 0.0015923819737508893f, 0.000902979401871562f, 0.00100347597617656f, 0.0010823989287018776f, 0.0016259351978078485f, 0.0012231181608512998f, 0.0011959095718339086f, 0.001157767022959888f, 0.003261273493990302f, 0.0012172620045021176f, 0.0013468877878040075f, 0.001155670965090394f, 0.0010268097976222634f, 0.0011646845377981663f, 0.0013050762936472893f, 0.0012367794988676906f, 0.0014286759542301297f, 0.0018084958428516984f, 0.003400962334126234f, 0.0014988428447395563f, 0.001738429651595652f, 0.0015079538570716977f, 0.0014002626994624734f, 0.0011502174893394113f, 0.0011458799708634615f, 0.0008954198565334082f, 0.0011257145088165998f, 0.0015295201446861029f, 0.001104481634683907f, 0.0013185160933062434f, 0.0023297639563679695f, 0.0028995820321142673f, 0.0012935773702338338f, 0.0018471046350896358f, 0.0015435366658493876f, 0.0020483063999563456f, 0.0012107706861570477f, 0.001461433945223689f, 0.0011358654592186213f, 0.0012105741770938039f, 0.001132169389165938f, 0.0016152056632563472f, 0.0009825887391343713f, 0.0021404754370450974f, 0.001576440641656518f, 0.001250801607966423f, 0.000955434050410986f, 0.001846157480031252f, 0.0009989625541493297f, 0.0010342124151065946f, 0.001162065425887704f, 0.0011781022185459733f, 0.0015777332009747624f, 0.0020985740702599287f, 0.001170358038507402f, 0.0010816451394930482f, 0.0012236764887347817f, 0.0011388665297999978f, 0.0012768299784511328f, 0.0014743618667125702f, 0.0013407488586381078f, 0.0012798149837180972f, 0.0011909521417692304f, 0.0009400541894137859f, 0.0011551269562914968f, 0.0031533928122371435f, 0.0012057730928063393f, 0.001454432145692408f, 0.001358564360998571f, 0.0013645822182297707f, 0.0014151608338579535f, 0.0011780635686591268f, 0.0013262260472401977f, 0.0013906746171414852f, 0.0010375111596658826f, 0.001099203946068883f, 0.0013439025497063994f, 0.000982539146207273f, 0.0013835954014211893f, 0.001319761504419148f, 0.0009840857237577438f, 0.0008969288319349289f, 0.001277839532122016f, 0.001249491237103939f, 0.0014275083085522056f, 0.0014533346984535456f, 0.0011315159499645233f, 0.0009778734529390931f, 0.0022958002518862486f, 0.0017328838584944606f, 0.0016188147710636258f, 0.0012311285827308893f, 0.0013464344665408134f, 0.0015764448326081038f, 0.0011584621388465166f, 0.001382450107485056f, 0.002652872586622834f, 0.0011149767087772489f, 0.0009895031107589602f, 0.001229611225426197f, 0.0009851198410615325f, 0.001152983051724732f, 0.0012056196574121714f, 0.0010081047657877207f, 0.0013538622297346592f, 0.0015065675834193826f, 0.001236337935552001f, 0.0011692168191075325f, 0.001863774610683322f, 0.001388993114233017f, 0.00125155970454216f, 0.0031546575482934713f, 0.0015815665246918797f, 0.0019051118288189173f, 0.001007079379633069f, 0.001956360414624214f, 0.001006228500045836f, 0.0017470229649916291f, 0.0016118795610964298f, 0.0012440333375707269f, 0.0021344523411244154f, 0.0022336505353450775f, 0.001629036501981318f, 0.0010271158535033464f, 0.0013858306920155883f, 0.0015745158307254314f, 0.0009147686650976539f, 0.002560314256697893f, 0.0034305022563785315f, 0.0012963617919012904f, 0.001251988229341805f, 0.0012786153238266706f, 0.0011724472278729081f, 0.0012569752288982272f, 0.001570430351421237f, 0.002653515664860606f, 0.0016142013482749462f, 0.001841299352236092f, 0.0016180954407900572f, 0.0011226133210584521f, 0.0015438729897141457f, 0.0014250290114432573f, 0.00121352169662714f, 0.0018381262198090553f, 0.003033319255337119f, 0.001776076969690621f, 0.001388555858284235f, 0.0012652860023081303f, 0.0013305742759257555f, 0.0013058772310614586f, 0.0013491741847246885f, 0.0013494587037712336f, 0.0013023774372413754f, 0.0010365064954385161f, 0.0009670268627814949f, 0.0018011917127296329f, 0.001408440642990172f, 0.0011613384122028947f, 0.0012356933439150453f, 0.001054431777447462f, 0.0012083343463018537f, 0.0009753862977959216f, 0.0013889126712456346f, 0.002368025481700897f, 0.002158515155315399f, 0.001443875371478498f, 0.00113563088234514f, 0.0016905704978853464f, 0.0018527917563915253f, 0.0013832057593390346f, 0.0009901599260047078f, 0.0012120736064389348f, 0.001539506483823061f, 0.0011123038129881024f, 0.001571284024976194f, 0.0011105177691206336f, 0.0012474635150283575f, 0.0011782229412347078f, 0.001120732631534338f, 0.0010857960442081094f, 0.002009538933634758f, 0.001158880884759128f, 0.0012404818553477526f, 0.0012928583892062306f, 0.0032664944883435965f, 0.0013313581002876163f, 0.0015105648199096322f, 0.0014867066638544202f, 0.0014457630459219217f, 0.0010498297633603215f, 0.0011862657265737653f, 0.0016929225530475378f, 0.0012004729360342026f, 0.001450581243261695f);
static const ai_layer_format_type conv2d_25_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_26_t_in_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_26_t_in_0_shape_h_const_u16 = 1;
static const ai_u16 conv2d_26_t_in_0_shape_ch_const_u16 = 256;
static const ai_u16 conv2d_26_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_26_t_weight_0_shape_h_const_u16 = 3;
static const ai_i32 conv2d_26_l_pad_W_0_const_s32 = 1;
static const ai_i32 conv2d_26_l_pad_H_0_const_s32 = 1;
static const ai_u16 conv2d_26_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_26_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_26_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_26_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_26_t_in_0_fmt_scale_const_f32 = 0.016210265457630157f;
static const ai_float conv2d_26_t_out_0_fmt_scale_const_f32 = 0.011937644332647324f;
static const ai_float conv2d_26_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.011387837119400501f, 0.01139313355088234f, 0.009305098094046116f, 0.009622406214475632f, 0.011164497584104538f, 0.011410237289965153f, 0.010190561413764954f, 0.009326949715614319f, 0.010233579203486443f, 0.008674299344420433f, 0.01039495412260294f, 0.01142994873225689f, 0.007104624528437853f, 0.010873612947762012f, 0.009566327556967735f, 0.010130376555025578f, 0.011520727537572384f, 0.008088492788374424f, 0.008160942234098911f, 0.010180761106312275f, 0.00847331341356039f, 0.009143206290900707f, 0.010494283400475979f, 0.011829376220703125f, 0.008837639354169369f, 0.010457063093781471f, 0.012578888796269894f, 0.010596726089715958f, 0.010739263147115707f, 0.008347929455339909f, 0.010457885451614857f, 0.011826226487755775f, 0.008480723015964031f, 0.010713777504861355f, 0.010578778572380543f, 0.011285456828773022f, 0.010127931833267212f, 0.010780963115394115f, 0.01332132425159216f, 0.010867415927350521f, 0.00839743111282587f, 0.009954430162906647f, 0.010214706882834435f, 0.012859351001679897f, 0.012271815910935402f, 0.008787710219621658f, 0.010377089492976665f, 0.011968860402703285f, 0.010320540517568588f, 0.012360460124909878f, 0.007637979928404093f, 0.012577135115861893f, 0.00943087413907051f, 0.010238061659038067f, 0.012076056562364101f, 0.0104109151288867f, 0.008859475143253803f, 0.010021165944635868f, 0.008544519543647766f, 0.009889530949294567f, 0.011067344807088375f, 0.010728588327765465f, 0.006718062795698643f, 0.00910240225493908f, 0.01055911649018526f, 0.01569102518260479f, 0.00907079130411148f, 0.010017561726272106f, 0.00981497298926115f, 0.011011742986738682f, 0.011204598471522331f, 0.008222614414989948f, 0.008726637810468674f, 0.013098572380840778f, 0.009782386012375355f, 0.015168446116149426f, 0.010166188701987267f, 0.00913141667842865f, 0.009087786078453064f, 0.011413956992328167f, 0.010260554030537605f, 0.010637976229190826f, 0.009471246972680092f, 0.010967320762574673f, 0.010535593144595623f, 0.008848203346133232f, 0.010814131237566471f, 0.010607771575450897f, 0.009556456468999386f, 0.011011864989995956f, 0.011204534210264683f, 0.010135437361896038f, 0.011905121617019176f, 0.010479940101504326f, 0.010524221695959568f, 0.010828820057213306f, 0.010879953391849995f, 0.009347638115286827f, 0.011486456729471684f, 0.012239261530339718f, 0.008385959081351757f, 0.013196155428886414f, 0.010561169125139713f, 0.010423097759485245f, 0.011114311404526234f, 0.010719899088144302f, 0.01282176561653614f, 0.009229276329278946f, 0.0093472795560956f, 0.009354454465210438f, 0.011072259396314621f, 0.009151780977845192f, 0.01079551875591278f, 0.011110489256680012f, 0.010199107229709625f, 0.012068812735378742f, 0.008621892891824245f, 0.011884489096701145f, 0.00859951600432396f, 0.010145897977054119f, 0.008559281937777996f, 0.011255582794547081f, 0.010463985614478588f, 0.010502371937036514f, 0.006616849917918444f, 0.008636724203824997f, 0.011649886146187782f, 0.013545388355851173f, 0.008940658532083035f, 0.010290914215147495f, 0.009427545592188835f, 0.010854309424757957f, 0.01203156914561987f, 0.010026375763118267f, 0.013837800361216068f, 0.0066034686751663685f, 0.008777663111686707f, 0.013694042339920998f, 0.01018428709357977f, 0.010166826657950878f, 0.010110089555382729f, 0.010841483250260353f, 0.009766574949026108f, 0.009752150624990463f, 0.010488591156899929f, 0.013015988282859325f, 0.009768053889274597f, 0.01113853044807911f, 0.010195541195571423f, 0.01018732413649559f, 0.01210641860961914f, 0.010581400245428085f, 0.010665535926818848f, 0.008038822561502457f, 0.010541979223489761f, 0.006249431520700455f, 0.008143979124724865f, 0.008925752714276314f, 0.012204134836792946f, 0.010424572043120861f, 0.009669610299170017f, 0.010700362734496593f, 0.008151175454258919f, 0.007981628179550171f, 0.009661342948675156f, 0.010984724387526512f, 0.00953897088766098f, 0.009288170374929905f, 0.01024703774601221f, 0.009064039215445518f, 0.010145120322704315f, 0.014803149737417698f, 0.014499503187835217f, 0.011385896243155003f, 0.009532787837088108f, 0.011076729744672775f, 0.011295811273157597f, 0.022768450900912285f, 0.008562388829886913f, 0.008951636962592602f, 0.010455171577632427f, 0.009705486707389355f, 0.009543685242533684f, 0.01120366994291544f, 0.011561705730855465f, 0.00849752128124237f, 0.009966608136892319f, 0.009884018450975418f, 0.009779510088264942f, 0.011619298718869686f, 0.011152289807796478f, 0.0093293571844697f, 0.009250418283045292f, 0.009832451120018959f, 0.009487581439316273f, 0.010379668325185776f, 0.009759421460330486f, 0.008534587919712067f, 0.013259421102702618f, 0.009530974552035332f, 0.009098943322896957f, 0.009600719437003136f, 0.010761336423456669f, 0.009479016996920109f, 0.008932828903198242f, 0.014446133747696877f, 0.010926629416644573f, 0.008980518206954002f, 0.013402659446001053f, 0.01634868048131466f, 0.010891848243772984f, 0.011676504276692867f, 0.009791234508156776f, 0.010130444541573524f, 0.009520531632006168f, 0.0092951450496912f, 0.007028323598206043f, 0.010058688931167126f, 0.015468044206500053f, 0.010452764108777046f, 0.00934858713299036f, 0.010033484548330307f, 0.009830751456320286f, 0.010815218091011047f, 0.008917044848203659f, 0.012031513266265392f, 0.011110039427876472f, 0.007688191719353199f, 0.009698047302663326f, 0.008986645378172398f, 0.011974005959928036f, 0.010187686420977116f, 0.008849022909998894f, 0.010537841357290745f, 0.010952931828796864f, 0.008284307084977627f, 0.010796956717967987f, 0.009505173191428185f, 0.015170233324170113f, 0.009561631828546524f, 0.011530623771250248f, 0.010908658616244793f, 0.010867057368159294f, 0.011579080484807491f, 0.011691092513501644f, 0.009008311666548252f, 0.011785370297729969f, 0.01453437004238367f, 0.01217861007899046f, 0.009697696194052696f, 0.01026145275682211f, 0.011245334520936012f, 0.009657229296863079f, 0.008752312511205673f, 0.009916127659380436f, 0.012068998999893665f);
static const ai_u16 conv2d_26_t_out_0_shape_w_const_u16 = 1;
static const ai_u16 conv2d_26_t_out_0_shape_h_const_u16 = 1;



static const ai_u32 nl_30_t_in_0_shape_ch_prod_const_u32 = 10;

static const ai_u32 conversion_31_t_out_0_shape_h_w_ch_d_prod_const_u32 = 10;
static const ai_float conversion_31_t_in_0_fmt_scale_const_f32 = 0.00390625f;
static const ai_i8 conversion_31_t_in_0_fmt_zero_const_s8 = -128;
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
    ai_i8* conv2d_3_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2348);
    ai_i16* conv2d_3_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr) conv2d_3_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_3_t_in_0_ptr_const_s8, conv2d_3_t_in_0_shape_w_const_u16, conv2d_3_t_in_0_shape_h_const_u16, conv2d_3_l_stride_1_const_u16, conv2d_3_l_stride_0_const_u16, conv2d_3_t_in_0_shape_ch_const_u16, conv2d_3_t_weight_0_ptr_const_s8, conv2d_3_t_out_0_shape_ch_const_u16, conv2d_3_t_weight_1_ptr_const_s32, conv2d_3_t_in_0_fmt_zero_const_s8, conv2d_3_t_out_0_fmt_zero_const_s8, conv2d_3_t_in_0_fmt_scale_const_f32, conv2d_3_t_out_0_fmt_scale_const_f32, conv2d_3_t_weight_0_fmt_scale_const_f32, conv2d_3_l_out_ch_format_const_layer_format_type, conv2d_3_t_out_0_ptr_s8, 1, 192, conv2d_3_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr) conv2d_3_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_3 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_4 */
  {
      const ai_i8* conv2d_4_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2348);
    const ai_i8* conv2d_4_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[6] + 0);
    const ai_i32* conv2d_4_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[7] + 0);
    ai_i8* conv2d_4_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 596);
    ai_i16* conv2d_4_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) conv2d_4_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_4_t_in_0_ptr_const_s8, conv2d_4_t_in_0_shape_w_const_u16, conv2d_4_t_in_0_shape_h_const_u16, conv2d_4_t_in_0_shape_ch_const_u16, conv2d_4_t_weight_0_ptr_const_s8, conv2d_4_t_weight_0_shape_w_const_u16, conv2d_4_t_weight_0_shape_h_const_u16, conv2d_4_l_pad_W_0_const_s32, conv2d_4_l_pad_H_0_const_s32, conv2d_4_l_stride_1_const_u16, conv2d_4_l_stride_0_const_u16, conv2d_4_t_weight_1_ptr_const_s32, conv2d_4_t_in_0_fmt_zero_const_s8, conv2d_4_t_out_0_fmt_zero_const_s8, conv2d_4_t_in_0_fmt_scale_const_f32, conv2d_4_t_out_0_fmt_scale_const_f32, conv2d_4_t_weight_0_fmt_scale_const_f32, conv2d_4_t_out_0_ptr_s8, conv2d_4_t_out_0_shape_w_const_u16, conv2d_4_t_out_0_shape_h_const_u16, 0, 593, conv2d_4_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) conv2d_4_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_4 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5 */
  {
      const ai_i8* conv2d_5_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 596);
    const ai_i8* conv2d_5_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[8] + 0);
    const ai_i32* conv2d_5_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[9] + 0);
    ai_i8* conv2d_5_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1620);
    ai_i16* conv2d_5_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_5_t_in_0_ptr_const_s8, conv2d_5_t_in_0_shape_w_const_u16, conv2d_5_t_in_0_shape_h_const_u16, conv2d_5_l_stride_1_const_u16, conv2d_5_l_stride_0_const_u16, conv2d_5_t_in_0_shape_ch_const_u16, conv2d_5_t_weight_0_ptr_const_s8, conv2d_5_t_out_0_shape_ch_const_u16, conv2d_5_t_weight_1_ptr_const_s32, conv2d_5_t_in_0_fmt_zero_const_s8, conv2d_5_t_out_0_fmt_zero_const_s8, conv2d_5_t_in_0_fmt_scale_const_f32, conv2d_5_t_out_0_fmt_scale_const_f32, conv2d_5_t_weight_0_fmt_scale_const_f32, conv2d_5_l_out_ch_format_const_layer_format_type, conv2d_5_t_out_0_ptr_s8, 1, 384, conv2d_5_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_5 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_6 */
  {
      const ai_i8* conv2d_6_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1620);
    const ai_i8* conv2d_6_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[10] + 0);
    const ai_i32* conv2d_6_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[11] + 0);
    ai_i8* conv2d_6_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 3668);
    ai_i16* conv2d_6_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 1, {(stai_ptr) conv2d_6_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_6_t_in_0_ptr_const_s8, conv2d_6_t_in_0_shape_w_const_u16, conv2d_6_t_in_0_shape_h_const_u16, conv2d_6_t_in_0_shape_ch_const_u16, conv2d_6_t_weight_0_ptr_const_s8, conv2d_6_t_weight_0_shape_w_const_u16, conv2d_6_t_weight_0_shape_h_const_u16, conv2d_6_l_pad_W_0_const_s32, conv2d_6_l_pad_H_0_const_s32, conv2d_6_l_stride_1_const_u16, conv2d_6_l_stride_0_const_u16, conv2d_6_t_weight_1_ptr_const_s32, conv2d_6_t_in_0_fmt_zero_const_s8, conv2d_6_t_out_0_fmt_zero_const_s8, conv2d_6_t_in_0_fmt_scale_const_f32, conv2d_6_t_out_0_fmt_scale_const_f32, conv2d_6_t_weight_0_fmt_scale_const_f32, conv2d_6_t_out_0_ptr_s8, conv2d_6_t_out_0_shape_w_const_u16, conv2d_6_t_out_0_shape_h_const_u16, 0, 1185, conv2d_6_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, {(stai_ptr) conv2d_6_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
  {
      const ai_i8* conv2d_7_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 3668);
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
    ai_i8* conv2d_8_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 3684);
    ai_i16* conv2d_8_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2496);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16, conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_t_in_0_shape_ch_const_u16, conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_weight_0_shape_w_const_u16, conv2d_8_t_weight_0_shape_h_const_u16, conv2d_8_l_pad_W_0_const_s32, conv2d_8_l_pad_H_0_const_s32, conv2d_8_l_stride_1_const_u16, conv2d_8_l_stride_0_const_u16, conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8, conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32, conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32, conv2d_8_t_out_0_ptr_s8, conv2d_8_t_out_0_shape_w_const_u16, conv2d_8_t_out_0_shape_h_const_u16, 0, 1185, conv2d_8_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_8 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_9 */
  {
      const ai_i8* conv2d_9_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 3684);
    const ai_i8* conv2d_9_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[16] + 0);
    const ai_i32* conv2d_9_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[17] + 0);
    ai_i8* conv2d_9_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 768);
    ai_i16* conv2d_9_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_9_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_9_t_in_0_ptr_const_s8, conv2d_9_t_in_0_shape_w_const_u16, conv2d_9_t_in_0_shape_h_const_u16, conv2d_9_l_stride_1_const_u16, conv2d_9_l_stride_0_const_u16, conv2d_9_t_in_0_shape_ch_const_u16, conv2d_9_t_weight_0_ptr_const_s8, conv2d_9_t_out_0_shape_ch_const_u16, conv2d_9_t_weight_1_ptr_const_s32, conv2d_9_t_in_0_fmt_zero_const_s8, conv2d_9_t_out_0_fmt_zero_const_s8, conv2d_9_t_in_0_fmt_scale_const_f32, conv2d_9_t_out_0_fmt_scale_const_f32, conv2d_9_t_weight_0_fmt_scale_const_f32, conv2d_9_l_out_ch_format_const_layer_format_type, conv2d_9_t_out_0_ptr_s8, 1, 768, conv2d_9_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_9_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_9 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_10 */
  {
      const ai_i8* conv2d_10_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 768);
    const ai_i8* conv2d_10_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[18] + 0);
    const ai_i32* conv2d_10_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[19] + 0);
    ai_i8* conv2d_10_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 4164);
    ai_i16* conv2d_10_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(10, 1, {(stai_ptr) conv2d_10_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_10_t_in_0_ptr_const_s8, conv2d_10_t_in_0_shape_w_const_u16, conv2d_10_t_in_0_shape_h_const_u16, conv2d_10_t_in_0_shape_ch_const_u16, conv2d_10_t_weight_0_ptr_const_s8, conv2d_10_t_weight_0_shape_w_const_u16, conv2d_10_t_weight_0_shape_h_const_u16, conv2d_10_l_pad_W_0_const_s32, conv2d_10_l_pad_H_0_const_s32, conv2d_10_l_stride_1_const_u16, conv2d_10_l_stride_0_const_u16, conv2d_10_t_weight_1_ptr_const_s32, conv2d_10_t_in_0_fmt_zero_const_s8, conv2d_10_t_out_0_fmt_zero_const_s8, conv2d_10_t_in_0_fmt_scale_const_f32, conv2d_10_t_out_0_fmt_scale_const_f32, conv2d_10_t_weight_0_fmt_scale_const_f32, conv2d_10_t_out_0_ptr_s8, conv2d_10_t_out_0_shape_w_const_u16, conv2d_10_t_out_0_shape_h_const_u16, 0, 2369, conv2d_10_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, {(stai_ptr) conv2d_10_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_10 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 4164);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[20] + 0);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[21] + 0);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 896);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_l_stride_1_const_u16, conv2d_11_l_stride_0_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_t_out_0_shape_ch_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_l_out_ch_format_const_layer_format_type, conv2d_11_t_out_0_ptr_s8, 1, 896, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 896);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[22] + 0);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[23] + 0);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1920);
  
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
    ai_i8* conv2d_13_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1792);
    ai_i16* conv2d_13_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 256);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) conv2d_13_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_13_t_in_0_ptr_const_s8, conv2d_13_t_in_0_shape_w_const_u16, conv2d_13_t_in_0_shape_h_const_u16, conv2d_13_l_stride_1_const_u16, conv2d_13_l_stride_0_const_u16, conv2d_13_t_in_0_shape_ch_const_u16, conv2d_13_t_weight_0_ptr_const_s8, conv2d_13_t_out_0_shape_ch_const_u16, conv2d_13_t_weight_1_ptr_const_s32, conv2d_13_t_in_0_fmt_zero_const_s8, conv2d_13_t_out_0_fmt_zero_const_s8, conv2d_13_t_in_0_fmt_scale_const_f32, conv2d_13_t_out_0_fmt_scale_const_f32, conv2d_13_t_weight_0_fmt_scale_const_f32, conv2d_13_l_out_ch_format_const_layer_format_type, conv2d_13_t_out_0_ptr_s8, 1, 1536, conv2d_13_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) conv2d_13_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_13 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_14 */
  {
      const ai_i8* conv2d_14_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1792);
    const ai_i8* conv2d_14_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[26] + 0);
    const ai_i32* conv2d_14_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[27] + 0);
    ai_i8* conv2d_14_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_14_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2304);
  
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
    ai_i8* conv2d_15_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    ai_i16* conv2d_15_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 512);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) conv2d_15_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_15_t_in_0_ptr_const_s8, conv2d_15_t_in_0_shape_w_const_u16, conv2d_15_t_in_0_shape_h_const_u16, conv2d_15_l_stride_1_const_u16, conv2d_15_l_stride_0_const_u16, conv2d_15_t_in_0_shape_ch_const_u16, conv2d_15_t_weight_0_ptr_const_s8, conv2d_15_t_out_0_shape_ch_const_u16, conv2d_15_t_weight_1_ptr_const_s32, conv2d_15_t_in_0_fmt_zero_const_s8, conv2d_15_t_out_0_fmt_zero_const_s8, conv2d_15_t_in_0_fmt_scale_const_f32, conv2d_15_t_out_0_fmt_scale_const_f32, conv2d_15_t_weight_0_fmt_scale_const_f32, conv2d_15_l_out_ch_format_const_layer_format_type, conv2d_15_t_out_0_ptr_s8, 1, 1792, conv2d_15_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) conv2d_15_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_15 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_16 */
  {
      const ai_i8* conv2d_16_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    const ai_i8* conv2d_16_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[30] + 0);
    const ai_i32* conv2d_16_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[31] + 0);
    ai_i8* conv2d_16_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_16_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2816);
  
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
    ai_i8* conv2d_17_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    ai_i16* conv2d_17_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 512);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 1, {(stai_ptr) conv2d_17_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_17_t_in_0_ptr_const_s8, conv2d_17_t_in_0_shape_w_const_u16, conv2d_17_t_in_0_shape_h_const_u16, conv2d_17_l_stride_1_const_u16, conv2d_17_l_stride_0_const_u16, conv2d_17_t_in_0_shape_ch_const_u16, conv2d_17_t_weight_0_ptr_const_s8, conv2d_17_t_out_0_shape_ch_const_u16, conv2d_17_t_weight_1_ptr_const_s32, conv2d_17_t_in_0_fmt_zero_const_s8, conv2d_17_t_out_0_fmt_zero_const_s8, conv2d_17_t_in_0_fmt_scale_const_f32, conv2d_17_t_out_0_fmt_scale_const_f32, conv2d_17_t_weight_0_fmt_scale_const_f32, conv2d_17_l_out_ch_format_const_layer_format_type, conv2d_17_t_out_0_ptr_s8, 1, 1792, conv2d_17_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, {(stai_ptr) conv2d_17_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_17 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_18 */
  {
      const ai_i8* conv2d_18_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    const ai_i8* conv2d_18_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[34] + 0);
    const ai_i32* conv2d_18_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[35] + 0);
    ai_i8* conv2d_18_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_18_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2816);
  
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
    ai_i8* conv2d_19_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    ai_i16* conv2d_19_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 512);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_19_t_in_0_ptr_const_s8, conv2d_19_t_in_0_shape_w_const_u16, conv2d_19_t_in_0_shape_h_const_u16, conv2d_19_l_stride_1_const_u16, conv2d_19_l_stride_0_const_u16, conv2d_19_t_in_0_shape_ch_const_u16, conv2d_19_t_weight_0_ptr_const_s8, conv2d_19_t_out_0_shape_ch_const_u16, conv2d_19_t_weight_1_ptr_const_s32, conv2d_19_t_in_0_fmt_zero_const_s8, conv2d_19_t_out_0_fmt_zero_const_s8, conv2d_19_t_in_0_fmt_scale_const_f32, conv2d_19_t_out_0_fmt_scale_const_f32, conv2d_19_t_weight_0_fmt_scale_const_f32, conv2d_19_l_out_ch_format_const_layer_format_type, conv2d_19_t_out_0_ptr_s8, 1, 1792, conv2d_19_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_19 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_20 */
  {
      const ai_i8* conv2d_20_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    const ai_i8* conv2d_20_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[38] + 0);
    const ai_i32* conv2d_20_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[39] + 0);
    ai_i8* conv2d_20_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_20_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2816);
  
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
    ai_i8* conv2d_21_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    ai_i16* conv2d_21_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 512);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, {(stai_ptr) conv2d_21_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_21_t_in_0_ptr_const_s8, conv2d_21_t_in_0_shape_w_const_u16, conv2d_21_t_in_0_shape_h_const_u16, conv2d_21_l_stride_1_const_u16, conv2d_21_l_stride_0_const_u16, conv2d_21_t_in_0_shape_ch_const_u16, conv2d_21_t_weight_0_ptr_const_s8, conv2d_21_t_out_0_shape_ch_const_u16, conv2d_21_t_weight_1_ptr_const_s32, conv2d_21_t_in_0_fmt_zero_const_s8, conv2d_21_t_out_0_fmt_zero_const_s8, conv2d_21_t_in_0_fmt_scale_const_f32, conv2d_21_t_out_0_fmt_scale_const_f32, conv2d_21_t_weight_0_fmt_scale_const_f32, conv2d_21_l_out_ch_format_const_layer_format_type, conv2d_21_t_out_0_ptr_s8, 1, 1792, conv2d_21_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, {(stai_ptr) conv2d_21_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_21 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_22 */
  {
      const ai_i8* conv2d_22_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    const ai_i8* conv2d_22_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[42] + 0);
    const ai_i32* conv2d_22_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[43] + 0);
    ai_i8* conv2d_22_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_22_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2816);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(22, 1, {(stai_ptr) conv2d_22_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_22_t_in_0_ptr_const_s8, conv2d_22_t_in_0_shape_w_const_u16, conv2d_22_t_in_0_shape_h_const_u16, conv2d_22_t_in_0_shape_ch_const_u16, conv2d_22_t_weight_0_ptr_const_s8, conv2d_22_t_weight_0_shape_w_const_u16, conv2d_22_t_weight_0_shape_h_const_u16, conv2d_22_l_pad_W_0_const_s32, conv2d_22_l_pad_H_0_const_s32, conv2d_22_l_stride_1_const_u16, conv2d_22_l_stride_0_const_u16, conv2d_22_t_weight_1_ptr_const_s32, conv2d_22_t_in_0_fmt_zero_const_s8, conv2d_22_t_out_0_fmt_zero_const_s8, conv2d_22_t_in_0_fmt_scale_const_f32, conv2d_22_t_out_0_fmt_scale_const_f32, conv2d_22_t_weight_0_fmt_scale_const_f32, conv2d_22_t_out_0_ptr_s8, conv2d_22_t_out_0_shape_w_const_u16, conv2d_22_t_out_0_shape_h_const_u16, 0, 4737, conv2d_22_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(22, 1, {(stai_ptr) conv2d_22_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_22 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_23 */
  {
      const ai_i8* conv2d_23_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_23_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[44] + 0);
    const ai_i32* conv2d_23_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[45] + 0);
    ai_i8* conv2d_23_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    ai_i16* conv2d_23_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 512);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(23, 1, {(stai_ptr) conv2d_23_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_23_t_in_0_ptr_const_s8, conv2d_23_t_in_0_shape_w_const_u16, conv2d_23_t_in_0_shape_h_const_u16, conv2d_23_l_stride_1_const_u16, conv2d_23_l_stride_0_const_u16, conv2d_23_t_in_0_shape_ch_const_u16, conv2d_23_t_weight_0_ptr_const_s8, conv2d_23_t_out_0_shape_ch_const_u16, conv2d_23_t_weight_1_ptr_const_s32, conv2d_23_t_in_0_fmt_zero_const_s8, conv2d_23_t_out_0_fmt_zero_const_s8, conv2d_23_t_in_0_fmt_scale_const_f32, conv2d_23_t_out_0_fmt_scale_const_f32, conv2d_23_t_weight_0_fmt_scale_const_f32, conv2d_23_l_out_ch_format_const_layer_format_type, conv2d_23_t_out_0_ptr_s8, 1, 1792, conv2d_23_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(23, 1, {(stai_ptr) conv2d_23_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_23 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_24 */
  {
      const ai_i8* conv2d_24_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2304);
    const ai_i8* conv2d_24_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[46] + 0);
    const ai_i32* conv2d_24_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[47] + 0);
    ai_i8* conv2d_24_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_24_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2816);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(24, 1, {(stai_ptr) conv2d_24_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_24_t_in_0_ptr_const_s8, conv2d_24_t_in_0_shape_w_const_u16, conv2d_24_t_in_0_shape_h_const_u16, conv2d_24_t_in_0_shape_ch_const_u16, conv2d_24_t_weight_0_ptr_const_s8, conv2d_24_t_weight_0_shape_w_const_u16, conv2d_24_t_weight_0_shape_h_const_u16, conv2d_24_l_pad_W_0_const_s32, conv2d_24_l_pad_H_0_const_s32, conv2d_24_l_stride_1_const_u16, conv2d_24_l_stride_0_const_u16, conv2d_24_t_weight_1_ptr_const_s32, conv2d_24_t_in_0_fmt_zero_const_s8, conv2d_24_t_out_0_fmt_zero_const_s8, conv2d_24_t_in_0_fmt_scale_const_f32, conv2d_24_t_out_0_fmt_scale_const_f32, conv2d_24_t_weight_0_fmt_scale_const_f32, conv2d_24_t_out_0_ptr_s8, conv2d_24_t_out_0_shape_w_const_u16, conv2d_24_t_out_0_shape_h_const_u16, 0, 4737, conv2d_24_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(24, 1, {(stai_ptr) conv2d_24_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_24 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_25 */
  {
      const ai_i8* conv2d_25_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_25_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[48] + 0);
    const ai_i32* conv2d_25_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[49] + 0);
    ai_i8* conv2d_25_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 12032);
    ai_i16* conv2d_25_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 128);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(25, 1, {(stai_ptr) conv2d_25_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_25_t_in_0_ptr_const_s8, conv2d_25_t_in_0_shape_w_const_u16, conv2d_25_t_in_0_shape_h_const_u16, conv2d_25_l_stride_1_const_u16, conv2d_25_l_stride_0_const_u16, conv2d_25_t_in_0_shape_ch_const_u16, conv2d_25_t_weight_0_ptr_const_s8, conv2d_25_t_out_0_shape_ch_const_u16, conv2d_25_t_weight_1_ptr_const_s32, conv2d_25_t_in_0_fmt_zero_const_s8, conv2d_25_t_out_0_fmt_zero_const_s8, conv2d_25_t_in_0_fmt_scale_const_f32, conv2d_25_t_out_0_fmt_scale_const_f32, conv2d_25_t_weight_0_fmt_scale_const_f32, conv2d_25_l_out_ch_format_const_layer_format_type, conv2d_25_t_out_0_ptr_s8, 1, 3072, conv2d_25_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(25, 1, {(stai_ptr) conv2d_25_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_25 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_26 */
  {
      const ai_i8* conv2d_26_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 12032);
    const ai_i8* conv2d_26_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[50] + 0);
    const ai_i32* conv2d_26_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[51] + 0);
    ai_i8* conv2d_26_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 9476);
    ai_i16* conv2d_26_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(26, 1, {(stai_ptr) conv2d_26_t_in_0_ptr_const_s8});
    
  forward_lite_dw_sssa8_ch(conv2d_26_t_in_0_ptr_const_s8, conv2d_26_t_in_0_shape_w_const_u16, conv2d_26_t_in_0_shape_h_const_u16, conv2d_26_t_in_0_shape_ch_const_u16, conv2d_26_t_weight_0_ptr_const_s8, conv2d_26_t_weight_0_shape_w_const_u16, conv2d_26_t_weight_0_shape_h_const_u16, conv2d_26_l_pad_W_0_const_s32, conv2d_26_l_pad_H_0_const_s32, conv2d_26_l_stride_1_const_u16, conv2d_26_l_stride_0_const_u16, conv2d_26_t_weight_1_ptr_const_s32, conv2d_26_t_in_0_fmt_zero_const_s8, conv2d_26_t_out_0_fmt_zero_const_s8, conv2d_26_t_in_0_fmt_scale_const_f32, conv2d_26_t_out_0_fmt_scale_const_f32, conv2d_26_t_weight_0_fmt_scale_const_f32, conv2d_26_t_out_0_ptr_s8, conv2d_26_t_out_0_shape_w_const_u16, conv2d_26_t_out_0_shape_h_const_u16, 0, 9473, conv2d_26_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(26, 1, {(stai_ptr) conv2d_26_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_26 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_27 */
  {
    
  forward_lite_conv2d_27(net_ctx);
  }
  /* LITE_KERNEL_SECTION END conv2d_27 */
  /* LITE_KERNEL_SECTION BEGIN gemm_29 */
  {
    
  forward_lite_gemm_29(net_ctx);
  }
  /* LITE_KERNEL_SECTION END gemm_29 */
  /* LITE_KERNEL_SECTION BEGIN nl_30 */
  {
      ai_i8* nl_30_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 496);
    const ai_i8* nl_30_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 612);
    ai_i32* nl_30_t_scratch_0_ptr_s32 = (ai_i32*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(30, 1, {(stai_ptr) nl_30_t_in_0_ptr_const_s8});
    
  forward_lite_nl_softmax_is8os8(nl_30_t_out_0_ptr_s8, nl_30_t_in_0_ptr_const_s8, nl_30_t_in_0_shape_ch_prod_const_u32, 1, 10, 1089941248, 24, -124, nl_30_t_scratch_0_ptr_s32);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(30, 1, {(stai_ptr) nl_30_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END nl_30 */
  /* LITE_KERNEL_SECTION BEGIN conversion_31 */
  {
      const ai_i8* conversion_31_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 496);
    ai_float* conversion_31_t_out_0_ptr_f32 = (ai_float*)(net_ctx->_outputs[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(31, 1, {(stai_ptr) conversion_31_t_in_0_ptr_const_s8});
    
  forward_lite_node_convert_integer_is8of32(conversion_31_t_in_0_ptr_const_s8, conversion_31_t_out_0_ptr_f32, conversion_31_t_out_0_shape_h_w_ch_d_prod_const_u32, conversion_31_t_in_0_fmt_scale_const_f32, conversion_31_t_in_0_fmt_zero_const_s8);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(31, 1, {(stai_ptr) conversion_31_t_out_0_ptr_f32});
  }
  /* LITE_KERNEL_SECTION END conversion_31 */
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

