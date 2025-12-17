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

train_labels_bin = np.where(train_labels == 0, 0, 1)
test_labels_bin = np.where(test_labels == 0, 0, 1)

model = tf.keras.models.Sequential([
    tf.keras.layers.Dense(1, input_shape=[7], activation='sigmoid')
])

model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate=1e-3),
              loss='binary_crossentropy',
              metrics=['accuracy'])

print("TRAINING MODEL...")
history = model.fit(train_huMoments, 
                    train_labels_bin, 
                    batch_size=128, 
                    epochs=50, 
                    class_weight={0: 8.0, 1: 1.0}, 
                    verbose=1,
                    validation_split=0.1)

plt.figure(figsize=(12, 5))
plt.subplot(1, 2, 1)
plt.plot(history.history['loss'], label='Training Loss')
plt.plot(history.history['val_loss'], label='Validation Loss')
plt.title('Model Loss over Epochs')
plt.xlabel('Epochs')
plt.ylabel('Loss')
plt.legend()
plt.grid(True)

plt.subplot(1, 2, 2)
plt.plot(history.history['accuracy'], label='Training Accuracy')
plt.plot(history.history['val_accuracy'], label='Validation Accuracy')
plt.title('Model Accuracy over Epochs')
plt.xlabel('Epochs')
plt.ylabel('Accuracy')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()

predictions = model.predict(test_huMoments)
predictions_bin = (predictions > 0.5).astype("int32")

cm = confusion_matrix(test_labels_bin, predictions_bin)
disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["Zero (0)", "Non-Zero"])

fig, ax = plt.subplots(figsize=(6, 6))
disp.plot(cmap=plt.cm.Blues, ax=ax)
plt.title("Confusion Matrix")
plt.show()

loss, acc = model.evaluate(test_huMoments, test_labels_bin, verbose=0)
print(f"Final Test Accuracy: {acc*100:.2f}%")