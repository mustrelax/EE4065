import os
import sys
import numpy as np
import tensorflow as tf
from tensorflow import keras
import matplotlib
matplotlib.use('Agg') 
import matplotlib.pyplot as plt
from sklearn.metrics import confusion_matrix, classification_report, accuracy_score, precision_score

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'suplementary'))

from resnetv1 import get_resnetv1
from squeezenetv11 import get_squeezenetv11
from efficientnetv2 import get_efficientnetv2
from mobilenetv1 import get_mobilenetv1
from mobilenetv2 import get_mobilenetv2
from fdmobilenet import get_fdmobilenet
from st_efficientnet_lc_v1 import get_st_efficientnet_lc_v1
from st_fdmobilenet_v1 import get_st_fdmobilenet_v1


# Configuration
NUM_CLASSES = 10
DATA_SHAPE = (32, 32, 3)
EPOCHS = 20
BATCH_SIZE = 64
PATIENCE = 5
MODELS_DIR = os.path.join(os.path.dirname(__file__), 'models')
RESULTS_DIR = os.path.join(os.path.dirname(__file__), 'results')


def prepare_tensor(images, out_shape):
    images = tf.expand_dims(images, axis=-1)
    images = tf.repeat(images, 3, axis=-1)
    images = tf.image.resize(images, out_shape[:2])
    images = images / 255.0
    return images


def load_mnist_data():
    print("Loading MNIST dataset...")
    (train_images, train_labels), (val_images, val_labels) = tf.keras.datasets.mnist.load_data()
    
    print("Preprocessing data...")
    train_images = prepare_tensor(train_images, DATA_SHAPE)
    val_images = prepare_tensor(val_images, DATA_SHAPE)
    
    train_labels = tf.keras.utils.to_categorical(train_labels, NUM_CLASSES)
    val_labels = tf.keras.utils.to_categorical(val_labels, NUM_CLASSES)
    
    print(f"Training samples: {len(train_images)}")
    print(f"Validation samples: {len(val_images)}")
    print(f"Image shape: {train_images.shape[1:]}")
    
    return (train_images, train_labels), (val_images, val_labels)


def create_callbacks(model_name):
    model_path = os.path.join(MODELS_DIR, f'{model_name}.h5')
    
    callbacks = [
        keras.callbacks.ModelCheckpoint(
            model_path,
            save_best_only=True,
            monitor='val_accuracy',
            verbose=1
        ),
        keras.callbacks.EarlyStopping(
            monitor='val_accuracy',
            patience=PATIENCE,
            verbose=1,
            restore_best_weights=True
        ),
        keras.callbacks.ReduceLROnPlateau(
            monitor='val_loss',
            factor=0.5,
            patience=3,
            verbose=1,
            min_lr=1e-6
        )
    ]
    return callbacks


def train_model(model, model_name, train_data, val_data):
    print(f"\n{'='*60}")
    print(f"Training {model_name}")
    print(f"{'='*60}")
    
    # Compile model
    model.compile(
        optimizer=keras.optimizers.Adam(learning_rate=1e-3),
        loss='categorical_crossentropy',
        metrics=['accuracy']
    )
    
    model.summary()
    
    # Train
    train_images, train_labels = train_data
    val_images, val_labels = val_data
    
    history = model.fit(
        x=train_images,
        y=train_labels,
        epochs=EPOCHS,
        batch_size=BATCH_SIZE,
        validation_data=(val_images, val_labels),
        callbacks=create_callbacks(model_name),
        verbose=1
    )
    
    # Evaluate
    loss, accuracy = model.evaluate(val_images, val_labels, verbose=0)
    print(f"\n{model_name} - Final Validation Accuracy: {accuracy*100:.2f}%")
    
    return model, history, accuracy


def evaluate_and_plot_metrics(model, model_name, val_images, val_labels):
    os.makedirs(RESULTS_DIR, exist_ok=True)
    
    # Get predictions
    y_pred_prob = model.predict(val_images, verbose=0)
    y_pred = np.argmax(y_pred_prob, axis=1)
    y_true = np.argmax(val_labels, axis=1)
    
    # Compute metrics
    acc = accuracy_score(y_true, y_pred)
    prec = precision_score(y_true, y_pred, average='weighted')
    
    print(f"\n{model_name} Metrics:")
    print(f"  Accuracy:  {acc*100:.2f}%")
    print(f"  Precision: {prec*100:.2f}%")
    
    # Confusion matrix
    cm = confusion_matrix(y_true, y_pred)
    
    # Plot confusion matrix
    plt.figure(figsize=(8, 6))
    plt.imshow(cm, interpolation='nearest', cmap=plt.cm.Blues)
    plt.title(f'{model_name} Confusion Matrix')
    plt.colorbar()
    plt.xlabel('Predicted')
    plt.ylabel('True')
    plt.xticks(range(10))
    plt.yticks(range(10))
    
    # Add text annotations
    for i in range(10):
        for j in range(10):
            plt.text(j, i, str(cm[i, j]), ha='center', va='center',
                     color='white' if cm[i, j] > cm.max()/2 else 'black')
    
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f'{model_name}_confusion_matrix.png'), dpi=150)
    plt.close()
    
    print(f"  Saved: {model_name}_confusion_matrix.png")
    
    return acc, prec


