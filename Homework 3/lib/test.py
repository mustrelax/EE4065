# just to compare with the results from mcu

import cv2
import numpy as np
import matplotlib.pyplot as plt

image = cv2.imread('monk.png')
image = cv2.resize(image, (128, 128))
gray_image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

ret, otsu_thresh = cv2.threshold(gray_image, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

kernel = np.ones((3, 3), np.uint8)

dilated = cv2.dilate(otsu_thresh, kernel, iterations=1)
eroded = cv2.erode(otsu_thresh, kernel, iterations=1)
opened = cv2.morphologyEx(otsu_thresh, cv2.MORPH_OPEN, kernel)
closed = cv2.morphologyEx(otsu_thresh, cv2.MORPH_CLOSE, kernel)

fig, axes = plt.subplots(2, 3, figsize=(12, 8))

axes[0, 0].imshow(gray_image, cmap='gray')
axes[0, 0].set_title('Original Gray')
axes[0, 0].axis('off')

axes[0, 1].imshow(otsu_thresh, cmap='gray')
axes[0, 1].set_title(f'Otsu (T={ret:.0f})')
axes[0, 1].axis('off')

axes[0, 2].imshow(dilated, cmap='gray')
axes[0, 2].set_title('Dilation')
axes[0, 2].axis('off')

axes[1, 0].imshow(eroded, cmap='gray')
axes[1, 0].set_title('Erosion')
axes[1, 0].axis('off')

axes[1, 1].imshow(opened, cmap='gray')
axes[1, 1].set_title('Opening (Erosion→Dilation)')
axes[1, 1].axis('off')

axes[1, 2].imshow(closed, cmap='gray')
axes[1, 2].set_title('Closing (Dilation→Erosion)')
axes[1, 2].axis('off')

plt.tight_layout()
plt.show()