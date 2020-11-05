#!/usr/bin/env python
# -*- coding:utf-8 -*-
import argparse
from net.models import AudioNet,LabelSmoothingLoss
from trainer.train_proc import TrainProc
from trainer.onnx_proc import OnnxProc
from utils.statistics import Statistics
from hparams import hparams as hps

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument('-m', type=int, default=0, metavar='mode', help='input run mode (default: 0)')
    args = parser.parse_args()

    if args.m == 0:
        run_train()
    elif args.m == 1:
        run_test()
    elif args.m == 2:
        run_onnx()
    elif args.m == 3:        
        draw_trainimage()
    elif args.m == 4:
        run_jit()
    else: 
        pass

def run_train():
        
        trainer = TrainProc()
        trainer.train()

def run_test():

    imgpath = 'test.png'
    trainer = TrainProc()
    trainer.test(imgpath)        

def draw_trainimage():
    
    stat = Statistics()
    train,val = stat.files_to_list(hps.log_file)
    stat.draw_trainimg(train,val,"train.png")
    
def run_onnx():
    
    model_path = hps.model_path + hps.ckpt_pth
    onnx_model = 'onnx_proto'
    out_init = 'init.pb'
    out_pred = 'pred.pb'
    
    onnx = OnnxProc(model_path, hps.chn, hps.row, hps.col)
    onnx.create_onnx_model(onnx_model)
    onnx.create_caffe2_model(onnx_model, out_init, out_pred)

def run_jit():
    model_path = hps.model_path + hps.ckpt_pth
	out_path = 'model.pt'
	example_input = torch.rand(1, hps.chn, hps.row, hps.col)
    device = torch.device("cpu")
    model = AudioNet(hps.class_num).to(device)
	script_module = torch.jit.trace(model, example_input)
	script_module.save(out_path)

if __name__ == '__main__':
    main()
