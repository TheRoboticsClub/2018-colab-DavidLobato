import numpy

import theano
import theano.tensor as T

import lasagne


def build_xor_mlp(input_var=None):
    l_in = lasagne.layers.InputLayer(shape=(1, 2), input_var=input_var)

    l_hid = lasagne.layers.DenseLayer(
        l_in, num_units=2,
        nonlinearity=lasagne.nonlinearities.rectify,
        W=lasagne.init.GlorotUniform()
    )

    l_out = lasagne.layers.DenseLayer(
        l_hid, num_units=1,
        nonlinearity=lasagne.nonlinearities.sigmoid
    )

    return l_out


def run_xor_mlp():
    D = (
        numpy.array([[0, 0], [0, 1], [1, 0], [1, 1]], dtype=theano.config.floatX),
        numpy.array([[0], [1], [1], [0]], dtype=theano.config.floatX) # xor
        # numpy.array([[1], [0], [0], [1]], dtype=theano.config.floatX) # xnor
    )

    training_steps = 10000

    rng = numpy.random.RandomState(1111)
    #lasagne.random.set_rng(rng)

    # Declare Theano symbolic variables
    input_var = T.matrix('inputs')
    target_var = T.matrix('targets')

    # Create neural network model
    network = build_xor_mlp(input_var)

    prediction = lasagne.layers.get_output(network)
    loss = lasagne.objectives.binary_crossentropy(prediction, target_var).mean()

    params = lasagne.layers.get_all_params(network, trainable=True)
    updates = lasagne.updates.adam(loss_or_grads=loss, params=params, learning_rate=0.1)

    test_prediction = lasagne.layers.get_output(network, deterministic=True)
    test_output = T.cast(T.gt(test_prediction, 0.5), theano.config.floatX)
    test_loss = lasagne.objectives.binary_crossentropy(test_prediction, target_var).mean()
    test_err = T.mean(T.neq(test_output, target_var))

    train_fn = theano.function([input_var, target_var], loss, updates=updates)
    val_fn = theano.function([input_var, target_var], [test_loss, test_err, test_output])

    print("test on D before training:")
    avg_loss, err, output = val_fn(D[0], D[1])
    print("Loss: %f" % avg_loss)
    print("Error: %f%%" % (err * 100))
    print("Output:")
    print(output)

    for i in range(training_steps):
        avg_cost = train_fn(D[0], D[1])

    print("test on D after training:")
    avg_loss, err, output = val_fn(D[0], D[1])
    print("Loss: %f" % avg_loss)
    print("Error: %f%%" % (err * 100))
    print("Output:")
    print(output)


if __name__ == '__main__':
    run_xor_mlp()
