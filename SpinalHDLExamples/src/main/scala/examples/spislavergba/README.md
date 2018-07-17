This is a simple spi slave device that can control the 3 leds available on iCE40UP5K-B-EVN. The spi protocol is as simple as it gets: one byte, the least significant 3 bits control the status of the leds (xxxxxRGB). The returned data represent the current status.

To check this example on an iCE40UP5K-B-EVN you can use the tool [spi_send](../../../../test/c/spitools/spi_send.c)