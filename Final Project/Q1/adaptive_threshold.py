"""
Question 1-a: Size-Based Adaptive Thresholding

This script finds the optimal threshold to isolate a bright object 
of approximately a target size (pixel count) from a dark background.
"""

import cv2
import numpy as np
import matplotlib.pyplot as plt


def find_threshold_by_area(gray_image, target_area=1000, tolerance_percent=1, max_iterations=16):
    """
    Find the threshold value that results in approximately target_area white pixels.
    
    Uses binary search for efficiency.
    
    Args:
        gray_image: Grayscale input image
        target_area: Target number of white pixels
        tolerance_percent: Acceptable error as percentage of target (default 1%)
        max_iterations: Maximum search iterations
    
    Returns:
        tuple: (threshold_value, binary_image, pixel_count)
    """
    low, high = 0, 255
    best_threshold = 128
    best_error = gray_image.size
    best_binary = None
    
    tolerance = max(1, int(target_area * tolerance_percent / 100))
    
    for iteration in range(max_iterations):
        mid = (low + high) // 2
        
        # Apply threshold
        _, binary = cv2.threshold(gray_image, mid, 255, cv2.THRESH_BINARY)
        
        # Count white pixels
        pixel_count = cv2.countNonZero(binary)
        error = abs(pixel_count - target_area)
        
        print(f"Iter {iteration}: T={mid}, Count={pixel_count}, Error={error}")
        
        # Track best result
        if error < best_error:
            best_error = error
            best_threshold = mid
            best_binary = binary.copy()
        
        # Check if within tolerance
        if error <= tolerance:
            print(f"Found threshold {mid} within tolerance ({tolerance})")
            return mid, binary, pixel_count
        
        # Binary search adjustment
        if pixel_count > target_area:
            # Too many white pixels, increase threshold (stricter)
            low = mid + 1
        else:
            # Too few white pixels, decrease threshold (looser)
            high = mid - 1
        
        # Check if search space exhausted
        if low > high:
            break
    
    print(f"Best threshold: {best_threshold} with error: {best_error}")
    
    # Recompute with best threshold
    _, best_binary = cv2.threshold(gray_image, best_threshold, 255, cv2.THRESH_BINARY)
    return best_threshold, best_binary, cv2.countNonZero(best_binary)


def main():
    # Load image
    image_path = 'test_image.jpg'
    original = cv2.imread(image_path)
    
    if original is None:
        print(f"Error: Could not load image '{image_path}'")
        print("Creating a synthetic test image...")
        
        # Create synthetic image: dark background with bright circle
        original = np.zeros((240, 320, 3), dtype=np.uint8)
        cv2.circle(original, (160, 120), 18, (255, 255, 255), -1)  # ~1000 pixels
    
    # Convert to grayscale
    gray = cv2.cvtColor(original, cv2.COLOR_BGR2GRAY)
    
    # Set target area
    target_area = 1000
    
    print(f"\n=== Finding threshold for {target_area} pixels ===\n")
    
    # Find optimal threshold
    threshold, binary, pixel_count = find_threshold_by_area(gray, target_area)
    
    print(f"\n=== Results ===")
    print(f"Threshold: {threshold}")
    print(f"Pixel Count: {pixel_count}")
    print(f"Target: {target_area}")
    print(f"Error: {abs(pixel_count - target_area)} pixels")
    
    # Visualization with Matplotlib
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    
    # Original image (convert BGR to RGB for matplotlib)
    axes[0].imshow(cv2.cvtColor(original, cv2.COLOR_BGR2RGB))
    axes[0].set_title('Original Image')
    axes[0].axis('off')
    
    # Binary result
    axes[1].imshow(binary, cmap='gray')
    axes[1].set_title(f'Binary Result (T={threshold}, Pixels={pixel_count})')
    axes[1].axis('off')
    
    plt.tight_layout()
    plt.savefig('threshold_result.png', dpi=150)
    plt.show()
    
    print(f"\nResult saved to 'threshold_result.png'")


if __name__ == '__main__':
    main()
