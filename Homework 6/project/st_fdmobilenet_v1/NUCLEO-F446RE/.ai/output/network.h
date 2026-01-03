/**
  ******************************************************************************
  * @file    network.h
  * @author  AST Embedded Analytics Research Platform
  * @date    2025-12-31T12:38:51+0000
  * @brief   ST.AI Tool Automatic Code Generator for Embedded NN computing
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
#ifndef STAI_NETWORK_H
#define STAI_NETWORK_H

#include "stai.h"
#include "stai_debug.h"

/*****************************************************************************/
/*  Original model name and signature  */
#define STAI_NETWORK_ORIGIN_MODEL_NAME         "st_fdmobilenet_v1_quant"
#define STAI_NETWORK_ORIGIN_MODEL_SIGNATURE    "0x6ae6379491dd57665b5d1728f5145be8"

/*  Generated model name and signature  */
#define STAI_NETWORK_MODEL_NAME                "network"
#define STAI_NETWORK_MODEL_SIGNATURE           (0x6f8354e26ad63878)


/*****************************************************************************/
/*  Macros to declare network context size and required alignment  */
#define STAI_NETWORK_CONTEXT_ALIGNMENT   8
#define STAI_NETWORK_CONTEXT_SIZE        (sizeof(_stai_network_context))

/*  Macro to declare network flags  */
#define STAI_NETWORK_FLAGS               (STAI_FLAG_INPUTS|STAI_FLAG_OUTPUTS|STAI_FLAG_WEIGHTS)

/*  Macro to declare number of network C nodes generated  */
#define STAI_NETWORK_NODES_NUM           (27)

/*  Macro to declare number macc for generated network model  */
#define STAI_NETWORK_MACC_NUM            (509908)

/*  Macros to declare input buffers characteristics  */
#define STAI_NETWORK_IN_NUM              (1)
#define STAI_NETWORK_IN_SIZE_BYTES       (12292)

#define STAI_NETWORK_IN_ALIGNMENTS \
  { 4}
#define STAI_NETWORK_IN_NAMES \
  { NULL}
#define STAI_NETWORK_IN_FORMATS \
  { STAI_FORMAT_FLOAT32}
#define STAI_NETWORK_IN_SIZES \
  {3072}
#define STAI_NETWORK_IN_SIZES_BYTES \
  {12288}

#define STAI_NETWORK_IN_1_ALIGNMENT   4
#define STAI_NETWORK_IN_1_NAME        (NULL)
#define STAI_NETWORK_IN_1_FLAGS       (STAI_FLAG_INPUTS|STAI_FLAG_PREALLOCATED|STAI_FLAG_CHANNEL_LAST|STAI_FLAG_HAS_BATCH)
#define STAI_NETWORK_IN_1_FORMAT      (STAI_FORMAT_FLOAT32)
#define STAI_NETWORK_IN_1_SHAPE       {1,32,32,3}
#define STAI_NETWORK_IN_1_BATCH       (1)
#define STAI_NETWORK_IN_1_HEIGHT      (32)
#define STAI_NETWORK_IN_1_WIDTH       (32)
#define STAI_NETWORK_IN_1_CHANNEL     (3)
#define STAI_NETWORK_IN_1_RANK        (3)
#define STAI_NETWORK_IN_1_SIZE        (3072)
#define STAI_NETWORK_IN_1_SIZE_BYTES  (12288)

/*****************************************************************************/
/*  Macros to declare output buffers characteristics  */
#define STAI_NETWORK_OUT_NUM             (1)
#define STAI_NETWORK_OUT_SIZE_BYTES      (44)
#define STAI_NETWORK_OUT_ALIGNMENTS \
  { 4}
#define STAI_NETWORK_OUT_NAMES \
  { NULL}
#define STAI_NETWORK_OUT_FORMATS \
  { STAI_FORMAT_FLOAT32}
#define STAI_NETWORK_OUT_SIZES \
  { 10}
#define STAI_NETWORK_OUT_SIZES_BYTES \
  {40}

