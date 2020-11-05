# -*- coding: utf-8 -*-
from __future__ import division
import numpy as np
import torch.nn as nn
from .net import create_net


class AudioNet(nn.Module):

    def __init__(self, classCount):
    
        super(AudioNet, self).__init__()
        self.model = create_net(classCount)
        kernelCount = self.model.classifier.in_features
        self.model.classifier = nn.Sequential(nn.Linear(kernelCount, classCount), nn.Sigmoid())
        
    
    def forward(self, x):
        out = self.model(x)
        return out


class LabelSmoothingLoss(nn.Module):
    r'''
    Label Smoothing Loss function
    '''

    def __init__(self, classes_num, label_smoothing=0.0, dim=-1):
    
        super(LabelSmoothingLoss, self).__init__()
        self.confidence = 1.0 - label_smoothing
        self.label_smoothing = label_smoothing
        self.classes_num = classes_num
        self.dim = dim
        self.criterion = nn.KLDivLoss(reduction='batchmean')

    def forward(self, pred, target):
    
        pred = pred.log_softmax(dim=self.dim)
        smooth_label = torch.empty(size=pred.size(), device=target.device)
        smooth_label.fill_(self.label_smoothing / (self.classes_num - 1))
        smooth_label.scatter_(1, target.data.unsqueeze(1), self.confidence)
        return self.criterion(pred, Variable(smooth_label, requires_grad=False))
        
        