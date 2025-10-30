import numpy as np
import cv2
import os

IMAGE_WIDTH = 160
IMAGE_HEIGHT = 120

def reconstruct_from_bin(bin_filename):
    if not os.path.exists(bin_filename):
        print(f"Warning: '{bin_filename}' not found! Skipping...")
        return

    print(f"'{bin_filename}' processing...")

    pixel_data = np.fromfile(bin_filename, dtype=np.uint8)

    expected_size = IMAGE_HEIGHT * IMAGE_WIDTH
    if pixel_data.size != expected_size:
        print(f"Error: File size ({pixel_data.size} byte) unexpected. Expected file size: ({expected_size} byte).")
        return

    image_matrix = pixel_data.reshape((IMAGE_HEIGHT, IMAGE_WIDTH))
    output_png_filename = bin_filename.replace('.bin', '.png')

    cv2.imwrite(output_png_filename, image_matrix)
    print(f"-> Image '{output_png_filename}' saved.")

if __name__ == "__main__":
    
    bin_files = [f for f in os.listdir('.') if f.endswith('.bin')]

    if not bin_files:
        print("There is no .bin files to process.")
    else:
        for bf in bin_files:
            reconstruct_from_bin(bf)

        print("\nAll images constructed.")