#define STAI_NETWORK_OUT_1_ALIGNMENT   4
#define STAI_NETWORK_OUT_1_NAME        (NULL)
#define STAI_NETWORK_OUT_1_FLAGS       (STAI_FLAG_OUTPUTS|STAI_FLAG_PREALLOCATED|STAI_FLAG_CHANNEL_LAST|STAI_FLAG_HAS_BATCH)
#define STAI_NETWORK_OUT_1_FORMAT      (STAI_FORMAT_FLOAT32)
#define STAI_NETWORK_OUT_1_SHAPE       {1,10}
#define STAI_NETWORK_OUT_1_BATCH       (1)
#define STAI_NETWORK_OUT_1_CHANNEL     (10)
#define STAI_NETWORK_OUT_1_RANK        (1)
#define STAI_NETWORK_OUT_1_SIZE        ((10))
#define STAI_NETWORK_OUT_1_SIZE_BYTES  (40)

/*****************************************************************************/
/*  Macros to declare activations buffers characteristics  */
#define STAI_NETWORK_ACTIVATIONS_SIZE              (12288)
#define STAI_NETWORK_ACTIVATIONS_SIZE_BYTES        (12288)
#define STAI_NETWORK_ACTIVATIONS_ALIGNMENTS \
  { 4}
#define STAI_NETWORK_ACTIVATIONS_SIZES \
  { 12288}
#define STAI_NETWORK_ACTIVATIONS_NUM               (1)

#define STAI_NETWORK_ACTIVATION_1_FLAGS      (STAI_FLAG_ACTIVATIONS)
#define STAI_NETWORK_ACTIVATION_1_SIZE       (12288)
#define STAI_NETWORK_ACTIVATION_1_SIZE_BYTES (12288)
#define STAI_NETWORK_ACTIVATION_1_ALIGNMENT  4
/*  Macros to declare weights buffers characteristics  */

#define STAI_NETWORK_WEIGHTS_NUM                  (48)
#define STAI_NETWORK_WEIGHTS_SIZE                  (149704)
#define STAI_NETWORK_WEIGHTS_SIZE_BYTES            (149704)
#define STAI_NETWORK_WEIGHTS_ALIGNMENTS \
  { 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4}
#define STAI_NETWORK_WEIGHTS_SIZES \
  { 432,64,144,64,768,192,432,192,3840,320,720,320,5120,256,576,256,6144,384,864,384,6144,256,576,256,8192,512,1152,512,16384,512,1152,512,16384,512,1152,512,16384,512,1152,512,16384,512,1152,512,32768,1024,2560,40}
