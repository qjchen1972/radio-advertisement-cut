#include "../utils/caffe2_net.h"
#include "../utils/utils.h"
#include "mel.h"

static caffe2net::Net predictor;

int  initMel(const char* init_net, const char*  predict_net) {
	if (!predictor.initNet(init_net, predict_net)) {
		std::cout << "init net error" << std::endl;
		return 0;
	}

	std::vector<int64_t> dim;
	predictor.initInput("input", &dim);
	//std::cout << "init net error" << std::endl;mel
	//printf("init net error\n");
	printf("input dim is %ld %lld  %lld %lld \n", dim[0],dim[1],dim[2],dim[3]);
	return 1;
}


bool  notAdv(std::vector<float> &mel) {	

	caffe2net::Mat cm;
	cm.data = mel;
	predictor.setInput("input", cm);
	if (!predictor.run()) {
		std::cout << "predict error" << std::endl;
		return 0;
	}
	std::vector<float> vec;
	predictor.setOutput("output", vec);
	//if(vec[0] > 0.5)
		//printf(" size is  %d, ans is %f \n", vec.size(),vec[0]);
	return vec[0] > 0.5;
}