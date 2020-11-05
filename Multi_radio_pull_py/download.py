# -*- coding:utf-8 -*-  
import time
import select
import os
import subprocess
import numpy as np
import logging
import copy
from program_set import ProgramSet
from hparams import hparams as hps

logging.basicConfig(level=logging.INFO, 
                    format='%(message)s',
                    filename=hps.log_file)                  
                    
                    
class Download(object):


    def __init__(self, radio_url, radio_path, timezone=0):
        
        #logging.info(" %s, %s, %d" %(radio_url, radio_path, timezone))
        self.timezone = timezone
        self.proglist = ProgramSet(os.path.join(radio_path, hps.program_path))
        self.download_path = os.path.join(radio_path, hps.mp3_path)
        self.radio_url = radio_url
        hps.dw_cmd.append(radio_url)
        hps.dw_cmd.append('-y')
        
    def run(self):
    
        week, id = self.proglist.get_now_prog(self.timezone)
        print(week, id)
        today, today_dir = self.__mkdir()
        file_path, proc = self.__download_one(week, id, today_dir)       
        print(self.proglist.prog_list[week][id])
        while True:
            _, _, _ = select.select([], [], [], 1)
            new_week,new_id = self.proglist.get_now_prog(self.timezone)
            #print('while', new_week,new_id)
            if week != new_week:
               proc.terminate()
               self.__insert_db(self.proglist.prog_list[week][id],file_path, today)
               today, today_dir = self.__mkdir()
               week, id = new_week,new_id
               file_path, proc = self.__download_one(week, id, today_dir)
            elif new_id != id:
               proc.terminate()
               self.__insert_db(self.proglist.prog_list[week][id],file_path,today)
               week, id = new_week,new_id
               file_path, proc = self.__download_one(week, id, today_dir)
            else:
               #out = proc.stderr.readlines()
               pass
               
                
    
    def __mkdir(self):

        today = self.__get_today_dir()
        today_dir = os.path.join(self.download_path, today)
        md_cmd = copy.copy(hps.md_cmd)
        md_cmd.append(today_dir)
        self.__exec(md_cmd, True)
        return today, today_dir     

    def __download_one(self, week, id, today_dir):
       
        file_path = os.path.join(today_dir, self.proglist.prog_list[week][id][1] + '.mp3')
        print('down 0',  file_path)      
        dw_cmd = copy.copy(hps.dw_cmd)
        dw_cmd.append(file_path)
        proc = self.__exec(dw_cmd)
        return file_path, proc
                

    def __get_today_dir(self):
        
        localtime = time.localtime(time.time() + self.timezone * 3600)
        return  "%d-%02d-%02d" %(localtime.tm_year, localtime.tm_mon, localtime.tm_mday)
        
    def __exec(self, cmd, wait=False):
    
        #proc = subprocess.Popen(cmd, shell=True, close_fds=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        #cmd = cmd + ['|',
        print(cmd)
        #proc = subprocess.Popen(cmd, close_fds=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)  
        proc = subprocess.Popen(cmd) #, stderr=subprocess.DEVNULL)        
        if wait is True:
            proc.wait()
        return proc
        
        
    def __insert_db(self, info, file_path, today_dir):
        
        sql = "INSERT INTO prolist(proName,proDate,proTime,proHost,proRadioName,proFileName,proFilePath) \
               VALUES( \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")" %(info[0], today_dir,
               info[1], info[5], self.radio_url, info[1]+".mp3", file_path)               
               
        db_cmd = copy.copy(hps.db_cmd)
        db_cmd.append("set names utf8;" + sql)
        self.__exec(db_cmd)
        
        
        
