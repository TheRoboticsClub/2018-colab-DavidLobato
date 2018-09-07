/*  
 *  Adapted code from iceprog.
 *
 * 
 *  iceprog -- simple programming tool for FTDI-based Lattice iCE programmers
 *
 *  Copyright (C) 2015  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  Relevant Documents:
 *  -------------------
 *  http://www.latticesemi.com/~/media/Documents/UserManuals/EI/icestickusermanual.pdf
 *  http://www.micron.com/~/media/documents/products/data-sheet/nor-flash/serial-nor/n25q/n25q_32mb_3v_65nm.pdf
 *  http://www.ftdichip.com/Support/Documents/AppNotes/AN_108_Command_Processor_for_MPSSE_and_MCU_Host_Bus_Emulation_Modes.pdf
 */

#define _GNU_SOURCE

#include <ftdi.h>
#include <libusb.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

static struct ftdi_context ftdic;
static bool ftdic_open = false;
static bool verbose = false;
static bool ftdic_latency_set = false;
static unsigned char ftdi_latency;

static void check_rx()
{
	while (1) {
		uint8_t data;
		int rc = ftdi_read_data(&ftdic, &data, 1);
		if (rc <= 0)
			break;
		fprintf(stderr, "unexpected rx byte: %02X\n", data);
	}
}

static void error(int status)
{
	check_rx();
	fprintf(stderr, "ABORT.\n");
	if (ftdic_open) {
		if (ftdic_latency_set)
			ftdi_set_latency_timer(&ftdic, ftdi_latency);
		ftdi_usb_close(&ftdic);
	}
	ftdi_deinit(&ftdic);
	exit(status);
}

static uint8_t recv_byte()
{
	uint8_t data;
	while (1) {
		int rc = ftdi_read_data(&ftdic, &data, 1);
		if (rc < 0) {
			fprintf(stderr, "Read error.\n");
			error(2);
		}
		if (rc == 1)
			break;
		usleep(100);
	}
	return data;
}

static void send_byte(uint8_t data)
{
	int rc = ftdi_write_data(&ftdic, &data, 1);
	if (rc != 1) {
		fprintf(stderr, "Write error (single byte, rc=%d, expected %d).\n", rc, 1);
		error(2);
	}
}

static void send_spi(uint8_t *data, int n)
{
	if (n < 1)
		return;

	send_byte(0x11);
	send_byte(n - 1);
	send_byte((n - 1) >> 8);

	int rc = ftdi_write_data(&ftdic, data, n);
	if (rc != n) {
		fprintf(stderr, "Write error (chunk, rc=%d, expected %d).\n", rc, n);
		error(2);
	}
}

static void xfer_spi(uint8_t *data, int n)
{
	if (n < 1)
		return;

	send_byte(0x31);
	send_byte(n - 1);
	send_byte((n - 1) >> 8);

	int rc = ftdi_write_data(&ftdic, data, n);
	if (rc != n) {
		fprintf(stderr, "Write error (chunk, rc=%d, expected %d).\n", rc, n);
		error(2);
	}

	for (int i = 0; i < n; i++)
		data[i] = recv_byte();
}

static void set_gpio(int slavesel_b, int creset_b)
{
	uint8_t gpio = 0;

	if (slavesel_b) {
		// ADBUS4 (GPIOL0)
		gpio |= 0x10;
	}

	if (creset_b) {
		// ADBUS7 (GPIOL3)
		gpio |= 0x80;
	}

	send_byte(0x80);
	send_byte(gpio);
	send_byte(0x93);
}

static int get_cdone()
{
	uint8_t data;
	send_byte(0x81);
	data = recv_byte();
	// ADBUS6 (GPIOL2)
	return (data & 0x40) != 0;
}

static void usage()
{
	fprintf(stderr, "Simple program to send data to spi slave.\n");
}


