# -*- coding:utf-8 -*-
from __future__ import print_function
from __future__ import division
import matplotlib 
#matplotlib.use('Agg')
matplotlib.use('Qt5Agg')
import matplotlib.pyplot as plt
import numpy as np
from sklearn.metrics import roc_auc_score, confusion_matrix,roc_curve, auc,classification_report
import math
import random

class Statistics(object): 
    def __init__(self):
        pass

    def draw_auroc(self, label, predict, img_path):

        fpr, tpr, threshold = roc_curve(label, predict)
        roc_auc = auc(fpr, tpr)
        plt.plot(fpr, tpr, 'b', label='AUC = %0.2f' %(roc_auc))
        plt.legend(loc='lower right')
        plt.plot([0, 1], [0, 1], 'r--')
        plt.xlim([0, 1])
        plt.ylim([0, 1])
        plt.ylabel('True Positive Rate')
        plt.xlabel('False Positive Rate')
        plt.savefig(img_path)
        plt.show()
        

    def draw_trainimg(self,train_loss, val_loss,img_path):
                
        plt.figure(figsize=(8,6), dpi=100)
        plt.plot(range(len(train_loss)), train_loss, label='train', color='red')
        plt.plot(range(len(val_loss)), val_loss, label='val', color='blue')
        plt.xlabel('epoch')
        plt.ylabel('loss')
        plt.savefig(img_path)
        plt.show()
        
        
    def show_confusion_matrix(self, label, predict, threshold):
        a = predict > threshold
        print(confusion_matrix(label.reshape(-1),a.reshape(-1)))
        #print(confusion_matrix(label.reshape(-1), predict.reshape(-1)))
        

    def get_min_threshold(self, label, predict):
        r"""
        the best threshold is minimum distance to upper left corner
        tpr = tp /(tp + fn)
        fpr = fp /(fp + tn)
        """
        fpr, tpr, threshold = roc_curve(label, predict)
        min = math.sqrt((tpr[0] -1.0)*(tpr[0] -1.0) + fpr[0]*fpr[0] )
        hold = 0
        for i in range(1,len(fpr)):
            temp = math.sqrt((tpr[i] -1.0)*(tpr[i] -1.0) + fpr[i]*fpr[i] )
            if temp < min: 
                min = temp
                hold = i
        print('%0.4f    %0.4f     %0.4f' %(tpr[hold],fpr[hold],threshold[hold]))
        return threshold[hold]
        
    def files_to_list(self, file):
        train = []
        val = []
        with open(file, encoding = 'utf-8') as f:
            for line in f:
                items = line.split()
                if not items: break
                train.append(float(items[5]))
                val.append(float(items[8]))
        return np.array(train),np.array(val)
    
if __name__ == '__main__':
    stat = Statistics()
    a = [0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95]
    b = [random.random() for i in range(18)]
    c = [random.randint(0,1) for i in range(18)]
    print(a)
    print(b)

    #a1 = np.array(a)
    #b1 = np.array(b)
    #print(a)
    #print(b)
    
    stat.draw_auroc(c,b,"auroc.png")
    stat.draw_trainimg(a,b,"train.png")
    