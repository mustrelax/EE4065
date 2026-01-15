import torch
from torch.utils.data import Dataset
import torchvision.datasets as datasets
import numpy as np
import cv2
import random

RESOLUTION = 128
CELLS = 8
CATEGORIES = 10

class SyntheticDigitDataset(Dataset):
    def __init__(self, train_mode=True, root_folder="data"):
        self.source = datasets.MNIST(root=root_folder, train=train_mode, download=True, transform=None)
        self.pool = list(range(len(self.source)))
        self.res = RESOLUTION
        self.cells = CELLS
        self.cats = CATEGORIES
        
    def __len__(self):
        return len(self.source)
    
    def _make_background(self):
        shade = random.randint(220, 255)
        bg = np.full((self.res, self.res), shade, dtype=np.float32)
        jitter = np.random.normal(0, 10, (self.res, self.res))
        bg = np.clip(bg + jitter, 0, 255)
        return bg
    
    def _augment_digit(self, raw_img):
        if random.random() > 0.5:
            kern = np.ones((2, 2), np.uint8)
            raw_img = cv2.erode(raw_img, kern, iterations=1)
        new_h = random.randint(24, 48)
        new_w = random.randint(24, 48)
        return cv2.resize(raw_img, (new_w, new_h)), new_w, new_h
    
    def _blend_onto_canvas(self, canvas, digit_img, px, py, dw, dh):
        shade = random.randint(0, 60)
        mask = digit_img.astype(np.float32) / 255.0
        region = canvas[py:py+dh, px:px+dw]
        blended = (shade * mask) + (region * (1.0 - mask))
        canvas[py:py+dh, px:px+dw] = blended
        
    def _compute_target(self, px, py, dw, dh, cat):
        center_x = (px + dw / 2) / self.res
        center_y = (py + dh / 2) / self.res
        norm_w = dw / self.res
        norm_h = dh / self.res
        
        cell_col = int(center_x * self.cells)
        cell_row = int(center_y * self.cells)
        
        if cell_col >= self.cells or cell_row >= self.cells:
            return None
            
        offset_x = center_x * self.cells - cell_col
        offset_y = center_y * self.cells - cell_row
        
        return cell_row, cell_col, offset_x, offset_y, norm_w, norm_h, cat
    
    def __getitem__(self, idx):
        count = random.randint(3, 6)
        picks = [idx] + random.sample(self.pool, count - 1)
        
        frame = self._make_background()
        targets = torch.zeros((self.cells, self.cells, 5 + self.cats))
        used = np.zeros((self.res, self.res), dtype=np.uint8)
        
        for pick in picks:
            digit_pil, cat = self.source[pick]
            digit_np = np.array(digit_pil)
            aug_digit, dw, dh = self._augment_digit(digit_np)
            
            for _ in range(10):
                px = random.randint(0, self.res - dw)
                py = random.randint(0, self.res - dh)
                
                overlap = np.sum(used[py:py+dh, px:px+dw])
                if overlap < (dw * dh * 0.2):
                    self._blend_onto_canvas(frame, aug_digit, px, py, dw, dh)
                    used[py:py+dh, px:px+dw] = 1
                    
                    info = self._compute_target(px, py, dw, dh, cat)
                    if info and targets[info[0], info[1], 0] == 0:
                        r, c, ox, oy, nw, nh, cat = info
                        targets[r, c, 0] = 1.0
                        targets[r, c, 1] = ox
                        targets[r, c, 2] = oy
                        targets[r, c, 3] = nw
                        targets[r, c, 4] = nh
                        targets[r, c, 5 + cat] = 1.0
                    break
                    
        tensor = torch.tensor(frame / 255.0, dtype=torch.float32).unsqueeze(0)
        return tensor, targets
