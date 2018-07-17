This is a spi slave device implementing a memory. Read and write are implemented with a very simple protocol:

\<cmdh>\<cmdl>\<data>

* cmdh: b7: op, b6-0: address upper bits, op = 0 read, 1 write
* cmdl: b7-0: address lower bits
* data: dummy data on reads, data to be written otherwise
* ...\[data]: bulk read/write 

Address width depends on parameters defined at SpiSlaveMemConfig 

Bulk read/write is allowed while /CS is asserted. Any following bytes will increment the address +1. It will rollup when overflowing.

To check this example on an iCE40UP5K-B-EVN you can use the tool [spi_send](../../../../test/c/spitools/spi_send.c)