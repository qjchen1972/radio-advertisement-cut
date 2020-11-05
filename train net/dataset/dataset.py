import os
import torch
import random
import numpy as np
from hparams import hparams as hps
from torch.utils.data import Dataset
from utils.audio import Audio

random.seed(0)

class MelDataset(Dataset):

    def __init__(self, wav_path, label_file, is_train=True):
        self.f_list = self.files_to_list(wav_path, label_file)
        random.shuffle(self.f_list)
        self.size = hps.chn * hps.row * hps.col / hps.num_mels
        self.is_train = is_train
        self.audio = Audio(hps.sample_rate)

    def files_to_list(self, wav_path, label_file):
        f_list = []
        with open(label_file, encoding = 'utf-8') as f:
            for line in f:
                items = line.split()
                if not items: break
                filepath = os.path.join(wav_path, items[0])
                f_list.append([filepath, int(items[1])])
        return f_list

    def get_mel(self, filename):
        wav = self.audio.load_wav(filename)
        mel = self.audio.melspectrogram(wav).astype(np.float32)
        mel = np.transpose(mel) 
        if self.is_train:
            pos = random.randint(0, mel.shape[0] - self.size)
        else:
            pos = 0
        return torch.Tensor(mel[int(pos) : int(pos + self.size), :].reshape(hps.chn,hps.row,hps.col))


    def __getitem__(self, index):
        return {"input": self.get_mel(self.f_list[index][0]), "label": torch.Tensor([self.f_list[index][1]])}

    def __len__(self):
        return len(self.f_list)


