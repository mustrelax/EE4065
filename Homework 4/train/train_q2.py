import numpy as np
import cv2
import tensorflow as tf
from sklearn.metrics import confusion_matrix, ConfusionMatrixDisplay
import matplotlib.pyplot as plt

print("MNIST DATASET LOADING...")
(train_images, train_labels), (test_images, test_labels) = tf.keras.datasets.mnist.load_data()

def extract_hu_moments(images):
    hu_features = np.empty((len(images), 7))
    for i, img in enumerate(images):
        moments = cv2.moments(img, binaryImage=False)
        hu = cv2.HuMoments(moments).reshape(7)
        hu_features[i] = hu
    return hu_features

train_huMoments = extract_hu_moments(train_images)
test_huMoments = extract_hu_moments(test_images)

features_mean = np.mean(train_huMoments, axis=0)
features_std = np.std(train_huMoments, axis=0)
features_std[features_std == 0] = 1.0

train_huMoments = (train_huMoments - features_mean) / features_std
test_huMoments = (test_huMoments - features_mean) / features_std

model = tf.keras.models.Sequential([
    tf.keras.layers.Dense(100, input_shape=[7], activation='relu'),
    tf.keras.layers.Dense(100, activation='relu'),
    tf.keras.layers.Dense(10, activation='softmax')
])

model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate=1e-4),
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

callback = tf.keras.callbacks.EarlyStopping(monitor='loss', patience=5)

print("TRAINING MODEL...")
history = model.fit(train_huMoments, 
                    train_labels, 
                    epochs=200, 
                    batch_size=32, 
                    callbacks=[callback],
                    verbose=1,
                    validation_split=0.1)


plt.figure(figsize=(12, 5))
plt.subplot(1, 2, 1)
plt.plot(history.history['loss'], label='Training Loss')
plt.plot(history.history['val_loss'], label='Validation Loss')
plt.title('MLP Loss over Epochs')
plt.xlabel('Epochs')
plt.ylabel('Loss')
plt.legend()
plt.grid(True)

plt.subplot(1, 2, 2)
plt.plot(history.history['accuracy'], label='Training Accuracy')
plt.plot(history.history['val_accuracy'], label='Validation Accuracy')
plt.title('MLP Accuracy over Epochs')
plt.xlabel('Epochs')
plt.ylabel('Accuracy')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()



predictions = model.predict(test_huMoments)
predicted_classes = np.argmax(predictions, axis=1)
cm = confusion_matrix(test_labels, predicted_classes)
disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=np.arange(10))

fig, ax = plt.subplots(figsize=(10, 10))
disp.plot(cmap=plt.cm.Greens, ax=ax)
plt.title("MLP Confusion Matrix")
plt.show()

loss, acc = model.evaluate(test_huMoments, test_labels, verbose=0)
print(f"Final Test Accuracy: {acc*100:.2f}%")