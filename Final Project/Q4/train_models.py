import tensorflow as tf
import numpy as np
import os
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.metrics import confusion_matrix, classification_report
from mobilenetv1 import get_mobilenetv1
from resnetv1 import get_resnetv1
from squeezenetv11 import get_squeezenetv11

# Configuration
IMG_SIZE = 64
BATCH_SIZE = 32
EPOCHS = 20
NUM_CLASSES = 10
MODELS_DIR = "models"
PLOTS_DIR = "plots"

def load_data():
    print("Loading MNIST dataset...")
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()

    # Normalize to [0, 1]
    x_train = x_train.astype("float32") / 255.0
    x_test = x_test.astype("float32") / 255.0

    # Add channel dimension
    x_train = np.expand_dims(x_train, axis=-1)
    x_test = np.expand_dims(x_test, axis=-1)

    return (x_train, y_train), (x_test, y_test)

def create_datasets(x_train, y_train, x_test, y_test):
    def preprocess(image, label):
        # Resize to 64x64
        image = tf.image.resize(image, [IMG_SIZE, IMG_SIZE])
        return image, label

    train_ds = tf.data.Dataset.from_tensor_slices((x_train, y_train))
    train_ds = train_ds.map(preprocess, num_parallel_calls=tf.data.AUTOTUNE)
    train_ds = train_ds.shuffle(10000).batch(BATCH_SIZE).prefetch(tf.data.AUTOTUNE)

    test_ds = tf.data.Dataset.from_tensor_slices((x_test, y_test))
    test_ds = test_ds.map(preprocess, num_parallel_calls=tf.data.AUTOTUNE)
    test_ds = test_ds.batch(BATCH_SIZE).prefetch(tf.data.AUTOTUNE)

    return train_ds, test_ds

def save_plots(history, y_true, y_pred_classes, model_name):
    if not os.path.exists(PLOTS_DIR):
        os.makedirs(PLOTS_DIR)

    # 1. Accuracy & Loss
    plt.figure(figsize=(12, 5))
    
    # Accuracy
    plt.subplot(1, 2, 1)
    plt.plot(history.history['accuracy'], label='Train Accuracy')
    plt.plot(history.history['val_accuracy'], label='Val Accuracy')
    plt.title(f'{model_name} Accuracy')
    plt.xlabel('Epochs')
    plt.ylabel('Accuracy')
    plt.legend()
    plt.grid(True)

    # Loss
    plt.subplot(1, 2, 2)
    plt.plot(history.history['loss'], label='Train Loss')
    plt.plot(history.history['val_loss'], label='Val Loss')
    plt.title(f'{model_name} Loss')
    plt.xlabel('Epochs')
    plt.ylabel('Loss')
    plt.legend()
    plt.grid(True)

    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, f"{model_name}_metrics.png"))
    plt.close()

    # 2. Confusion Matrix
    cm = confusion_matrix(y_true, y_pred_classes)
    plt.figure(figsize=(10, 8))
    sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', xticklabels=range(10), yticklabels=range(10))
    plt.title(f'{model_name} Confusion Matrix')
    plt.xlabel('Predicted')
    plt.ylabel('True')
    plt.savefig(os.path.join(PLOTS_DIR, f"{model_name}_confusion_matrix.png"))
    plt.close()

    # Print Classification Report
    print(f"\n--- Classification Report for {model_name} ---")
    print(classification_report(y_true, y_pred_classes, digits=4))

def train_and_evaluate(model, train_ds, test_ds, y_test, model_name, input_shape):
    print(f"\n========================================")
    print(f"Training {model_name}...")
    print(f"========================================")
    
    model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])
    
    history = model.fit(train_ds, epochs=EPOCHS, validation_data=test_ds)
    
    # Save Model
    model.save(os.path.join(MODELS_DIR, f"{model_name}.keras"))
    print(f"{model_name} saved.")

    # Evaluate on Test Set
    print(f"Evaluating {model_name}...")
    # Predict
    y_pred_probs = model.predict(test_ds)
    y_pred_classes = np.argmax(y_pred_probs, axis=1)

    # Generate Plots
    save_plots(history, y_test, y_pred_classes, model_name)

def main():
    if not os.path.exists(MODELS_DIR):
        os.makedirs(MODELS_DIR)

    (x_train, y_train), (x_test, y_test) = load_data()
    train_ds, test_ds = create_datasets(x_train, y_train, x_test, y_test)
    input_shape = (IMG_SIZE, IMG_SIZE, 1)

    # 1. MobileNet
    mobilenet = get_mobilenetv1(input_shape=input_shape, alpha=0.25, num_classes=NUM_CLASSES, pretrained_weights=None)
    train_and_evaluate(mobilenet, train_ds, test_ds, y_test, "mobilenetv1", input_shape)

    # 2. ResNet
    # ResNet20
    resnet = get_resnetv1(input_shape=input_shape, depth=20, num_classes=NUM_CLASSES)
    train_and_evaluate(resnet, train_ds, test_ds, y_test, "resnetv1", input_shape)

    # 3. SqueezeNet
    squeezenet = get_squeezenetv11(input_shape=input_shape, num_classes=NUM_CLASSES)
    train_and_evaluate(squeezenet, train_ds, test_ds, y_test, "squeezenetv11", input_shape)

if __name__ == "__main__":
    main()
