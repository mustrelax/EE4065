import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 
import tensorflow as tf

from matplotlib import pyplot as plt
import numpy as np

#from efficientnetv2 import get_efficientnetv2 as EfficientNet
from st_efficientnet_lc_v1 import get_st_efficientnet_lc_v1 as EfficientNet

num_classes = 10
(train_images, train_labels), (
    test_images,
    test_labels,
) = tf.keras.datasets.mnist.load_data()
data_shape = (32, 32, 3)

def prepare_tensor(images, out_shape):
    images = tf.expand_dims(images, axis=-1)
    images = tf.repeat(images, 3, axis=-1)
    images = tf.image.resize(images, out_shape[:2])
    images = images / 255.0
    return images

train_images = prepare_tensor(train_images, data_shape)
test_images = prepare_tensor(test_images, data_shape)

# convert class vectors to binary class matrices
train_labels = tf.keras.utils.to_categorical(train_labels, num_classes)
test_labels = tf.keras.utils.to_categorical(test_labels, num_classes)

#def get_efficientnetv2(input_shape: tuple, model_type: str = None, num_classes: int = None, dropout: float = None,pretrained_weights: str = "imagenet")
#model = EfficientNet(data_shape, model_type="B0", num_classes=num_classes, pretrained_weights="imagenet")

#get_st_efficientnet_lc_v1(input_shape: Tuple[int, int, int] = None, num_classes: int = None, dropout: float = None)
model = EfficientNet(data_shape, num_classes=num_classes)

model.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])

history = model.fit(
    train_images,
    train_labels,
    batch_size=128,
    epochs=3,
    validation_data=(test_images, test_labels),
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

print(history.history.keys())
#  "Accuracy"
plt.figure()
plt.plot(history.history['accuracy'])
plt.plot(history.history['val_accuracy'])
plt.title('model accuracy')
plt.ylabel('accuracy')
plt.xlabel('epoch')
plt.legend(['train', 'validation'], loc='upper left')
# "Loss"
plt.figure()
plt.plot(history.history['loss'])
plt.plot(history.history['val_loss'])
plt.title('model loss')
plt.ylabel('loss')
plt.xlabel('epoch')
plt.legend(['train', 'validation'], loc='upper left')
plt.show()