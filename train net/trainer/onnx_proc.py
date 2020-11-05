import torch
import torch.onnx
import io
import onnx
import caffe2.python.onnx.backend as backend
import numpy as np
from caffe2.python.predictor import mobile_exporter
from hparams import hparams as hps
from net.models import AudioNet

class OnnxProc(object):    

    def __init__(self, model_path, chn, height, width, is_cpu=True):        

        self.device = torch.device("cpu" if is_cpu else "cuda:0")
        self.model = AudioNet(hps.class_num).to(self.device)

        if is_cpu:
            modelCheckpoint = torch.load(model_path,map_location='cpu')
        else:
            modelCheckpoint = torch.load(model_path)
        self.model.load_state_dict(modelCheckpoint['state_dict'])

        self.chn = chn
        self.height = height
        self.width = width

    def create_onnx_model(self, model_path, input_names=["input"], output_names=["output"]):

        dummy_input = torch.randn(1, self.chn, self.height,self.width).to(self.device)
        torch.onnx.export(self.model,  
                          dummy_input,
                          model_path, 
                          verbose=True,
                          input_names=input_names, 
                          output_names=output_names)
        
        
    def create_caffe2_model(self, onnx_model_path, out_init_path, out_pred_path):

        model = onnx.load(onnx_model_path)
        # Check that the IR is well formed
        onnx.checker.check_model(model)
        # Print a human readable representation of the graph
        #print(onnx.helper.printable_graph(model.graph))
        #rep = backend.prepare(model, device="CUDA:0") # or "CPU"
        rep = backend.prepare(model, device="CPU")
        # For the Caffe2 backend:
        # rep.predict_net is the Caffe2 protobuf for the network
        # rep.workspace is the Caffe2 workspace for the network
        #(see the class caffe2.python.onnx.backend.Workspace)
        outputs = rep.run(np.random.randn(1, 
                                          self.chn, 
                                          self.height, 
                                          self.width).astype(np.float32))

        print(outputs[0])

        c2_workspace = rep.workspace
        c2_graph = rep.predict_net
        init_net, predict_net = mobile_exporter.Export(c2_workspace, c2_graph, c2_graph.external_input)

        with io.open(out_init_path, 'wb') as f:
            f.write(init_net.SerializeToString())

        with open(out_pred_path, 'wb') as f:
            f.write(predict_net.SerializeToString())
        
