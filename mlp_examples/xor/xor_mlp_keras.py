import os
import argparse
# TensorFlow and tf.keras
import tensorflow as tf
from tensorflow import keras
from tensorflow.contrib import quantize
from tensorflow.python import debug as tf_debug
from tensorflow.python.util import serialization

# Helper libraries
import numpy as np
import json

parser = argparse.ArgumentParser(description='XOR MLP train and evaluation.')
parser.add_argument('-t', '--train', action='store_true', help='Train XOR MLP')
parser.add_argument('-d', '--dump-variables', metavar='PATH', help='Dump variables to file')

args = parser.parse_args()

sess = tf.Session()
keras.backend.set_session(sess)

checkpoint_prefix = os.path.join("models", "xor_mlp.ckpt")

(data, labels) = (
    np.array([[0, 0], [0, 1], [1, 0], [1, 1]]),
    np.array([[0], [1], [1], [0]]) # xor
    #np.array([[1], [0], [0], [1]]) # xnor
)

glorot_init = keras.initializers.glorot_uniform()

model = keras.Sequential([
    keras.layers.Dense(5, input_dim=2, activation=tf.nn.relu, kernel_initializer=glorot_init),
    keras.layers.Dense(1, activation=tf.nn.sigmoid, kernel_initializer=glorot_init)
])

if args.train:
    quantize.experimental_create_training_graph(weight_bits=8, activation_bits=8, quant_delay=5000)

    model.compile(optimizer=tf.keras.optimizers.Adam(),
           loss=tf.keras.losses.binary_crossentropy,
           metrics=['accuracy'])
    sess.run(tf.global_variables_initializer())
    model.fit(data, labels, epochs=10000)
    tf.train.Saver().save(sess, checkpoint_prefix)

    test_loss, test_acc = model.evaluate(data, labels)

    print('Test accuracy:', test_acc)
else:
    quantize.experimental_create_eval_graph(weight_bits=8, activation_bits=8)
    tf.train.Saver().restore(sess, checkpoint_prefix)
    print('Prediction:', model.predict(data))

if args.dump_variables:
    variables = {}
    for tf_variable in tf.global_variables():
        variables[tf_variable.name] = sess.run(tf_variable)
    with open(args.dump_variables, 'w') as f:
        json.dump(variables, f, ensure_ascii=False, default=serialization.get_json_type)
