import argparse
import numpy as np
import cv2
import serial
import librosa


SAMPLE_RATE = 8000 
DURATION = 1.0
N_MFCC = 13
MAX_FRAMES = 20
INPUT_SIZE = N_MFCC * MAX_FRAMES

SYNC_BYTE = 0xAA
INPUT_SIZE_BYTES = INPUT_SIZE * 4


# MFCC Feature Extraction 
def extract_features(file_path):

    # Load audio at 8kHz
    audio, sr = librosa.load(file_path, sr=SAMPLE_RATE)
    
    # Pad or truncate to 1 second
    max_len = int(SAMPLE_RATE * DURATION)
    if len(audio) < max_len:
        audio = np.pad(audio, (0, max_len - len(audio)))
    else:
        audio = audio[:max_len]
    
    # Compute MFCC using librosa
    mfccs = librosa.feature.mfcc(y=audio, sr=sr, n_mfcc=N_MFCC)
    
    # Fix time dimension to MAX_FRAMES
    if mfccs.shape[1] < MAX_FRAMES:
        mfccs = np.pad(mfccs, ((0, 0), (0, MAX_FRAMES - mfccs.shape[1])))
    else:
        mfccs = mfccs[:, :MAX_FRAMES]
    
    return mfccs.flatten().astype(np.float32)

# UART Communication
def send_features_and_receive_result(features, port='COM9', baudrate=115200, timeout=5):
    try:
        with serial.Serial(port, baudrate, timeout=timeout) as ser:
            # Send sync byte
            ser.write(bytes([SYNC_BYTE]))
            print(f"Sent sync byte: 0x{SYNC_BYTE:02X}")
            
            # Send features as little-endian float32
            feature_bytes = features.astype('<f4').tobytes()
            ser.write(feature_bytes)
            print(f"Sent {len(feature_bytes)} bytes of features")
            
            # Wait for result
            result = ser.read(1)
            if len(result) == 1:
                digit = result[0]
                print(f"Received digit: {digit}")
                return digit
            else:
                print("Timeout")
                return -1
                
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return -1


# OpenCV Display
def display_digit(digit, duration=3000):

    width, height = 800, 600
    img = np.zeros((height, width, 3), dtype=np.uint8)
    
    text = str(digit)
    font = cv2.FONT_HERSHEY_SIMPLEX
    font_scale = 15
    thickness = 30
    color = (0, 255, 0)

    (text_width, text_height), baseline = cv2.getTextSize(text, font, font_scale, thickness)
    x = (width - text_width) // 2
    y = (height + text_height) // 2
    
    cv2.putText(img, text, (x, y), font, font_scale, color, thickness, cv2.LINE_AA)
    
    title = "Recognized Digit"
    title_font_scale = 1.5
    title_thickness = 3
    (title_width, _), _ = cv2.getTextSize(title, font, title_font_scale, title_thickness)
    cv2.putText(img, title, ((width - title_width) // 2, 60), font, 
                title_font_scale, (255, 255, 255), title_thickness, cv2.LINE_AA)

    cv2.imshow('Keyword Spotting Result', img)
    
    if duration > 0:
        cv2.waitKey(duration)
    else:
        print("Press any key to close the window")
        cv2.waitKey(0)
    
    cv2.destroyAllWindows()

# Main
def main():
    parser = argparse.ArgumentParser(description='Keyword Spotting UART Client')
    parser.add_argument('wav_file', nargs='?', help='Path to WAV file')
    parser.add_argument('--port', default='COM9', help='COM port')
    parser.add_argument('--baudrate', type=int, default=115200, help='Baud rate (default: 115200)')
    
    args = parser.parse_args()
    
    if not args.wav_file:
        parser.print_help()
        return
    
    print(f"\n{'='*50}")
    print("Keyword Spotting UART Client")
    print(f"{'='*50}")
    
    print(f"\nLoading: {args.wav_file}")
    try:
        features = extract_features(args.wav_file)
        print(f"Features: {len(features)} values")
        print(f"Range: [{features.min():.3f}, {features.max():.3f}]")
    except Exception as e:
        print(f"Error extracting features: {e}")
        display_error("Failed to extract features")
        return
    
    if len(features) != INPUT_SIZE:
        print(f"Warning: Expected {INPUT_SIZE} features, got {len(features)}")
    
    print(f"\nSending to MCU via {args.port}...")
    digit = send_features_and_receive_result(features, args.port, args.baudrate)
    
    if digit >= 0 and digit <= 9:
        print(f"\n*** Recognized Digit: {digit} ***")
        display_digit(digit, duration=0)
    else:
        print("\nFailed to get valid result from MCU")

if __name__ == '__main__':
    main()
