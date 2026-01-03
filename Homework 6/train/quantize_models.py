import os
import sys
import numpy as np
import tensorflow as tf
from tensorflow import keras

# Configuration
MODELS_DIR = os.path.join(os.path.dirname(__file__), 'models')
DATA_SHAPE = (32, 32, 3)

def prepare_tensor(images, out_shape):
    images = tf.expand_dims(images, axis=-1)
    images = tf.repeat(images, 3, axis=-1)
    images = tf.image.resize(images, out_shape[:2])
    images = images / 255.0
    return images

def get_representative_dataset():
    print("Loading MNIST for representative dataset...")
    (train_images, _), _ = tf.keras.datasets.mnist.load_data()
    
    # Preprocess a subset of images
    # We use 100 samples as recommended for representative datasets
    num_calibration_steps = 100
    
    # Shuffle and pick a subset
    indices = np.random.choice(len(train_images), num_calibration_steps, replace=False)
    calibration_images = train_images[indices]
    
    # Preprocess
    print("Preprocessing representative data...")
    calibration_images = prepare_tensor(calibration_images, DATA_SHAPE)
    
    def representative_data_gen():
        for input_value in calibration_images:
            # Model expects [1, 32, 32, 3]
            input_value = tf.expand_dims(input_value, axis=0)
            yield [input_value]
            
    return representative_data_gen

def quantize_model(h5_path):
    base_name = os.path.splitext(os.path.basename(h5_path))[0]
    output_path = os.path.join(os.path.dirname(h5_path), f'{base_name}_quant.tflite')
    
    try:
        print(f"\nProcessing: {base_name}")
        
        # Load Keras model
        model = keras.models.load_model(h5_path)
        
        # Create converter
        converter = tf.lite.TFLiteConverter.from_keras_model(model)
        
        # Set optimization flags
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        
        # Set representative dataset
        converter.representative_dataset = get_representative_dataset()
        
        # Ensure full integer quantization
        converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
        
        # Convert
        print("Converting with quantization...")
        tflite_model = converter.convert()
        
        # Save
        with open(output_path, 'wb') as f:
            f.write(tflite_model)
            
        original_size = os.path.getsize(h5_path) / 1024
        quant_size = len(tflite_model) / 1024
        
        print(f"Saved: {output_path}")
        print(f"Original H5 size: {original_size:.1f} KB")
        print(f"Quantized size:   {quant_size:.1f} KB")
        print(f"Reduction ratio:  {original_size/quant_size:.2f}x")
        
        return {
            'name': base_name,
            'original_size': original_size,
            'quant_size': quant_size
        }
        
    except Exception as e:
        print(f"Error converting {base_name}: {e}")
        return None

def main():
    if not os.path.exists(MODELS_DIR):
        print(f"Models directory not found: {MODELS_DIR}")
        return
        
    h5_files = [f for f in os.listdir(MODELS_DIR) if f.endswith('.h5')]
    
    if not h5_files:
        print("No .h5 files found to quantize.")
        return
        
    results = []
    for f in h5_files:
        h5_path = os.path.join(MODELS_DIR, f)
        res = quantize_model(h5_path)
        if res:
            results.append(res)
            
    # Summary
    print("\n" + "="*60)
    print("QUANTIZATION SUMMARY")
    print("="*60)
    print(f"{'Model':<25} {'Orig (KB)':>10} {'Quant (KB)':>12} {'Ratio':>8}")
    print("-"*57)
    
    for r in results:
        ratio = r['original_size'] / r['quant_size']
        print(f"{r['name']:<25} {r['original_size']:>10.1f} {r['quant_size']:>12.1f} {ratio:>8.2f}x")

if __name__ == "__main__":
    main()
