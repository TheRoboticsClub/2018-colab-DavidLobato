This is a simple spi slave device that can control 8 output lines. The spi protocol is as simple as it gets: one byte, each bit controls the status of the output lines. The returned data represent the current status.

To check this example on an iCE40UP5K-B-EVN you can use the tool [spi_send](../../../../test/c/spitools/spi_send.c)