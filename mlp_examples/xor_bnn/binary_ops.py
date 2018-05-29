# from https://github.com/MatthieuCourbariaux/BinaryNet/blob/master/Run-time/binary_ops.py
import numpy as np
import theano
import theano.tensor as T


def sign_numpy(x):
    return np.float32(2.*np.greater_equal(x,0)-1.)


def sign_theano(x):
    return T.cast(2.*T.ge(x,0)-1., theano.config.floatX)