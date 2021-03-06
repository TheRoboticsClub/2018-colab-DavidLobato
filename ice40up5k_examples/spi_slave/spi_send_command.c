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
	uint8_t gpio = 1;

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

static void hexdump(FILE *stream, const char *str, const uint8_t *packet, size_t length)
{
        int n = 0;

        if (!length) {
                fprintf(stream, "%s zero-length signal packet\n", str);
                return;
        }

        while (length--) {
                if (n % 16 == 0) {
                        fprintf(stream, "%s %08X ", str, n);
                }

                fprintf(stream, "%02X ", *packet++);

                n++;
                if (n % 8 == 0) {
                        if (n % 16 == 0) {
                                fprintf(stream, "\n");
                        } else {
                                fprintf(stream, " ");
                        }
                }
        }

        if (n % 16) {
                fprintf(stream, "\n");
        }
}


static void help(const char *progname)
{
	fprintf(stderr, "Simple program to send data to spi slave.\n");
}

int main(int argc, char **argv)
{
    const char *devstr = NULL;
    enum ftdi_interface ifnum = INTERFACE_A;

	/* used for error reporting */
	const char *my_name = argv[0];
	for (size_t i = 0; argv[0][i]; i++)
		if (argv[0][i] == '/')
			my_name = argv[0] + i + 1;

	if (argc < 2) {
		help(my_name);
		error(2);
	}

	uint8_t command = 0x00;
	if (sscanf(argv[1], "%"SCNx8, &command) != 1) {
		help(my_name);
		error(2);
	}

	fprintf(stderr, "Sending command: 0x%02"SCNx8"\n", command);


    // ---------------------------------------------------------
	// Initialize USB connection to FT2232H
	// ---------------------------------------------------------

	fprintf(stderr, "init..\n");

	ftdi_init(&ftdic);
	ftdi_set_interface(&ftdic, ifnum);

    if (devstr != NULL) {
		if (ftdi_usb_open_string(&ftdic, devstr)) {
			fprintf(stderr, "Can't find iCE FTDI USB device (device string %s).\n", devstr);
			error(2);
		}
	} else {
		if (ftdi_usb_open(&ftdic, 0x0403, 0x6010) && ftdi_usb_open(&ftdic, 0x0403, 0x6014)) {
			fprintf(stderr, "Can't find iCE FTDI USB device (vendor_id 0x0403, device_id 0x6010 or 0x6014).\n");
			error(2);
		}
	}

	ftdic_open = true;

    if (ftdi_usb_reset(&ftdic)) {
		fprintf(stderr, "Failed to reset iCE FTDI USB device.\n");
		error(2);
	}

	if (ftdi_usb_purge_buffers(&ftdic)) {
		fprintf(stderr, "Failed to purge buffers on iCE FTDI USB device.\n");
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

	if (ftdi_set_bitmode(&ftdic, 0xff, BITMODE_MPSSE) < 0) {
		fprintf(stderr, "Failed to set BITMODE_MPSSE on iCE FTDI USB device.\n");
		error(2);
	}

	// enable clock divide by 5
	send_byte(0x8b);

	// set 6 MHz clock
	send_byte(0x86);
	send_byte(0x00);
	send_byte(0x00);

    fprintf(stderr, "cdone: %s\n", get_cdone() ? "high" : "low");

	set_gpio(1, 1);
	usleep(100000);

	//send command
	//hexdump(stderr, "sent>", &command, 1);
	set_gpio(0, 1);
	send_spi(&command, 1);
	set_gpio(1, 1);
	//hexdump(stderr, "recv>", data, sizeof(data));

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
