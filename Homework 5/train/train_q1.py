import tensorflow as tf
import numpy as np
import librosa
import os
import glob
from sklearn.model_selection import train_test_split
import matplotlib.pyplot as plt
from sklearn.metrics import confusion_matrix, ConfusionMatrixDisplay


DATASET_PATH = "C:/Users/mustrelax/Desktop/EE4065/Homework 5/train/dataset"
SAMPLE_RATE = 8000
DURATION = 1.0
N_MFCC = 13
MAX_FRAMES = 20 

def extract_features(file_path):
    # Load audio
    audio, sr = librosa.load(file_path, sr=SAMPLE_RATE)
    
    # Pad or truncate to specific duration to ensure fixed input size
    max_len = int(SAMPLE_RATE * DURATION)
    if len(audio) < max_len:
        audio = np.pad(audio, (0, max_len - len(audio)))
    else:
        audio = audio[:max_len]
        
    # Compute MFCC
    mfccs = librosa.feature.mfcc(y=audio, sr=sr, n_mfcc=N_MFCC)
    if mfccs.shape[1] < MAX_FRAMES:
        mfccs = np.pad(mfccs, ((0, 0), (0, MAX_FRAMES - mfccs.shape[1])))
    else:
        mfccs = mfccs[:, :MAX_FRAMES]
        
    return mfccs.flatten()

print("Loading Dataset...")
files = glob.glob(os.path.join(DATASET_PATH, "*.wav"))
X = []
y = []

for file_path in files:
    filename = os.path.basename(file_path)
    # Filename format: digit_speaker_index.wav
    label = int(filename.split('_')[0]) 
    
    features = extract_features(file_path)
    X.append(features)
    y.append(label)

X = np.array(X)
y = np.array(y)

# Split data
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

print(f"Input Shape: {X_train.shape[1]}")

# --- MODEL DEFINITION ---
model = tf.keras.Sequential([
    tf.keras.layers.Dense(64, activation='relu', input_shape=(X_train.shape[1],)),
    tf.keras.layers.Dropout(0.2),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dense(10, activation='softmax')
])

model.compile(optimizer='adamW', loss='sparse_categorical_crossentropy', metrics=['accuracy'])

print("Training Audio Model...")
history = model.fit(X_train, y_train, epochs=200, batch_size=32, validation_data=(X_test, y_test))

# --- TFLITE CONVERSION ---
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

with open('audio_model.tflite', 'wb') as f:
    f.write(tflite_model)

print("SAVED: audio_model.tflite")
print("-" * 30)
print(f"#define MFCC_NUM_COEFFS {N_MFCC}")
print(f"#define MFCC_NUM_FRAMES {MAX_FRAMES}")
print("-" * 30)


plt.figure(figsize=(12, 5))

plt.subplot(1, 2, 1)
plt.plot(history.history['loss'], label='Training Loss')
plt.plot(history.history['val_loss'], label='Validation Loss')
plt.title('Audio Model Loss')
plt.xlabel('Epochs')
plt.ylabel('Loss')
plt.legend()
plt.grid(True)

plt.subplot(1, 2, 2)
plt.plot(history.history['accuracy'], label='Training Accuracy')
plt.plot(history.history['val_accuracy'], label='Validation Accuracy')
plt.title('Audio Model Accuracy')
plt.xlabel('Epochs')
plt.ylabel('Accuracy')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()

y_pred_probs = model.predict(X_test)
y_pred_classes = np.argmax(y_pred_probs, axis=1)

cm = confusion_matrix(y_test, y_pred_classes)

disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=np.arange(10))
fig, ax = plt.subplots(figsize=(8, 8))
disp.plot(cmap=plt.cm.Blues, ax=ax)
plt.title("Audio Model Confusion Matrix")
plt.show()