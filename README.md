# radio advertisement cut

## Introduction
Cut radio stations from the network to remove advertising, traffic information and the hourly news. Only the actual program content is retained.

#### Difficulties:
1. Generally, one hour of radio content, only about 30 minutes of normal programming, advertising and programming content is not regular
2. There are nearly 30 programs with different contents and hosts in a week
3. There is no open solution of this type at present

#### Solution
1. Cut audio into 4-second blocks and convert them into Mel spectrum (pcen spectrum can also be used to express features)
2. Use the spectrum to judge whether it is a normal program, and then cut it

#### Key points of the scheme
1. After many tests, we use the spectrum chart composed of spectrum details and envelope to identify and train. Usually only envelope is used.
2. A 401 * 80 square matrix is obtained by using 80 Mel filter banks and 4 seconds of sound. Finally, the 160 * 160 square matrix is used as the training and recognition input
3. Using a castrated densenet network (430000 parameters), the deployed caffe2 network is about 2.5m
4. Data enhancement method similar to the graph is adopted
   * The volume of the sound will be increased or decreased at random
   * A random number is added or subtracted as a whole
   * The generated Mel spectrum is randomly cut into a 160 * 160 square matrix   
5. In order to deploy on normal devices, a corresponding simple library is rewritten according to librosa of Python

#### Conclusion

1. Confusion matrix of test set

| | Abnormal program (actual result) | normal program (actual result)
|:-------|:-----:|:-------:|
| abnormal (prediction results) | 1607 | 3 |
| Normal program (prediction result) | 1 | 2287  |

  Accuracy = 99.89%

2. In the actual test, the effect is very good. I have specific test results, charts and data here. Call me when you need it

#### Some references

  * [http://blog.csdn.net/zouxy09/article/details/9156785](http://blog.csdn.net/zouxy09/article/details/9156785)
  * [https://blog.csdn.net/zzc15806/article/details/79246716](https://blog.csdn.net/zzc15806/article/details/79246716)
  * [https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html](https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html)
  
  
## Content

#### [createlabel](https://github.com/qjchen1972/radio-advertisement-cut/tree/main/createlabel)

* A part of normal programs are marked manually. The program is divided into normal program wav and Abnormal program wav. 
* Then, the wavs is divided into 4 seconds into a wav (2 seconds interval is used for segmentation) 
* and then it is converted into Mel spectrum  80 * 480. In order to facilitate training, it is converted to 160 * 240 JPEG

#### [train net](https://github.com/qjchen1972/radio-advertisement-cut/tree/main/train%20net)
* Using Pytorch as a training framework
* Randomly cut 160 * 160 as training input

#### [deladv(PytorchV1.3)](https://github.com/qjchen1972/radio-advertisement-cut/tree/main/deladv(PytorchV1.3)) and [deladv(PytorchV1.0)](https://github.com/qjchen1972/radio-advertisement-cut/tree/main/deladv(PytorchV1.0))


