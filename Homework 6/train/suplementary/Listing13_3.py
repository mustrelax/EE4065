import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 
import tensorflow as tf

from matplotlib import pyplot as plt
import numpy as np

# Model / data parameters
num_classes = 10
input_shape = (28, 28, 1)


# Load MNIST dataset
mnist = tf.keras.datasets.mnist
(train_images, train_labels), (test_images, test_labels) = mnist.load_data()

# Normalize the input image so that each pixel value is between 0 to 1.
train_images = train_images / 255.0
test_images = test_images / 255.0

# Define the model architecture
model = tf.keras.Sequential([
        tf.keras.Input(shape=input_shape),
        tf.keras.layers.Conv2D(32, kernel_size=(3, 3), activation="relu", name = "mnist_conv1"),
        tf.keras.layers.MaxPooling2D(pool_size=(2, 2)),
        tf.keras.layers.Conv2D(64, kernel_size=(3, 3), activation="relu", name = "mnist_conv2"),
        tf.keras.layers.MaxPooling2D(pool_size=(2, 2)),
        tf.keras.layers.Flatten(),
        tf.keras.layers.Dense(32),
        tf.keras.layers.Dense(10),
        ])

# Train the digit classification model
model.compile(optimizer='adam',
              loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
              metrics=['accuracy'])
model.fit(
  train_images,
  train_labels,
  epochs=3,
  validation_data=(test_images, test_labels)
)

model.summary()

test_ind=0
im = test_images[test_ind]

plt.figure()
plt.imshow(im, cmap = "gray")

img = np.expand_dims(im, 0)
layer = model.get_layer(name="mnist_conv1")
feature_extractor = tf.keras.Model(inputs=model.inputs, outputs=layer.output)
activations = feature_extractor.predict(img) 
print(activations.shape)

filters, biases = layer.get_weights()

fig, ((conv_ax1, conv_ax2), (conv_ax3, conv_ax4)) = plt.subplots(2, 2)
ax = conv_ax1.imshow(filters[:, :, 0, 0], cmap = "gray")
ax = conv_ax2.imshow(filters[:, :, 0, 1], cmap = "gray")
ax = conv_ax3.imshow(filters[:, :, 0, 2], cmap = "gray")
ax = conv_ax4.imshow(filters[:, :, 0, 3], cmap = "gray")

fig, ((act_ax1, act_ax2), (act_ax3, act_ax4)) = plt.subplots(2, 2)
ax = act_ax1.imshow(activations[0, :, :, 0], cmap = "gray")
ax = act_ax2.imshow(activations[0, :, :, 1], cmap = "gray")
ax = act_ax3.imshow(activations[0, :, :, 2], cmap = "gray")
ax = act_ax4.imshow(activations[0, :, :, 3], cmap = "gray")

layer = model.get_layer(name="mnist_conv2")
feature_extractor = tf.keras.Model(inputs=model.inputs, outputs=layer.output)
activations = feature_extractor.predict(img) 
print(activations.shape)

filters, biases = layer.get_weights()

fig, ((conv_ax1, conv_ax2), (conv_ax3, conv_ax4)) = plt.subplots(2, 2)
ax = conv_ax1.imshow(filters[:, :, 0, 0], cmap = "gray")
ax = conv_ax2.imshow(filters[:, :, 0, 1], cmap = "gray")
ax = conv_ax3.imshow(filters[:, :, 0, 2], cmap = "gray")
ax = conv_ax4.imshow(filters[:, :, 0, 3], cmap = "gray")

fig, ((act_ax1, act_ax2), (act_ax3, act_ax4)) = plt.subplots(2, 2)
ax = act_ax1.imshow(activations[0, :, :, 0], cmap = "gray")
ax = act_ax2.imshow(activations[0, :, :, 1], cmap = "gray")
ax = act_ax3.imshow(activations[0, :, :, 2], cmap = "gray")
ax = act_ax4.imshow(activations[0, :, :, 3], cmap = "gray")

plt.show()
