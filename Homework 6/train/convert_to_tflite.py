import os
import sys
import tensorflow as tf
from tensorflow import keras

# Import tflite2cc converter
from tflite2cc import convert_tflite2cc


# Configuration
MODELS_DIR = os.path.join(os.path.dirname(__file__), 'models')


def convert_model_to_tflite(h5_path, output_dir=None):

    if output_dir is None:
        output_dir = os.path.dirname(h5_path)
    
    base_name = os.path.splitext(os.path.basename(h5_path))[0]
    tflite_path = os.path.join(output_dir, f'{base_name}.tflite')
    cc_path = os.path.join(output_dir, f'{base_name}.cc')
    
    try:
        print(f"\nConverting: {h5_path}")
        
        # Load the Keras model
        model = keras.models.load_model(h5_path)
        print(f"Model loaded successfully")
        
        # Create TFLite converter
        converter = tf.lite.TFLiteConverter.from_keras_model(model)
        
        # Apply optimizations
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        
        # Convert the model
        tflite_model = converter.convert()
        
        # Save the TFLite model
        with open(tflite_path, 'wb') as f:
            f.write(tflite_model)
        
        original_size = os.path.getsize(h5_path) / 1024
        tflite_size = len(tflite_model) / 1024
        
        print(f"TFLite model saved: {tflite_path}")
        print(f"Original size: {original_size:.1f} KB")
        print(f"TFLite size: {tflite_size:.1f} KB")
        print(f"Compression ratio: {original_size/tflite_size:.2f}x")
        
        # Convert to C++ header
        convert_tflite2cc(tflite_path, cc_path)
        
        return tflite_path, cc_path
        
    except Exception as e:
        print(f"Error converting {h5_path}: {e}")
        return None, None


def convert_all_models():
    print("="*60)
    print("TensorFlow Lite Model Conversion")
    print("="*60)
    
    if not os.path.exists(MODELS_DIR):
        print(f"Models directory not found: {MODELS_DIR}")
        print("Please run train_all_models.py first.")
        return
    
    # Find all .h5 files
    h5_files = [f for f in os.listdir(MODELS_DIR) if f.endswith('.h5')]
    
    if not h5_files:
        print("No .h5 model files found in models directory.")
        print("Please run train_all_models.py first.")
        return
    
    print(f"Found {len(h5_files)} model(s) to convert:")
    for f in h5_files:
        print(f"  - {f}")
    
    # Convert each model
    results = []
    for h5_file in h5_files:
        h5_path = os.path.join(MODELS_DIR, h5_file)
        tflite_path, cc_path = convert_model_to_tflite(h5_path)
        
        if tflite_path:
            results.append({
                'name': os.path.splitext(h5_file)[0],
                'h5_path': h5_path,
                'tflite_path': tflite_path,
                'cc_path': cc_path,
                'h5_size': os.path.getsize(h5_path) / 1024,
                'tflite_size': os.path.getsize(tflite_path) / 1024
            })
    
    # Print summary
    print("\n" + "="*60)
    print("CONVERSION SUMMARY")
    print("="*60)
    print(f"{'Model':<25} {'H5 Size':>10} {'TFLite Size':>12} {'Ratio':>8}")
    print("-"*55)
    
    for r in results:
        ratio = r['h5_size'] / r['tflite_size']
        print(f"{r['name']:<25} {r['h5_size']:>9.1f}K {r['tflite_size']:>11.1f}K {ratio:>7.2f}x")
    
    print(f"\nAll files saved to: {MODELS_DIR}")
    print("\nGenerated files:")
    print("  - .tflite files: TensorFlow Lite models")
    print("  - .cc files: C++ header files for microcontroller")


def main():
    if len(sys.argv) > 1:
        h5_path = sys.argv[1]
        if os.path.exists(h5_path):
            convert_model_to_tflite(h5_path)
        else:
            print(f"Model file not found: {h5_path}")
    else:
        convert_all_models()


if __name__ == "__main__":
    main()
