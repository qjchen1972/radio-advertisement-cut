
class hparams:

    
    ################################
    # Data Parameters              #
    ################################

    wav_path = 'data/wav_data'
    train_label = 'data/train_label.txt'
    val_label = 'data/val_label.txt'
    test_label = 'data/test_label.txt'
    

    ################################
    # Audio                        #
    ################################
    sample_rate = 44100
    num_mels = 80

    ################################
    # Model Parameters             #
    ################################
    class_num = 1
    chn = 1
    row = 160
    col = 160
    model_path = 'models/'
    ckpt_pth = 'model_ep15.pth'

    ################################
    # Train                        #
    ################################

    warmup_steps = 500
    log_file = 'answer.txt'
    with_cuda = True
    lr = 2e-3
    betas = (0.9, 0.999)
    weight_decay = 1e-6
    num_workers = 8
    max_epoch = 1000
    log_freq = 10
    batchSize = 32