# About

spi_send is a tool to transfer data to the spi slave examples

It uses the libmpsse library to talk to ftdi mpsse engine

# Install libmpsee

You can find a maintaned version of libmpsse here: https://github.com/l29ah/libmpsse.git

To build it and install it on your system:
$> git clone https://github.com/l29ah/libmpsse.git
$> cd libmpsse/src
$> ./configure --disable-python (I'm not using the python wrapper, so I've configured without it)
$> make
$> sudo make install


# Build spi_send

Adapt the makefile if you have not installed libmpsse on /usr/local
Then just run make

# Usage

spi_send will read from stdin and write received data from the spi slave to stdout.

If you have more than one ftdi device connected spi_send will just choose the first available one.
You can use the description string and the serial number to select the right one.
When you run the command it will list all available devices, for instance:

$> ./spi_send
Number of FTDI devices found: 2
Checking device: 0
Manufacturer: FTDI, Description: FT2232H Dev Board, Serial: AAAA
Vendor: 0x0403, Product: 0x6010
Bus: 2, Address: 12
Checking device: 1
Manufacturer: Lattice, Description: Lattice iCE40UP5K Breakout, Serial: 0000
Vendor: 0x0403, Product: 0x6010
Bus: 1, Address: 8
FT2232 Future Technology Devices International, Ltd initialized at 1000000Hz (SPI mode 0)

Here we have two FT2232 devices. It has selected the device 0.

If we want to select the device 1 we can use the description string:
$> ./spi_send "Lattice iCE40UP5K Breakout"

It's also possible to use the serial number string after the description in case you have several devices with the same description string.
$> ./spi_send "Lattice iCE40UP5K Breakout" "0000"

spi_tool wait for input from stdin. To finish the program you can just send an EOF with Ctrl+d