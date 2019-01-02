#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import logging
from os import environ
from sys import modules, stderr, stdout
from time import sleep
from binascii import hexlify
from pyftdi import FtdiLogger
from pyftdi.spi import SpiController, SpiIOError

if __name__ == '__main__':
    FtdiLogger.log.addHandler(logging.StreamHandler(stdout))
    level = environ.get('FTDI_LOGLEVEL', 'info').upper()
    try:
        loglevel = getattr(logging, level)
    except AttributeError:
        raise ValueError('Invalid log level: %s', level)
    FtdiLogger.set_level(loglevel)

    # Instanciate a SPI controller
    spi = SpiController()

    # Configure the first interface (IF/1) of the FTDI device as a SPI master
    spi.configure('ftdi://ftdi:2232h:AAAA/1')
    #spi.configure('ftdi://ftdi:2232h:1/?')

    # Get a port to a SPI slave w/ /CS on A*BUS3 and SPI mode 0 @ 3MHz
    slave = spi.get_port(cs=0, freq=3E6, mode=0)

    #gpio = spi.get_gpio()
    #gpio.set_direction(0x10, 0x10) #GPIOL0 as output

    # Assert GPIOL0 pin
    #gpio.write(0x10)

    # Synchronous exchange with the remote SPI slave
    write_buf = b'\x01'
    read_buf = slave.exchange(write_buf, duplex=True).tobytes()

    print('Sent: ', hexlify(write_buf))
    print('Got: ', hexlify(read_buf))