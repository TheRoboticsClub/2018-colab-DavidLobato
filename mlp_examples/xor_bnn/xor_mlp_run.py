import numpy

import theano
import theano.tensor as T

import lasagne
import binary_ops


def build_xor_mlp(input_var=None, hidden_units=4):
    mlp = lasagne.layers.InputLayer(shape=(1, 2), input_var=input_var)

    mlp = lasagne.layers.DenseLayer(
        mlp,
        nonlinearity=lasagne.nonlinearities.identity,
        num_units=hidden_units
    )

    mlp = lasagne.layers.BatchNormLayer(mlp)

    mlp = lasagne.layers.NonlinearityLayer(
        mlp,
        nonlinearity=binary_ops.sign_theano
    )

    mlp = lasagne.layers.DenseLayer(
        mlp,
        nonlinearity=lasagne.nonlinearities.identity,
        num_units=1
    )

    mlp = lasagne.layers.BatchNormLayer(mlp)

    return mlp


def run_xor_mlp():
    D = (
        numpy.array([[-1, -1], [-1, 1], [1, -1], [1, 1]], dtype=theano.config.floatX),
        numpy.array([[-1], [1], [1], [-1]], dtype=theano.config.floatX) # xor
        #numpy.array([[1], [-1], [-1], [1]], dtype=theano.config.floatX) # xnor
    )

    params_path = 'xor_bnn.npz'

    rng = numpy.random.RandomState(1111)
    lasagne.random.set_rng(rng)

    # Declare Theano symbolic variables
    input_var = T.matrix('inputs')
    target_var = T.matrix('targets')

    # Create neural network model
    mlp = build_xor_mlp(input_var, hidden_units=2)

    test_output = binary_ops.sign_theano(lasagne.layers.get_output(mlp, deterministic=True))
    test_err = T.mean(T.neq(test_output, target_var))

    val_fn = theano.function([input_var, target_var], [test_err, test_output])

    print("Loading the trained parameters and binarizing the weights...")

    # Load parameters
    with numpy.load(params_path) as f:
        param_values = [f['arr_%d' % i] for i in range(len(f.files))]
    lasagne.layers.set_all_param_values(mlp, param_values)

    # Binarize the weights
    params = lasagne.layers.get_all_params(mlp)
    for param in params:
        #print(param.name)
        if param.name == "W":
            param.set_value(binary_ops.sign_numpy(param.get_value()))

    print('Running...')

    error, output = val_fn(D[0], D[1])
    print("Error = " + str(error*100.) + "%")
    print("Output = " + str(output))


if __name__ == '__main__':
    run_xor_mlp()
