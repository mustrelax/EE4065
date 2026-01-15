import torch
import torch.nn as nn

# YOLO-FastestV2 Implementation
# Adapted from: https://github.com/dog-qiuqiu/Yolo-FastestV2

CATEGORIES = 10
SAMPLES_PER_BATCH = 16

def conv_block(in_ch, out_ch, pool=True):
    layers = [
        nn.Conv2d(in_ch, out_ch, 3, 1, 1, bias=False),
        nn.BatchNorm2d(out_ch),
        nn.ReLU(inplace=True)
    ]
    if pool:
        layers.append(nn.MaxPool2d(2, 2))
    return nn.Sequential(*layers)

class YoloFastestV2(nn.Module):
    def __init__(self, num_classes=CATEGORIES):
        super(YoloFastestV2, self).__init__()
        self.backbone = nn.Sequential(
            conv_block(1, 16),
            conv_block(16, 32),
            conv_block(32, 64),
            conv_block(64, 128),
            conv_block(128, 128, pool=False)
        )
        self.predictor = nn.Conv2d(128, 5 + num_classes, 1)
        
    def forward(self, x):
        features = self.backbone(x)
        raw = self.predictor(features)
        activated = torch.sigmoid(raw)
        return activated.permute(0, 2, 3, 1)

class DetectionLoss(nn.Module):
    def __init__(self, coord_weight=7.0, empty_weight=0.5):
        super().__init__()
        self.mse_fn = nn.MSELoss(reduction="sum")
        self.w_coord = coord_weight
        self.w_empty = empty_weight
        
    def forward(self, pred, truth):
        has_obj = truth[..., 0] == 1
        no_obj = truth[..., 0] == 0
        
        coord_err = self.mse_fn(pred[..., 1:5][has_obj], truth[..., 1:5][has_obj])
        
        conf_pos = self.mse_fn(pred[..., 0][has_obj], truth[..., 0][has_obj])
        conf_neg = self.mse_fn(pred[..., 0][no_obj], truth[..., 0][no_obj])
        
        cat_err = self.mse_fn(pred[..., 5:][has_obj], truth[..., 5:][has_obj])
        
        total = self.w_coord * coord_err + conf_pos + self.w_empty * conf_neg + cat_err
        return total / SAMPLES_PER_BATCH
