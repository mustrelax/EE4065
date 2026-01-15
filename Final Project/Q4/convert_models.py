import tensorflow as tf
import numpy as np
import os

IMG_SIZE = 64
MODELS_DIR = "models"
HEADERS_DIR = "headers"

def load_data_for_calibration():
    print("Loading MNIST dataset for calibration...")
    (x_train, _), (_, _) = tf.keras.datasets.mnist.load_data()
    
    # Just take a subset for calibration
    x_train = x_train[:1000]
    
    # Normalize to [0, 1]
    x_train = x_train.astype("float32") / 255.0
    
    # Add channel dimension
    x_train = np.expand_dims(x_train, axis=-1)
    resized_images = []
    for img in x_train:
        # tf.image.resize expects 3D or 4D tensor
        img_tensor = tf.convert_to_tensor(img)
        img_resized = tf.image.resize(img_tensor, [IMG_SIZE, IMG_SIZE])
        resized_images.append(img_resized.numpy())
    
    return np.array(resized_images)

def convert_to_tflite_int8(model_path, calibration_data):
    print(f"Converting {model_path} to TFLite Int8...")
    
    # Load Keras model
    try:
        model = tf.keras.models.load_model(model_path)
    except Exception as e:
        print(f"Error loading model {model_path}: {e}")
        return None

    # Create converter
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    
    # Int8 Quantization settings
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    
    def rep_dataset():
        for i in range(len(calibration_data)):
            # Yield: [1, 64, 64, 1]
            yield [np.expand_dims(calibration_data[i], axis=0)]
            
    converter.representative_dataset = rep_dataset
    
    # Ensure full integer quantization
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    
    tflite_model = converter.convert()
    return tflite_model

def convert_to_c_array(tflite_model, model_name):
    # Hex dump
    hex_array = []
    for byte in tflite_model:
        hex_array.append(f"0x{byte:02x}")
    
    # Format C code
    c_code = f"// Auto-generated C header for {model_name}\n"
    c_code += f"#ifndef {model_name.upper()}_MODEL_H\n"
    c_code += f"#define {model_name.upper()}_MODEL_H\n\n"
    c_code += f"const unsigned char {model_name}_model[] = {{\n"
    
    # Group by 12 bytes per line for readability
    for i in range(0, len(hex_array), 12):
        line = ", ".join(hex_array[i:i+12])
        c_code += f"  {line},\n"
        
    c_code += "};\n\n"
    c_code += f"const unsigned int {model_name}_model_len = {len(tflite_model)};\n\n"
    c_code += "#endif\n"
    
    return c_code

def main():
    if not os.path.exists(HEADERS_DIR):
        os.makedirs(HEADERS_DIR)
        
    calibration_data = load_data_for_calibration()
    
    model_files = [
        "mobilenetv1.keras",
        "resnetv1.keras",
        "squeezenetv11.keras"
    ]
    
    for filename in model_files:
        model_name = filename.split('.')[0]
        model_path = os.path.join(MODELS_DIR, filename)
        
        if not os.path.exists(model_path):
            print(f"Model file not found: {model_path}. Skipping.")
            continue
            
        tflite_model = convert_to_tflite_int8(model_path, calibration_data)
        
        if tflite_model:
            # Save .tflite file
            tflite_path = os.path.join(HEADERS_DIR, f"{model_name}.tflite")
            with open(tflite_path, "wb") as f:
                f.write(tflite_model)
            print(f"Saved {tflite_path}")
                
            header_name = model_name.replace("v1", "").replace("v11", "") + "_model" 
            
            c_code = convert_to_c_array(tflite_model, header_name)
            
            header_path = os.path.join(HEADERS_DIR, f"{header_name}.h")
            with open(header_path, "w") as f:
                f.write(c_code)
            print(f"Saved {header_path}")
            
            # Verify first few lines
            if "mobilenet" in header_name:
                print(f"\n--- Verification: First 5 lines of {header_path} ---")
                print("\n".join(c_code.splitlines()[:5]))
                print("----------------------------------------------------\n")

if __name__ == "__main__":
    main()