#define STAI_NETWORK_WEIGHTS_NUM        (48)
#define STAI_NETWORK_WEIGHT_1_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_1_SIZE        (432)
#define STAI_NETWORK_WEIGHT_1_SIZE_BYTES  (432)
#define STAI_NETWORK_WEIGHT_2_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_2_SIZE        (64)
#define STAI_NETWORK_WEIGHT_2_SIZE_BYTES  (64)
#define STAI_NETWORK_WEIGHT_3_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_3_SIZE        (144)
#define STAI_NETWORK_WEIGHT_3_SIZE_BYTES  (144)
#define STAI_NETWORK_WEIGHT_4_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_4_SIZE        (64)
#define STAI_NETWORK_WEIGHT_4_SIZE_BYTES  (64)
#define STAI_NETWORK_WEIGHT_5_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_5_SIZE        (768)
#define STAI_NETWORK_WEIGHT_5_SIZE_BYTES  (768)
#define STAI_NETWORK_WEIGHT_6_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_6_SIZE        (192)
#define STAI_NETWORK_WEIGHT_6_SIZE_BYTES  (192)
#define STAI_NETWORK_WEIGHT_7_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_7_SIZE        (432)
#define STAI_NETWORK_WEIGHT_7_SIZE_BYTES  (432)
#define STAI_NETWORK_WEIGHT_8_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_8_SIZE        (192)
#define STAI_NETWORK_WEIGHT_8_SIZE_BYTES  (192)
#define STAI_NETWORK_WEIGHT_9_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_9_SIZE        (3840)
#define STAI_NETWORK_WEIGHT_9_SIZE_BYTES  (3840)
#define STAI_NETWORK_WEIGHT_10_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_10_SIZE        (320)
#define STAI_NETWORK_WEIGHT_10_SIZE_BYTES  (320)
#define STAI_NETWORK_WEIGHT_11_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_11_SIZE        (720)
#define STAI_NETWORK_WEIGHT_11_SIZE_BYTES  (720)
#define STAI_NETWORK_WEIGHT_12_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_12_SIZE        (320)
#define STAI_NETWORK_WEIGHT_12_SIZE_BYTES  (320)
#define STAI_NETWORK_WEIGHT_13_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_13_SIZE        (5120)
#define STAI_NETWORK_WEIGHT_13_SIZE_BYTES  (5120)
#define STAI_NETWORK_WEIGHT_14_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_14_SIZE        (256)
#define STAI_NETWORK_WEIGHT_14_SIZE_BYTES  (256)
#define STAI_NETWORK_WEIGHT_15_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_15_SIZE        (576)
#define STAI_NETWORK_WEIGHT_15_SIZE_BYTES  (576)
#define STAI_NETWORK_WEIGHT_16_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_16_SIZE        (256)
#define STAI_NETWORK_WEIGHT_16_SIZE_BYTES  (256)
#define STAI_NETWORK_WEIGHT_17_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_17_SIZE        (6144)
#define STAI_NETWORK_WEIGHT_17_SIZE_BYTES  (6144)
#define STAI_NETWORK_WEIGHT_18_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_18_SIZE        (384)
#define STAI_NETWORK_WEIGHT_18_SIZE_BYTES  (384)
#define STAI_NETWORK_WEIGHT_19_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_19_SIZE        (864)
#define STAI_NETWORK_WEIGHT_19_SIZE_BYTES  (864)
#define STAI_NETWORK_WEIGHT_20_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_20_SIZE        (384)
#define STAI_NETWORK_WEIGHT_20_SIZE_BYTES  (384)
#define STAI_NETWORK_WEIGHT_21_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_21_SIZE        (6144)
#define STAI_NETWORK_WEIGHT_21_SIZE_BYTES  (6144)
#define STAI_NETWORK_WEIGHT_22_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_22_SIZE        (256)
#define STAI_NETWORK_WEIGHT_22_SIZE_BYTES  (256)
#define STAI_NETWORK_WEIGHT_23_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_23_SIZE        (576)
#define STAI_NETWORK_WEIGHT_23_SIZE_BYTES  (576)
#define STAI_NETWORK_WEIGHT_24_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_24_SIZE        (256)
#define STAI_NETWORK_WEIGHT_24_SIZE_BYTES  (256)
#define STAI_NETWORK_WEIGHT_25_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_25_SIZE        (8192)
#define STAI_NETWORK_WEIGHT_25_SIZE_BYTES  (8192)
#define STAI_NETWORK_WEIGHT_26_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_26_SIZE        (512)
#define STAI_NETWORK_WEIGHT_26_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_27_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_27_SIZE        (1152)
#define STAI_NETWORK_WEIGHT_27_SIZE_BYTES  (1152)
#define STAI_NETWORK_WEIGHT_28_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_28_SIZE        (512)
#define STAI_NETWORK_WEIGHT_28_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_29_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_29_SIZE        (16384)
#define STAI_NETWORK_WEIGHT_29_SIZE_BYTES  (16384)
#define STAI_NETWORK_WEIGHT_30_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_30_SIZE        (512)
#define STAI_NETWORK_WEIGHT_30_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_31_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_31_SIZE        (1152)
#define STAI_NETWORK_WEIGHT_31_SIZE_BYTES  (1152)
#define STAI_NETWORK_WEIGHT_32_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_32_SIZE        (512)
#define STAI_NETWORK_WEIGHT_32_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_33_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_33_SIZE        (16384)
#define STAI_NETWORK_WEIGHT_33_SIZE_BYTES  (16384)
#define STAI_NETWORK_WEIGHT_34_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_34_SIZE        (512)
#define STAI_NETWORK_WEIGHT_34_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_35_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_35_SIZE        (1152)
#define STAI_NETWORK_WEIGHT_35_SIZE_BYTES  (1152)
#define STAI_NETWORK_WEIGHT_36_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_36_SIZE        (512)
#define STAI_NETWORK_WEIGHT_36_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_37_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_37_SIZE        (16384)
#define STAI_NETWORK_WEIGHT_37_SIZE_BYTES  (16384)
#define STAI_NETWORK_WEIGHT_38_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_38_SIZE        (512)
#define STAI_NETWORK_WEIGHT_38_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_39_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_39_SIZE        (1152)
#define STAI_NETWORK_WEIGHT_39_SIZE_BYTES  (1152)
#define STAI_NETWORK_WEIGHT_40_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_40_SIZE        (512)
#define STAI_NETWORK_WEIGHT_40_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_41_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_41_SIZE        (16384)
#define STAI_NETWORK_WEIGHT_41_SIZE_BYTES  (16384)
#define STAI_NETWORK_WEIGHT_42_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_42_SIZE        (512)
#define STAI_NETWORK_WEIGHT_42_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_43_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_43_SIZE        (1152)
#define STAI_NETWORK_WEIGHT_43_SIZE_BYTES  (1152)
#define STAI_NETWORK_WEIGHT_44_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_44_SIZE        (512)
#define STAI_NETWORK_WEIGHT_44_SIZE_BYTES  (512)
#define STAI_NETWORK_WEIGHT_45_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_45_SIZE        (32768)
#define STAI_NETWORK_WEIGHT_45_SIZE_BYTES  (32768)
#define STAI_NETWORK_WEIGHT_46_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_46_SIZE        (1024)
#define STAI_NETWORK_WEIGHT_46_SIZE_BYTES  (1024)
#define STAI_NETWORK_WEIGHT_47_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_47_SIZE        (2560)
#define STAI_NETWORK_WEIGHT_47_SIZE_BYTES  (2560)
#define STAI_NETWORK_WEIGHT_48_FLAGS       ((STAI_FLAG_PREALLOCATED))
#define STAI_NETWORK_WEIGHT_48_SIZE        (40)
#define STAI_NETWORK_WEIGHT_48_SIZE_BYTES  (40)


