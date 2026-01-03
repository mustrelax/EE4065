# /*---------------------------------------------------------------------------------------------
#  * Copyright 2018 The TensorFlow Authors.
#  * Copyright (c) 2022-2023 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/

import tensorflow as tf
from tensorflow import keras
from keras.applications import imagenet_utils
from keras import layers
from keras.applications.efficientnet_v2 import (EfficientNetV2B0, EfficientNetV2B1, EfficientNetV2B2, EfficientNetV2B3,
                                                EfficientNetV2S)


def get_efficientnetv2(input_shape: tuple, model_type: str = None, num_classes: int = None, dropout: float = None,
                       pretrained_weights: str = "imagenet") -> tf.keras.Model:
    """
    Returns a transfer learning model based on efficient net v2 architecture pre-trained on ImageNet or random.

    Args:
        input_shape (tuple): Shape of the input tensor.
        model_type (string): B0, B1, B2, B3, S. Default is None.
        num_classes (int): Number of output classes of the target use-case. Default is None.
        dropout (float, optional): The dropout rate for the custom classifier.
        pretrained_weights (str, optional): The pre-trained weights to use. Either "imagenet" or None.

    Returns:
        tf.keras.Model: Transfer learning model based on efficient net v2 architecture.

    Raises:

    """

    if pretrained_weights:
        training = False
        # model is set in inference mode so that moving avg and var of any BN are kept untouched
        # should help the convergence according to Keras tutorial
    else:
        training = True

    # Define the input layer
    inputs = keras.Input(shape=input_shape)

    # fetch the backbone pre-trained on imagenet or random
    if model_type == "B0":
        backbone_func = EfficientNetV2B0
    elif model_type == "B1":
        backbone_func = EfficientNetV2B1
    elif model_type == "B2":
        backbone_func = EfficientNetV2B2
    elif model_type == "B3":
        backbone_func = EfficientNetV2B3
    elif model_type == "S":
        backbone_func = EfficientNetV2S

    base_model = backbone_func(
        include_top=False,
        weights=pretrained_weights,
        input_tensor=None,
        input_shape=input_shape,
        pooling="avg",
        classes=num_classes,
        include_preprocessing=False
    )
    
    # Create a new model on top
    x = base_model(inputs, training=training)

    if dropout:
        x = layers.Dropout(rate=dropout, name="dropout")(x)
    if num_classes > 2:
        outputs = keras.layers.Dense(num_classes, activation="softmax")(x)
    else:
        outputs = keras.layers.Dense(1, activation="sigmoid")(x)

    # Create the Keras model
    model = keras.Model(inputs=inputs, outputs=outputs, name="efficientnet_v2"+model_type)
    return model
