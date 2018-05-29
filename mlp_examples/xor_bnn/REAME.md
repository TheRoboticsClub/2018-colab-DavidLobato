# Binarized MLP XOR
This is a very simple MLP that can be trained to solve the non-linear function XOR with +1 and -1 weights and activations. 
It uses the techniques described on [BinaryNet: Training Deep Neural Networks with Weights and Activations Constrained to +1 or -1.](http://arxiv.org/abs/1602.02830)

## Requirements
* Python 2.7, Numpy, Scipy
* [Theano](http://deeplearning.net/software/theano/install.html)
* [Lasagne](http://lasagne.readthedocs.org/en/latest/user/installation.html)

## Train

Run `python xor_mlp.py` to produce the network parameters. They will be stored on a file.

```
$> python xor_mlp.py
W_LR_scale = 1.6329932
H = 1.0
W_LR_scale = 1.4142135
H = 1.0
test on D before training:
Loss: 2.750000
Error: 50.000000%
Output:
[[2.]
 [0.]
 [2.]
 [0.]]
test on D after training:
Loss: 0.000000
Error: 0.000000%
Output:
[[-1.0000004 ]
 [ 0.99999994]
 [ 0.99999994]
 [-1.0000004 ]]
('Parameters saved to: ', 'xor_bnn.npz')
```


## Run

Run `python xor_mlp_run.py` to load the trained binary parameters and run the network in feed forward mode.

```
$> python xor_mlp_run.py
Loading the trained parameters and binarizing the weights...
Running...
Error = 0.0%
Output = [[-1.]
 [ 1.]
 [ 1.]
 [-1.]]
```

 
  