/*****************************************************************************/
/*  Macros to declare states buffers characteristics  */
#define STAI_NETWORK_STATES_SIZE              (0)
#define STAI_NETWORK_STATES_SIZE_BYTES        (0)
#define STAI_NETWORK_STATES_ALIGNMENTS        {}
#define STAI_NETWORK_STATES_SIZES             { 0, }
#define STAI_NETWORK_STATES_NUM               (0)

#define STAI_NETWORK_STATE_1_FLAGS            (0x0)
#define STAI_NETWORK_STATE_1_ALIGNMENT        4
#define STAI_NETWORK_STATE_1_SIZE             (0)
#define STAI_NETWORK_STATE_1_SIZE_BYTES       (0)
/*****************************************************************************/
STAI_API_DECLARE_BEGIN

/*****************************************************************************/
/** Network private context: DO NOT EDIT NOR access this struct directly    **/
/*****************************************************************************/
/**
 * @brief Private internal context C struct definition
 * @details this is the C struct defining the internal context of the generated network.
 *          this struct must be allocated by applications as a byte array of a given size,
 *          but MUST not be accessed directly by application. It must be manipulated / queried only
 *          using st.ai APIs.
 */
typedef struct {
  uint32_t                     _magic;
  uint64_t                     _signature;
  stai_flags                   _flags;
  stai_return_code             _return_code;
  stai_event_cb                _callback;
  void*                        _callback_cookie;
  stai_ptr                     _activations[STAI_NETWORK_ACTIVATIONS_NUM];
  stai_ptr                     _weights[STAI_NETWORK_WEIGHTS_NUM];
  stai_ptr                     _inputs[STAI_NETWORK_IN_NUM];
  stai_ptr                     _outputs[STAI_NETWORK_OUT_NUM];
} _stai_network_context;


/*****************************************************************************/
/*  Network run management APIs Section  */

/**
 * @brief Initialize the network internal context
 * @details The application must provide the memory area where to initialize the internal network context as an opaque
 *          handler named network.
 *          Application must never access directly the directly the context but it must be accessed only with
 *          associates stai APIs. Application knows network context size by using macro
 *          @ref STAI_NETWORK_CONTEXT_SIZE or @ref stai_network_get_context_size(void) API
 * @param[in] network network context opaque pointer
 * @return STAI_SUCCESS on correct initialization, else 1st error return code from @ref stai_return_code enums.
 */
STAI_API_ENTRY
stai_return_code stai_network_init(
  stai_network* network);


/**
 * @brief Deinitialize the network internal context
 * @details The application must provide the memory area where to initialize the internal network context as an opaque
 *          handler named netwrok.
 *          Application must never access directly the directly the context but it must be accessed only with
 *          associates stai APIs.
 * @param[in] network network context opaque pointer
 * @return STAI_SUCCESS on correct de-initialization, else 1st error return code from @ref stai_return_code enums.
 */
