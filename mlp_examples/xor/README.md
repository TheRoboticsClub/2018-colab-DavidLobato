# MLP XOR
This is a very simple MLP that can be trained to solve the non-linear function XOR.


## Requirements
* Python 2.7, Numpy, Scipy
* [Theano](http://deeplearning.net/software/theano/install.html)
* [Lasagne](http://lasagne.readthedocs.org/en/latest/user/installation.html)


## Train + Run

Run `python xor_mlp.py` to train and test the network 
 
```
$> python xor_mlp.py 
test on D before training:
Loss: 0.703020
Error: 50.000000%
Output:
[[0.]
 [0.]
 [0.]
 [0.]]
test on D after training:
Loss: 0.000000
Error: 0.000000%
Output:
[[0.]
 [1.]
 [1.]
 [0.]]
```