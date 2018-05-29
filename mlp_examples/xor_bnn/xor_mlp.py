import numpy

import theano
import theano.tensor as T

import lasagne
import binary_net
import binary_ops

from collections import OrderedDict


def build_xor_mlp(input_var=None, hidden_units=4, binary=True, stochastic=False,
                  W_LR_scale='Glorot', H=1., activation=binary_net.binary_tanh_unit,
                  epsilon=1e-1, alpha=.1):
    mlp = lasagne.layers.InputLayer(shape=(1, 2), input_var=input_var)

    mlp = binary_net.DenseLayer(
        mlp,
        binary=binary,
        stochastic=stochastic,
        H=H,
        W_LR_scale=W_LR_scale,
        nonlinearity=lasagne.nonlinearities.identity,
        num_units=hidden_units
    )

    mlp = lasagne.layers.BatchNormLayer(
        mlp,
        epsilon=epsilon,
        alpha=alpha
    )

    mlp = lasagne.layers.NonlinearityLayer(
        mlp,
        nonlinearity=activation
    )

    mlp = binary_net.DenseLayer(
        mlp,
        binary=binary,
        stochastic=stochastic,
        H=H,
        W_LR_scale=W_LR_scale,
        nonlinearity=lasagne.nonlinearities.identity,
        num_units=1
    )

    mlp = lasagne.layers.BatchNormLayer(
        mlp,
        epsilon=epsilon,
        alpha=alpha
    )

    return mlp


def run_xor_mlp():
    D = (
        numpy.array([[-1, -1], [-1, 1], [1, -1], [1, 1]], dtype=theano.config.floatX),
        numpy.array([[-1], [1], [1], [-1]], dtype=theano.config.floatX) # xor
        #numpy.array([[1], [-1], [-1], [1]], dtype=theano.config.floatX) # xnor
    )

    params_path = 'xor_bnn.npz'
    binary = True
    training_steps = 10000
    LR = 0.001

    #rng = numpy.random.RandomState(1111)
    #lasagne.random.set_rng(rng)

    # Declare Theano symbolic variables
    input_var = T.matrix('inputs')
    target_var = T.matrix('targets')

    # Create neural network model
    mlp = build_xor_mlp(input_var, hidden_units=2, binary=binary)

    prediction = lasagne.layers.get_output(mlp)
    loss = T.mean(T.sqr(T.maximum(0.,1.-target_var*prediction))) #squared hinge loss

    if binary:
        binarizable_params = lasagne.layers.get_all_params(mlp, binary=True)
        grads = binary_net.compute_grads(loss, mlp)
        updates = lasagne.updates.adam(loss_or_grads=grads, params=binarizable_params, learning_rate=LR)
        updates = binary_net.clipping_scaling(updates, mlp)

        # other parameters updates
        params = lasagne.layers.get_all_params(mlp, trainable=True, binary=False)
        updates = OrderedDict(
            updates.items() +
            lasagne.updates.adam(loss_or_grads=loss, params=params, learning_rate=LR).items()
        )
    else:
        params = lasagne.layers.get_all_params(mlp, trainable=True)
        updates = lasagne.updates.adam(loss_or_grads=loss, params=params, learning_rate=LR)

    test_prediction = lasagne.layers.get_output(mlp, deterministic=True)
    test_loss = T.mean(T.sqr(T.maximum(0.,1.-target_var*test_prediction))) #squared hinge loss
    test_err = T.mean(T.neq(binary_ops.sign_theano(test_prediction), target_var))

    train_fn = theano.function([input_var, target_var], loss, updates=updates)
    val_fn = theano.function([input_var, target_var], [test_loss, test_err, test_prediction])

    print("test on D before training:")
    avg_loss, err, output = val_fn(D[0], D[1])
    print("Loss: %f" % avg_loss)
    print("Error: %f%%" % (err * 100))
    print("Output:")
    print(output)

    for i in range(training_steps):
        train_fn(D[0], D[1])

    print("test on D after training:")
    avg_loss, err, output = val_fn(D[0], D[1])
    print("Loss: %f" % avg_loss)
    print("Error: %f%%" % (err * 100))
    print("Output:")
    print(output)

    print("Parameters saved to: ", params_path)
    numpy.savez(params_path, *lasagne.layers.get_all_param_values(mlp))


if __name__ == '__main__':
    run_xor_mlp()