def plot_training_history(history, model_name):
    os.makedirs(RESULTS_DIR, exist_ok=True)
    
    # Accuracy plot
    plt.figure(figsize=(10, 4))
    
    plt.subplot(1, 2, 1)
    plt.plot(history.history['accuracy'], label='Train')
    plt.plot(history.history['val_accuracy'], label='Validation')
    plt.title(f'{model_name} - Accuracy')
    plt.xlabel('Epoch')
    plt.ylabel('Accuracy')
    plt.legend()
    plt.grid(True)
    
    # Loss plot
    plt.subplot(1, 2, 2)
    plt.plot(history.history['loss'], label='Train')
    plt.plot(history.history['val_loss'], label='Validation')
    plt.title(f'{model_name} - Loss')
    plt.xlabel('Epoch')
    plt.ylabel('Loss')
    plt.legend()
    plt.grid(True)
    
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f'{model_name}_training_curves.png'), dpi=150)
    plt.close()
    
    print(f"  Saved: {model_name}_training_curves.png")


def get_all_models():
    models = {}
    
    # ResNet V1
    print("Creating ResNet V1...")
    try:
        models['resnet_v1'] = get_resnetv1(
            num_classes=NUM_CLASSES,
            input_shape=DATA_SHAPE,
            depth=8,
            dropout=0.2
        )
    except Exception as e:
        print(f"Error creating ResNet: {e}")
    
    # SqueezeNet V1.1
    print("Creating SqueezeNet V1.1...")
    try:
        models['squeezenet_v11'] = get_squeezenetv11(
            num_classes=NUM_CLASSES,
            input_shape=DATA_SHAPE,
            dropout=0.5
        )
    except Exception as e:
        print(f"Error creating SqueezeNet: {e}")
    
    # EfficientNet V2-B0
    print("Creating EfficientNet V2-B0...")
    try:
        models['efficientnet_v2_b0'] = get_efficientnetv2(
            input_shape=DATA_SHAPE,
            model_type='B0',
            num_classes=NUM_CLASSES,
            dropout=0.2,
            pretrained_weights=None  # Train from scratch
        )
    except Exception as e:
        print(f"Error creating EfficientNet: {e}")
    
    # MobileNetV1
    print("Creating MobileNetV1...")
    try:
        models['mobilenet_v1'] = get_mobilenetv1(
            input_shape=DATA_SHAPE,
            alpha=0.25,
            num_classes=NUM_CLASSES,
            dropout=0.2,
            pretrained_weights=None
        )
    except Exception as e:
        print(f"Error creating MobileNetV1: {e}")
    
    # MobileNetV2
    print("Creating MobileNetV2...")
    try:
        models['mobilenet_v2'] = get_mobilenetv2(
            input_shape=DATA_SHAPE,
            alpha=0.35,
            num_classes=NUM_CLASSES,
            dropout=0.2,
            pretrained_weights=None
        )
    except Exception as e:
        print(f"Error creating MobileNetV2: {e}")
    
    # FDMobileNet
    print("Creating FDMobileNet...")
    try:
        models['fdmobilenet'] = get_fdmobilenet(
            input_shape=DATA_SHAPE,
            num_classes=NUM_CLASSES,
            alpha=0.25,
            dropout=0.2
        )
    except Exception as e:
        print(f"Error creating FDMobileNet: {e}")
    
    # ST EfficientNet LC V1
    print("Creating ST EfficientNet LC V1...")
    try:
        models['st_efficientnet_lc_v1'] = get_st_efficientnet_lc_v1(
            input_shape=DATA_SHAPE,
            num_classes=NUM_CLASSES,
            dropout=0.2
        )
    except Exception as e:
        print(f"Error creating ST EfficientNet LC: {e}")
    
    # ST FDMobileNet V1
    print("Creating ST FDMobileNet V1...")
    try:
        models['st_fdmobilenet_v1'] = get_st_fdmobilenet_v1(
            input_shape=DATA_SHAPE,
            num_classes=NUM_CLASSES,
            dropout=0.2
        )
    except Exception as e:
        print(f"Error creating ST FDMobileNet: {e}")
    
    return models


def main():
    os.makedirs(MODELS_DIR, exist_ok=True)
    train_data, val_data = load_mnist_data()
    
    # Create models
    print("\n" + "="*60)
    print("Creating CNN Models")
    print("="*60)
    models = get_all_models()
    
    # Train each model
    results = {}
    val_images, val_labels = val_data
    
    for model_name, model in models.items():
        try:
            trained_model, history, accuracy = train_model(model, model_name, train_data, val_data)
            
            # Save training curves
            plot_training_history(history, model_name)
            
            # Compute metrics and save confusion matrix
            acc, prec = evaluate_and_plot_metrics(trained_model, model_name, val_images, val_labels)
            results[model_name] = {'accuracy': acc, 'precision': prec}
            
            # Clear session to free memory
            keras.backend.clear_session()
            
        except Exception as e:
            print(f"Error training {model_name}: {e}")
            results[model_name] = {'accuracy': 0.0, 'precision': 0.0}
    
    # Print final results
    print("\n" + "="*60)
    print("TRAINING RESULTS SUMMARY")
    print("="*60)
    print(f"{'Model':<25} {'Accuracy':>10} {'Precision':>10}")
    print("-"*47)
    for model_name, metrics in sorted(results.items(), key=lambda x: x[1]['accuracy'], reverse=True):
        print(f"{model_name:<25} {metrics['accuracy']*100:>9.2f}% {metrics['precision']*100:>9.2f}%")
    
    print(f"\nModels saved to: {MODELS_DIR}")
    return results

if __name__ == "__main__":
    main()
