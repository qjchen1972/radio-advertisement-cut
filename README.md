# radio advertisement cut

## Introduction
Cut radio stations from the network to remove advertising, traffic information and the hourly news. Only the actual program content is retained.

### Difficulties:
1. Generally, one hour of radio content, only about 30 minutes of normal programming, advertising and programming content is not regular
2. There are nearly 30 programs with different contents and hosts in a week
3. There is no open solution of this type at present

### Solution
1. Cut audio into 4-second blocks and convert them into Mel spectrum (pcen spectrum can also be used to express features)
2. Use the spectrum to judge whether it is a normal program, and then cut it

### Key points of the scheme
1. After many tests, we use the spectrum chart composed of spectrum details and envelope to identify and train. Usually only envelope is used.
2. A 401 * 80 square matrix is obtained by using 80 Mel filter banks and 4 seconds of sound. Finally, the 160 * 160 square matrix is used as the training and recognition input
3. Using a castrated densenet network (430000 parameters), the deployed caffe2 network is about 2.5m
4. Data enhancement method similar to the graph is adopted
   * The volume of the sound will be increased or decreased at random
   * A random number is added or subtracted as a whole
   * The generated Mel spectrum is randomly cut into a 160 * 160 square matrix   
5. In order to deploy on normal devices, a corresponding simple library is rewritten according to librosa of Python


# test
ok
ok
哈哈哈
