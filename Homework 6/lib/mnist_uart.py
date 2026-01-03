import argparse
import numpy as np
import cv2
import serial

SYNC_BYTE = 0xBB

def load_image(path):
    img = cv2.imread(path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        raise ValueError(f"Cannot load: {path}")
    img = cv2.resize(img, (28, 28))
    if np.mean(img) > 127:
        img = 255 - img
    return img

def send_and_receive(img, port='COM9', baudrate=115200):
    try:
        with serial.Serial(port, baudrate, timeout=5) as ser:
            ser.write(bytes([SYNC_BYTE]))
            ser.write(img.flatten().tobytes())
            print(f"Sent 784 bytes")
            result = ser.read(1)
            return result[0] if len(result) == 1 else -1
    except Exception as e:
        print(f"Error: {e}")
        return -1

def display(digit, img=None):
    canvas = np.zeros((600, 800, 3), dtype=np.uint8)
    cv2.putText(canvas, str(digit), (280, 400), cv2.FONT_HERSHEY_SIMPLEX, 15, (0, 255, 0), 30)
    cv2.putText(canvas, "Recognized Digit", (150, 60), cv2.FONT_HERSHEY_SIMPLEX, 1.5, (255,255,255), 3)
    if img is not None:
        canvas[80:220, 20:160] = cv2.cvtColor(cv2.resize(img, (140, 140), interpolation=cv2.INTER_NEAREST), cv2.COLOR_GRAY2BGR)
    cv2.imshow('Result', canvas)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('image')
    parser.add_argument('--port', default='COM9')
    args = parser.parse_args()
    
    img = load_image(args.image)
    print(f"Sending 28x28 image to MCU...")
    digit = send_and_receive(img, args.port)
    
    if 0 <= digit <= 9:
        print(f"*** Recognized: {digit} ***")
        display(digit, img)
    else:
        print("Failed")

if __name__ == '__main__':
    main()
