# -*- coding:utf-8 -*-
import os
import time
import logging
import numpy as np
from hparams import hparams as hps

class ProgramSet(object):

    def __init__(self, dir_path):
    
        #logging.info(" set %s" %(dir_path))
        self.prog_list = self._dirtolist(dir_path)
        
        
    def get_now_prog(self, timezone):
        localtime = time.localtime(time.time() + timezone * 3600)
        week = localtime.tm_wday
        sec = localtime.tm_hour * 3600 + localtime.tm_min * 60 + localtime.tm_sec
        arr = np.array([int(self.prog_list[week][i][6]) for i in range(len(self.prog_list[week]))])
        lists = np.argwhere(arr <= sec).reshape(-1)
        list_id = lists[-1]         
        return week, list_id            

    def _dirtolist(self, dir_path):
    
        list = []
        for i in hps.program_list:        
            prgpath = os.path.join(dir_path, i)
            list.append(self._filetolist(prgpath))
        return list
        
    def _filetolist(self, file_path):
        
        list = []
        with open(file_path, encoding = 'utf-8') as f:
        #with open(file_path) as f:
            for line in f:
                items = line.split()
                if not items: break
                list.append([i for i in items] + [self._strtosec(items[1])])
        return list
        
    def _strtosec(self, str):
        items = str.strip().split("-", 1)
        times = items[0].split(":",1)
        start = int(times[0])*3600 + int(times[1])*60
        #times = items[1].split(":",1)        
        #end = 24*3600 if int(times[0]) == 0 and int(times[1]) == 0 else int(times[0])*3600 + int(times[1])*60
        return start
         