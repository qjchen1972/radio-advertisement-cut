# -*- coding:utf-8 -*-
from __future__ import print_function
import numpy as np
import logging
import torch
import torchvision.transforms as transforms
import torch.optim as optim
from torch.utils.data import DataLoader
import time
from net.models import AudioNet,LabelSmoothingLoss
from dataset.dataset import MelDataset
from .optim_schedule import ScheduledOptim
from utils.statistics import Statistics
from hparams import hparams as hps



logging.basicConfig(level=logging.INFO, 
                    format='%(message)s',
                    filename=hps.log_file)

class TrainProc(object):

    def __init__(self):
        
        cuda_condition = torch.cuda.is_available() and hps.with_cuda
        self.device = torch.device("cuda:0" if cuda_condition else "cpu")
        
        #proc model
        self.model = AudioNet(hps.class_num).to(self.device)
        self.optimizer = optim.Adam(self.model.parameters(), lr=hps.lr, betas=hps.betas, weight_decay=hps.weight_decay)
        #self.optimizer = optim.SGD(model.parameters(), lr=hps.lr,momentum=0.9)
        
        #self.criterion = nn.NLLLoss(ignore_index=0)        
        #self.criterion = LabelSmoothingLoss(hps.num_class, label_smoothing = 0.1)
        #self.criterion = torch.nn.CrossEntropyLoss()
            
        self.criterion = torch.nn.BCELoss()
        
        self.n_step = 0
        self.epoch = 0
        
		if hps.ckpt_pth != '':
		    self.load_model()

        # Distributed GPU training if CUDA can detect more than 1 GPU
        if hps.with_cuda and torch.cuda.device_count() > 1:
            print("Using %d GPUS for BERT" % torch.cuda.device_count())
            self.model = nn.DataParallel(self.model, device_ids=cuda_devices)

        self.optim_schedule = ScheduledOptim(self.optimizer, hps.lr, hps.warmup_steps, self.n_step)

        #proc dataset
        kwargs = {'num_workers': hps.num_workers, 'pin_memory': True} if cuda_condition else {}
        if hps.train_label:
            #transform_sequence = self.set_train_transform()
            train_dataset = MelDataset(hps.wav_path, hps.train_label)
            self.train_dataloader = DataLoader(dataset=train_dataset, batch_size=hps.batchSize, shuffle=True, **kwargs)
        else:
            self.train_dataloader = None

        if hps.test_label:
            #transform_sequence = self.set_test_transform()
            test_dataset = MelDataset(hps.wav_path, hps.test_label, is_train = False)
            self.test_dataloader = DataLoader(dataset=test_dataset, batch_size=hps.batchSize, shuffle=False, **kwargs)
        else:
            self.test_dataloader = None

        if hps.val_label:
            #transform_sequence = self.set_test_transform()
            val_dataset = MelDataset(hps.wav_path, hps.val_label, is_train = False)
            self.val_dataloader = DataLoader(dataset=val_dataset, batch_size=hps.batchSize, shuffle=False, **kwargs)
        else:
            self.val_dataloader = None

        #print("the Model: ",self.model)
        print("Total Parameters: ", sum([p.nelement() for p in self.model.parameters()]))


    def load_model(self):
        modelCheckpoint = torch.load(hps.model_path + hps.ckpt_pth)
        self.model.load_state_dict(modelCheckpoint['state_dict'])
        self.optimizer.load_state_dict(modelCheckpoint['optimizer'])
        self.n_step = modelCheckpoint['iterator']
        self.epoch = modelCheckpoint['epoch'] + 1



    def save_model(self):

        output_path = hps.model_path + "model_ep%d.pth" % self.epoch
        torch.save({'epoch': self.epoch, 
                    'state_dict': self.model.state_dict(), 
                    'optimizer': self.optimizer.state_dict(),
                    'iterator': self.optim_schedule.n_current_steps},
                    output_path)
        return output_path
    

    def set_train_transform(self):
        transform_list = []
        transform_list.append(transforms.RandomHorizontalFlip())
        transform_list.append(transforms.ToTensor())
        transform_list.append(transforms.Randomblack(mean=[0.0]))
        transform_list.append(transforms.RandomErasing(mean=[0.0]))
        transform_list.append(transforms.Normalize(mean=(0.5, 0.5, 0.5), std=(0.5, 0.5, 0.5))) 
        transform_sequence = transforms.Compose(transform_list)
        return transform_sequence


    def set_test_transform(self):
        transform_list = []
        transform_list.append(transforms.ToTensor())
        transform_list.append(transforms.Normalize(mean=(0.5, 0.5, 0.5), std=(0.5, 0.5, 0.5))) 
        transform_sequence = transforms.Compose(transform_list)
        return transform_sequence


    def train(self):

        if not self.train_dataloader:
            print("train data is not seted!")
            return

        for epoch in range(self.epoch, hps.max_epoch):
            oldtime = time.time()                        
            train_loss = self.iterator_train()
            
            val_loss = 0.0
            if not self.val_dataloader is None:
                val_loss, _, _ = self.iterator_infer(self.val_dataloader)
                
            print("EP:%d trainloss:%6f  valloss:%6f lr:%6f Model Saved on: %s" % (self.epoch, 
                                                                                  train_loss,
                                                                                  val_loss, 
                                                                                  self.optim_schedule.lr, 
                                                                                  self.save_model()))

            logging.info("Epoch %d : training_loss = %.6f  val_loss = %.6f time = %d" % (epoch, 
                                                                                         train_loss, 
                                                                                         val_loss, 
                                                                                         time.time() - oldtime))
            self.epoch += 1
            
    def test(self, imgpath):
        
        if not self.test_dataloader:
            print("test data is not seted!")
            return
        
        test_loss, label, pred = self.iterator_infer(self.test_dataloader)
        print(test_loss, label.size, (label == 1).sum())
        stat = Statistics()
        stat.draw_auroc(label, pred, imgpath)
        threshold = 0.5 #stat.get_min_threshold(label, pred)
        stat.show_confusion_matrix(label, pred, threshold)

    def iterator_train(self):

        self.model.train()
        loss_train = 0
        temploss = 0
        for batch, data in enumerate(self.train_dataloader):
            data = {key: value.to(self.device) for key, value in data.items()}
            var_output =  self.model(data['input'])
            loss = self.criterion(var_output, data['label'])
            self.optim_schedule.zero_grad()
            loss.backward()
            self.optim_schedule.step_and_update_lr()
            loss_train += loss.item()
            temploss = loss_train/(batch + 1)
            #if batch % hps.log_freq == 0 :
                #print("%d: whole loss = %.6f" % (batch, temploss))
        return temploss


    def iterator_infer(self, dataloader):

        self.model.eval()
        loss_train = 0
        
        out_label = torch.Tensor().to(self.device)
        out_pred = torch.Tensor().to(self.device)
        
        with torch.no_grad():
            for batch, data in enumerate(dataloader):
                data = {key: value.to(self.device) for key, value in data.items()}
                var_output =  self.model(data['input'])
                loss = self.criterion(var_output, data['label'])
                loss_train += loss.item()
                temploss = loss_train/(batch + 1)
                out_label = torch.cat((out_label, data['label']), 0)
                out_pred = torch.cat((out_pred, var_output), 0)
                
        return loss_train/(batch + 1),out_label.cpu().numpy(),out_pred.cpu().numpy()

   