int main(int argc, char **argv)
{
    const char *devstr = NULL;
    struct ftdi_device_list *device_list;
    enum ftdi_interface ifnum = INTERFACE_A;
	char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int ret;
    

    if (argc > 1) { //if present use it as devstr
        devstr = argv[1];
        fprintf(stderr, "Using device: %s\n", devstr);
    }

    // ---------------------------------------------------------
	// Initialize USB connection to FT2232H
	// ---------------------------------------------------------

	fprintf(stderr, "init..\n");

	ftdi_init(&ftdic);
	if ((ret = ftdi_usb_find_all(&ftdic, &device_list, 0, 0)) < 0) {
	    fprintf(stderr, "Can't find FTDI USB devices: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
	    error(2);
	}

	fprintf(stderr, "Number of FTDI devices found: %d\n", ret);

	int i;
	struct ftdi_device_list * current_dev;
	for(i = 0, current_dev = device_list; current_dev != NULL; i++,current_dev = current_dev->next) {
		char manufacturer[128], description[128], serial[128] = "N/A";
		struct libusb_device_descriptor desc = {0};

	    fprintf(stderr, "Checking device: %d\n", i);
		if ((ret = libusb_get_device_descriptor(current_dev->dev, &desc))) {
			fprintf(stderr, "libusb_get_device_descriptor failed: %d (%s)\n", ret, libusb_strerror(ret));
			error(2);
		}

	    if((ret = ftdi_usb_get_strings(&ftdic, current_dev->dev, 
										manufacturer, 128, 
										description, 128, 
										desc.iSerialNumber ? serial : NULL, 128)) < 0) {
	        fprintf(stderr, "ftdi_usb_get_strings failed: %d (%s)\n", ret, ftdi_get_error_string(&ftdic));
			error(2);
	    }
	    fprintf(stderr, "Manufacturer: %s, Description: %s, Serial: %s\n", manufacturer, description, serial);
		fprintf(stderr, "Vendor: 0x%04"PRIx16", Product: 0x%04"PRIx16"\n", desc.idVendor, desc.idProduct);
		fprintf(stderr, "Bus: %d, Address: %d\n", libusb_get_bus_number(current_dev->dev), libusb_get_device_address(current_dev->dev));
	}
    ftdi_list_free(&device_list);

	ftdi_set_interface(&ftdic, ifnum);

    if (devstr != NULL) {
		if (ftdi_usb_open_string(&ftdic, devstr)) {
			fprintf(stderr, "Can't find FTDI USB device (device string %s).\n", devstr);
			error(2);
		}
	} else {
		if (ftdi_usb_open(&ftdic, 0x0403, 0x6010) && ftdi_usb_open(&ftdic, 0x0403, 0x6014)) {
			fprintf(stderr, "Can't find FTDI USB device (vendor_id 0x0403, device_id 0x6010 or 0x6014).\n");
			error(2);
		}
	}

	ftdic_open = true;

    if (ftdi_usb_reset(&ftdic)) {
		fprintf(stderr, "Failed to reset FTDI USB device.\n");
		error(2);
	}

	if (ftdi_usb_purge_buffers(&ftdic)) {
		fprintf(stderr, "Failed to purge buffers on FTDI USB device.\n");
		error(2);
	}

	if (ftdi_get_latency_timer(&ftdic, &ftdi_latency) < 0) {
		fprintf(stderr, "Failed to get latency timer (%s).\n", ftdi_get_error_string(&ftdic));
		error(2);
	}

	/* 1 is the fastest polling, it means 1 kHz polling */
	if (ftdi_set_latency_timer(&ftdic, 1) < 0) {
		fprintf(stderr, "Failed to set latency timer (%s).\n", ftdi_get_error_string(&ftdic));
		error(2);
	}

	ftdic_latency_set = true;

	if (ftdi_set_bitmode(&ftdic, 0, BITMODE_RESET) < 0) {
		fprintf(stderr, "Failed to set BITMODE_RESET on FTDI USB device.\n");
		error(2);
	}

	if (ftdi_set_bitmode(&ftdic, 0, BITMODE_MPSSE) < 0) {
		fprintf(stderr, "Failed to set BITMODE_RESET on FTDI USB device.\n");
		error(2);
	}

	// enable clock divide by 5
	send_byte(0x8b);

	// set 1.5 MHz clock
	send_byte(0x86);
	send_byte(0x03);
	send_byte(0x00);

	set_gpio(1, 1);
	usleep(100000);

	//read lines and send hex bytes
	while (-1 != (read = getline(&line, &len, stdin))) {
		size_t idx = 0;
		int pos;
		uint8_t data;

		if (read < 2) //read includes \n
			continue;

		//assert SS
		set_gpio(0, 1);
		usleep(2000);

		do {
			if (1 != sscanf(line+idx, "%"SCNx8"%n", &data, &pos)) {
    			fprintf(stderr, "error reading at index %zu: %s", idx, line);
				exit(EXIT_FAILURE);
			}
			idx += pos;

			xfer_spi(&data, 1);
			fprintf(stdout, "%"PRIx8, data);
			if (idx < (read-2))
				fprintf(stdout," ");
		} while(idx < (read-2)); //read includes \n
		fprintf(stdout,"\n");

		//deassert SS
		set_gpio(1, 1);
    }
    free(line);

    // ---------------------------------------------------------
	// Exit
	// ---------------------------------------------------------

	fprintf(stderr, "Bye.\n");
	ftdi_set_latency_timer(&ftdic, ftdi_latency);
	ftdi_disable_bitbang(&ftdic);
	ftdi_usb_close(&ftdic);
	ftdi_deinit(&ftdic);
	return 0;
}