STAI_API_ENTRY
stai_return_code stai_network_deinit(
  stai_network* network);


/**
 * @brief Runs the network model inference on a single input sample
 * @details This API runs a newtwork inference. Network input data is expected to be available in expected format
 *          on the input buffers whose memory adresses are retrived using @ref stai_network_get_inputs() API.
 *          Alternatively the application could allocate the inputs and set the addresses using the
 *          @ref stai_network_set_inputs() API. Similar behaviour is expected for the network outputs handling.
 *          The features of each input / output buffer (format, byte size, etc.)may be queried using
 *          @ref stai_network_get_info() API or the set of specific macros available in this header file.
 *          When mode is MODE_SYNC the call to this API is blocking untile the end of the inference. In ASYNC_MODE the
 *          API will exit before the completion of the inference by returning the current state of the inference. It
 *          could be also used to query wether or not the inference has been completed.
 *          After completions of run API the results of the network inference may be retrieved at addresses returned by
 *          @ref stai_network_get_outputs() API.
 * @param[in] network network context opaque pointer
 * @param[in] mode the modality used to run the network. It may be synchronous (MODE_SYNC) or asyncronous (MODE_ASYNC).
 *            see @ref stai_run_mode enum
 * @return STAI_SUCCESS on correct run, else error 1st return code from @ref stai_return_code enums.
 */
STAI_API_ENTRY
stai_return_code stai_network_run(
  stai_network* network,
  const stai_run_mode mode);


/*****************************************************************************/
/*  Getters APIs Section  */

/**
 * @brief Returns the newtwork context size (in bytes)
 * @details This API is used to return the size in byte of the network internal context. The st.ai APIs could support
 *          multiple network contexts, thus tt is expected that the network context is always allocated by application
 *          but MUST be handled / queried / managed only by using st.ai APIs
 * @return the network context size in bytes
 */
STAI_API_ENTRY
stai_size stai_network_get_context_size(void);


/**
 * @brief Get the relevant info for the C generated network model.
 * @details See @ref stai_network_info C struct.
 * @param[in] network network context opaque pointer
 * @param[out] info a pointer to a @ref stai_network_info data struct
 * @return STAI_SUCCESS on correct run, else error 1st return code from @ref stai_return_code enums.
 */
STAI_API_ENTRY
stai_return_code stai_network_get_info(
  stai_network* network,
  stai_network_info* info);


/**
 * @brief Get the activations buffers addresses.
 * @details as an array of pointers of size n_activations
 * @param[in] network network context opaque pointer
 * @param[out] activations an array of pointers reporting the addresses of all the activation buffers
 * @param[out] n_activations the number of the network activations (i.e. the size of the activations array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note activations array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_get_activations(
  stai_network* network,
  stai_ptr* activations,
  stai_size* n_activations);


/**
 * @brief Get the weights buffers addresses.
 * @details as an array of pointers of size n_weights
 * @param[in] network network context opaque pointer
 * @param[out] weights an array of pointers reporting the addresses of all the weights buffers
 * @param[out] n_weights the number of the network weights (i.e. the size of the weights array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note weights array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_get_weights(
  stai_network* network,
  stai_ptr* weights,
  stai_size* n_weights);


/**
 * @brief Get the network inputs buffers addresses.
 * @details as an array of pointers of size n_inputs
 * @param[in] network network context opaque pointer
 * @param[out] inputs an array of pointers reporting the addresses of all the network inputs buffers
 * @param[out] n_inputs the number of the network inputs (i.e. the size of the inputs array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note inputs array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_get_inputs(
  stai_network* network,
  stai_ptr* inputs,
  stai_size* n_inputs);


/**
 * @brief Get the network outputs buffers addresses.
 * @details as an array of pointers of size n_outputs
 * @param[in] network network context opaque pointer
 * @param[out] outputs an array of pointers reporting the addresses of all the network outputs buffers
 * @param[out] n_outputs the number of the network outputs (i.e. the size of the outputs array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note outputs array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_get_outputs(
  stai_network* network,
  stai_ptr* outputs,
  stai_size* n_outputs);


/**
 * @brief Get the network internal states buffers addresses.
 * @details as an array of pointers of size n_states. Optional API for stateful newtorks. It gets the
 *          addresses of persistent internal states of the network. This APIs allows an application to manage
 *          stateful buffers (e.g. to save them). State buffers may be set back using
 *          @ref stai_network_set_states() dual API
 * @param[in] network network context opaque pointer
 * @param[out] states an array of pointers reporting the addresses of all the network internal states buffers
 * @param[out] n_states the number of the network internal states (i.e. the size of the states array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note outputs array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_get_states(
  stai_network* network,
  stai_ptr* states,
  stai_size* n_states);


/**
 * @brief Get the newtork error state.
 * @details In particular it return STAI_SUCCESS if no errors encountered during APIs orchestration, otherwise this
 *          API return the 1st error generated.
 * @param[in] network network context opaque pointer
 * @return STAI_SUCCESS if no errors, else the 1st error generated.
 */
