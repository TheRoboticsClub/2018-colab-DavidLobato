#!/bin/bash

echo "Make sure the design SpiSlaveMem is loaded and it has been reset."

OUTPUT=`tempfile`

./spi_mem_sequence.sh 2048 | ./spi_send > $OUTPUT
diff $OUTPUT expected_output_2048.txt
if [[ $? -ne 0 ]]; then
    echo "Test failed: $OUTPUT doesn't match expected_output_2048.txt"
else
    echo "Test passed!"
fi