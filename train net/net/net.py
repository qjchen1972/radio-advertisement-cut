# -*- coding: utf-8 -*-
from __future__ import division
import torch
from torch import nn
import torch.nn.functional as F
from math import floor
from collections import OrderedDict
from math import sqrt

class _DenseLayer(nn.Sequential):
    def __init__(self, num_input_features, growth_rate, bn_size, drop_rate):
        super(_DenseLayer, self).__init__()
        self.add_module('norm1', nn.BatchNorm2d(num_input_features,affine=True)),
        self.add_module('relu1', nn.ReLU(inplace=True)),
        self.add_module('conv1', nn.Conv2d(num_input_features, bn_size *
                        growth_rate, kernel_size=1, stride=1, bias=False)),
        self.add_module('norm2', nn.BatchNorm2d(bn_size * growth_rate,affine=True)),
        self.add_module('relu2', nn.ReLU(inplace=True)),
        self.add_module('conv2', nn.Conv2d(bn_size * growth_rate, growth_rate,
                        kernel_size=3, stride=1, padding=1, bias=False)),
        self.drop_rate = drop_rate

    def forward(self, x):
        new_features = super(_DenseLayer, self).forward(x)
        if self.drop_rate > 0:
            new_features = F.dropout(new_features, p=self.drop_rate, training=self.training)
        return torch.cat([x, new_features], 1)


class _DenseBlock(nn.Sequential):
    def __init__(self, num_layers, num_input_features, bn_size, growth_rate, drop_rate):
        super(_DenseBlock, self).__init__()
        for i in range(num_layers):
            layer = _DenseLayer(num_input_features + i * growth_rate, growth_rate, bn_size, drop_rate)
            self.add_module('denselayer%d' % (i + 1), layer)


class _Transition(nn.Sequential):
    def __init__(self, num_input_features, num_output_features):
        super(_Transition, self).__init__()
        self.add_module('norm', nn.BatchNorm2d(num_input_features))
        self.add_module('relu', nn.ReLU(inplace=True))
        self.add_module('conv', nn.Conv2d(num_input_features, num_output_features,
                                          kernel_size=1, stride=1, bias=False))
        self.add_module('pool', nn.AvgPool2d(kernel_size=2, stride=2))


class DenseNet(nn.Module):
    def __init__(self, growth_rate=32, block_config=(6,12, 24, 16),
                 num_init_features=64, bn_size=4, drop_rate=0, num_classes=1000):
        super(DenseNet, self).__init__()

        # First convolution
        self.features = nn.Sequential(OrderedDict([('conv0', nn.Conv2d(1, num_init_features, kernel_size=7, stride=2,padding=3, bias=False)),
                                                  ('norm0', nn.BatchNorm2d(num_init_features)),
                                                  ('relu0', nn.ReLU(inplace=True)),
                                                  ('pool0', nn.MaxPool2d(kernel_size=3, stride=2, padding=1)),
                                                  ]))

        # Each denseblock
        num_features = num_init_features
        for i, num_layers in enumerate(block_config):
            if num_layers == 0:continue
            block = _DenseBlock(num_layers=num_layers, num_input_features=num_features,
                                bn_size=bn_size, growth_rate=growth_rate, drop_rate=drop_rate)
            self.features.add_module('denseblock%d' % (i + 1), block)
            num_features = num_features + num_layers * growth_rate
            if i != len(block_config) - 1:
                trans = _Transition(num_input_features=num_features, num_output_features=num_features // 2)
                self.features.add_module('transition%d' % (i + 1), trans)
                num_features = num_features // 2

        # Final batch norm
        self.features.add_module('norm5', nn.BatchNorm2d(num_features))
        self.features.add_module('relu5', nn.ReLU(inplace=True))
        self.classifier = nn.Linear(num_features, num_classes)
        
        # init.
        for m in self.modules():
            if isinstance(m, nn.Conv2d):
                #nn.init.kaiming_normal_(m.weight)
                n = m.kernel_size[0] * m.kernel_size[1] * m.in_channels
                m.weight.data.normal_(0, sqrt(2. / n))
                if m.bias is not None: nn.init.constant_(m.bias, 0)
            elif isinstance(m, nn.BatchNorm2d):
                nn.init.constant_(m.weight, 1)
                nn.init.constant_(m.bias, 0)
            elif isinstance(m, nn.Linear):
                n = m.in_features
                m.weight.data.normal_(0, sqrt(2. / n))
                if m.bias is not None: nn.init.constant_(m.bias, 0)
        
    def forward(self, x):
        features = self.features(x)
        matrix_size = tuple(int(i) for i in features.size()[2:])
        out = F.avg_pool2d(features, kernel_size=matrix_size, stride=1).view(features.size(0), -1)
        out = self.classifier(out)
        return out


def create_net(classes,**kwargs):
    r"""
    121: 32, (6, 12, 24, 16), 64
    161: 48, (6, 12, 36, 24), 96
    169: 32, (6, 12, 32, 32), 64
    201: 32, (6, 12, 48, 32), 64
    now: 32, (6,8,10,12),16
    """
    
    return DenseNet(num_init_features=32, growth_rate=16,block_config=(6,8,10,12), bn_size=2, num_classes=classes,**kwargs)
    
    
