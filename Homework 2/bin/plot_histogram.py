# plot_histogram.py
import numpy as np
import matplotlib.pyplot as plt
import os

BIN_FILE_NAME = "equalized_histogram.bin"
PLOT_FILE_NAME = "equalized_histogram_plot.png"
EXPECTED_ELEMENTS = 256

def plot_histogram_from_bin(bin_file):
    if not os.path.exists(bin_file):
        print(f"Error: '{bin_file}' not found.")
        return

    hist_data = np.fromfile(bin_file, dtype=np.uint32)

    if hist_data.size != EXPECTED_ELEMENTS:
        print(f"Error: Read {hist_data.size} elements, but expected {EXPECTED_ELEMENTS}.")
        return
        
    print(f"Successfully read {hist_data.size} histogram bins.")

    # Create an array for the x-axis (0 to 255)
    pixel_levels = np.arange(EXPECTED_ELEMENTS)

    # Plot the histogram
    plt.figure(figsize=(10, 6))
    plt.bar(pixel_levels, hist_data, width=1.0)
    plt.title("Grayscale Image Equalized Histogram")
    plt.xlabel("Pixel Intensity Level (0-255)")
    plt.ylabel("Pixel Count")
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.xlim([0, 255])
    

    plt.savefig(PLOT_FILE_NAME)
    print(f"Plot saved as '{PLOT_FILE_NAME}'")
    
    plt.show()

if __name__ == "__main__":
    plot_histogram_from_bin(BIN_FILE_NAME)