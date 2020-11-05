import numpy as np


class ScheduledOptim(object):
    
    def __init__(self, optimizer, init_lr, n_warmup_steps,n_current_steps=0):
        self._optimizer = optimizer
        self.n_warmup_steps = n_warmup_steps
        self.n_current_steps = n_current_steps
        self.init_lr = init_lr

    def step_and_update_lr(self):
        "Step with the inner optimizer" 
        self._update_learning_rate()
        self._optimizer.step()

    def zero_grad(self):
        "Zero out the gradients by the inner optimizer"
        self._optimizer.zero_grad()

    def _get_lr_scale(self):
        r"""        
        noem scheme
        """
        
        return np.power(self.n_warmup_steps, 0.5) * np.min([np.power(self.n_current_steps, -0.5), np.power(self.n_warmup_steps, -1.5) * self.n_current_steps])

    def _update_learning_rate(self):
        
        self.n_current_steps += 1
        #print(self.init_lr, self._get_lr_scale())
        self.lr = self.init_lr * self._get_lr_scale()
        for param_group in self._optimizer.param_groups:
            param_group['lr'] = self.lr
