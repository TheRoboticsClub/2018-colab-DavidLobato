import os
# TensorFlow and tf.keras
import tensorflow as tf
from tensorflow import keras
from tensorflow.contrib import quantize
from tensorflow.python import debug as tf_debug

# Helper libraries
import numpy as np
import matplotlib.pyplot as plt

sess = tf.Session()

debug = False
if debug:
    sess = tf_debug.TensorBoardDebugWrapperSession(sess, 'localhost:6064')
keras.backend.set_session(sess)

graph_path = os.path.join("models", "xor_mlp.pbtxt")
checkpoint_prefix = os.path.join("models", "xor_mlp.ckpt")

(data, labels) = (
    np.array([[0, 0], [0, 1], [1, 0], [1, 1]]),
    np.array([[0], [1], [1], [0]]) # xor
    # numpy.array([[1], [0], [0], [1]]) # xnor
)

my_init = keras.initializers.glorot_uniform(seed=1)

model = keras.Sequential([
    keras.layers.Dense(5, input_dim=2, activation=tf.nn.relu, kernel_initializer=my_init),
    keras.layers.Dense(1, activation=tf.nn.sigmoid, kernel_initializer=my_init)
])

quantize.experimental_create_training_graph(weight_bits=8, activation_bits=8, quant_delay=5000)

model.compile(optimizer=tf.keras.optimizers.Adam(),
           loss=tf.keras.losses.binary_crossentropy,
           metrics=['accuracy'])

callbacks = [
    # Write TensorBoard logs to `./logs` directory
    keras.callbacks.TensorBoard(log_dir='./logs/xor_mlp'),
    #keras.callbacks.ModelCheckpoint(checkpoint_path, monitor='val_loss', save_best_only=True)
]
sess.run(tf.global_variables_initializer())

saver = tf.train.Saver()

model.fit(data, labels, epochs=10000, callbacks=callbacks)

# Save the checkpoint and eval graph proto to disk for freezing
with open(graph_path, 'w') as f:
    f.write(str(sess.graph.as_graph_def()))

saver.save(sess, checkpoint_prefix)

test_loss, test_acc = model.evaluate(data, labels)

print('Test accuracy:', test_acc)