STAI_API_ENTRY
stai_return_code stai_network_get_error(
  stai_network* network);


/*****************************************************************************/
/*  Setters APIS Section  */

/**
 * @brief Set the activations buffers addresses.
 * @details activations buffers are provides as as an array of pointers of size n_activations
 * @param[in] network network context opaque pointer
 * @param[in] activations an array of pointers reporting the addresses of all the activation buffers
 * @param[in] n_activations the number of the network activations (i.e. the size of the activations array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note activations array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_set_activations(
  stai_network* network,
  const stai_ptr* activations,
  const stai_size n_activations);


/**
 * @brief Set the weights buffers addresses.
 * @details weights buffers are provides as as an array of pointers of size n_weights
 * @param[in] network network context opaque pointer
 * @param[in] weights an array of pointers reporting the addresses of all the weights buffers
 * @param[in] n_weights the number of the network weights (i.e. the size of the weights array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note weights array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_set_weights(
  stai_network* network,
  const stai_ptr* weights,
  const stai_size n_weights);


/**
 * @brief Set the network inputs buffers addresses.
 * @details network inputs buffers are provides as as an array of pointers of size n_inputs
 * @param[in] network network context opaque pointer
 * @param[in] inputs an array of pointers reporting the addresses of all the network inputs buffers
 * @param[in] n_inputs the number of the network inputs (i.e. the size of the inputs array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note inputs array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_set_inputs(
  stai_network* network,
  const stai_ptr* inputs,
  const stai_size n_inputs);


/**
 * @brief Set the network outputs buffers addresses.
 * @details network outputs buffers are provides as as an array of pointers of size n_inputs
 * @param[in] network network context opaque pointer
 * @param[in] outputs an array of pointers reporting the addresses of all the network outputs buffers
 * @param[in] n_outputs the number of the network outputs (i.e. the size of the outputs array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note outputs array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_set_outputs(
  stai_network* network,
  const stai_ptr* outputs,
  const stai_size n_outputs);


/**
 * @brief Set the network internal states buffers addresses.
 * @details network states buffers are provides as as an array of pointers of size n_states
 * @param[in] network network context opaque pointer
 * @param[in] states an array of pointers reporting the addresses of all the network internal states buffers
 * @param[in] n_states the number of the network states (i.e. the size of the states array)
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 * @note states array pointers are passed by copy
 */
STAI_API_ENTRY
stai_return_code stai_network_set_states(
  stai_network* network,
  const stai_ptr* states,
  const stai_size n_states);


/**
 * @brief Set a callback function to inspect network activation buffers.
 * @details this is an API to retrieve the content on intermediate activations buffers while executing the
 *          @ref stai_network_run() API. It is a helper routine that could be used by an application for debug
 *          and / or benchmarking intermadiate results of the network.
 * @param[in] network network context opaque pointer
 * @param[in] cb a function pointer to the inspection callback routine. See @ref stai_event_cb function pointer for
 *            a definition of the callback API.
 * @param[in] cb_cookie an opaque pointer to an application memory buffer and / or variable.
 * @return STAI_SUCCESS if successfull, else error 1st return code from @ref stai_return_code enums.
 */
STAI_API_ENTRY
stai_return_code stai_network_set_callback(
  stai_network* network,
  const stai_event_cb cb,
  void* cb_cookie);



STAI_API_DECLARE_END

#endif    /* STAI_NETWORK_H */

