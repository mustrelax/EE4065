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

test_ind=10

im = test_images[test_ind]

# Add the image to a batch where it's the only member.
img = np.expand_dims(im, 0)

# First way of predicting the label

predictions = model.predict(img)

print("Predictions")
print(predictions)

predicted_label = np.argmax(predictions)
actual_label = np.argmax(test_labels[test_ind])

plt.figure()
plt.imshow(im, cmap=plt.cm.binary)
plt.title('actual label= ' + str(actual_label) + ', predicted label = '+ str(predicted_label))

plt.show()
