#include <inttypes.h>
#include <libusb.h>
#include <mpsse.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* List of known FT2232-based devices */
struct vid_pid supported_devices[] = {
    {0x0403, 0x6010, "FT2232 Future Technology Devices International, Ltd"},
    {0x0403, 0x6011, "FT4232 Future Technology Devices International, Ltd"},
    {0x0403, 0x6014, "FT232H Future Technology Devices International, Ltd"},

    /* These devices are based on FT2232 chips, but have not been tested. */
    {0x0403, 0x8878, "Bus Blaster v2 (channel A)"},
    {0x0403, 0x8879, "Bus Blaster v2 (channel B)"},
    {0x0403, 0xBDC8, "Turtelizer JTAG/RS232 Adapter A"},
    {0x0403, 0xCFF8, "Amontec JTAGkey"},
    {0x0403, 0x8A98, "TIAO Multi Protocol Adapter"},
    {0x15BA, 0x0003, "Olimex Ltd. OpenOCD JTAG"},
    {0x15BA, 0x0004, "Olimex Ltd. OpenOCD JTAG TINY"},

    {0, 0, NULL}};

/*
 * Opens and initializes the first FTDI device found.
 *
 * @mode      - Mode to open the device in. One of enum modes.
 * @freq      - Clock frequency to use for the specified mode.
 * @endianess - Specifies how data is clocked in/out (MSB, LSB).
 * @interface   - FTDI interface to use (IFACE_A - IFACE_D).
 * @description - Device product description (set to NULL if not needed).
 * @serial      - Device serial number (set to NULL if not needed).
 *
 * Returns a pointer to an MPSSE context structure.
 * On success, mpsse->open will be set to 1.
 * On failure, mpsse->open will be set to 0.
 */
struct mpsse_context *MPSSEDescSerial(enum modes mode, int freq, int endianess,
                                      int interface, const char *description,
                                      const char *serial) {
  int i = 0;
  struct mpsse_context *mpsse = NULL;

  for (i = 0; supported_devices[i].vid != 0; i++) {
    if ((mpsse = Open(supported_devices[i].vid, supported_devices[i].pid, mode,
                      freq, endianess, interface, description, serial)) !=
        NULL) {
      if (mpsse->open) {
        mpsse->description = (description == NULL)
                                 ? supported_devices[i].description
                                 : (char *)description;
        break;
      }
      /* If there is another device still left to try, free the context pointer
         and try again */
      else if (supported_devices[i + 1].vid != 0) {
        Close(mpsse);
        mpsse = NULL;
      }
    }
  }

  return mpsse;
}

void fprintf_buffer(FILE *stream, const char *format, uint8_t *data, size_t data_size) {
  for (int i = 0; i < data_size; i++) {
    fprintf(stream, format, data[i]);
    if (i < (data_size - 1)) {
      fprintf(stream, " ");
    }
  }
}

int main(int argc, char **argv) {
  int vid = 0, pid = 0;
  const char *description = NULL;
  const char *serial = NULL;
  struct ftdi_context ftdic;
  struct ftdi_device_list *device_list;
  enum ftdi_interface ifnum = INTERFACE_A;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int retval = EXIT_FAILURE;

  char *rxdata = NULL;
  struct mpsse_context *spi = NULL;

  if (argc > 1) {  // if present use it as description
    description = argv[1];
    fprintf(stderr, "Using device description: %s\n", description);
  }

  if (argc > 2) {  // if present use it as description
    serial = argv[2];
    fprintf(stderr, "Using device serial number: %s\n", serial);
  }

  // list all ftdi devices
  ftdi_init(&ftdic);
  if ((retval = ftdi_usb_find_all(&ftdic, &device_list, 0, 0)) < 0) {
    fprintf(stderr, "Can't find FTDI USB devices: %d (%s)\n", retval,
            ftdi_get_error_string(&ftdic));
    goto exit;
  }

  if (retval == 0) {
    fprintf(stderr, "No FTDI devices found. Exit\n");
    retval = EXIT_FAILURE;
    goto exit;
  }

  fprintf(stderr, "Number of FTDI devices found: %d\n", retval);

  int i;
  struct ftdi_device_list *current_dev;
  for (i = 0, current_dev = device_list; current_dev != NULL;
       i++, current_dev = current_dev->next) {
    char manufacturer[128], description[128], serial[128] = "N/A";
    struct libusb_device_descriptor desc = {0};

    fprintf(stderr, "Checking device: %d\n", i);
    if ((retval = libusb_get_device_descriptor(current_dev->dev, &desc))) {
      fprintf(stderr, "libusb_get_device_descriptor failed: %d (%s)\n", retval,
              libusb_strerror(retval));
      goto exit;
    }

    if ((retval = ftdi_usb_get_strings(
             &ftdic, current_dev->dev, manufacturer, 128, description, 128,
             desc.iSerialNumber ? serial : NULL, 128)) < 0) {
      fprintf(stderr, "ftdi_usb_get_strings failed: %d (%s)\n", retval,
              ftdi_get_error_string(&ftdic));
      goto exit;
    }
    fprintf(stderr, "Manufacturer: %s, Description: %s, Serial: %s\n",
            manufacturer, description, serial);
    fprintf(stderr, "Vendor: 0x%04" PRIx16 ", Product: 0x%04" PRIx16 "\n",
            desc.idVendor, desc.idProduct);
    fprintf(stderr, "Bus: %d, Address: %d\n",
            libusb_get_bus_number(current_dev->dev),
            libusb_get_device_address(current_dev->dev));
  }
  ftdi_list_free(&device_list);

  if ((spi = MPSSEDescSerial(SPI0, ONE_MHZ, MSB, IFACE_A, description,
                             serial)) != NULL &&
      spi->open) {
    fprintf(stderr, "%s initialized at %dHz (SPI mode 0)\n",
            GetDescription(spi), GetClock(spi));

    // read lines and send hex bytes
    while (-1 != (read = getline(&line, &len, stdin))) {
      size_t n = 0;
      int nconsumed;
      uint8_t data[SPI_TRANSFER_SIZE];
      size_t ndata = 0;

      line[strcspn(line, "\r\n")] = 0;  // trim trailing line ends
      if (0 == (read = strlen(line))) continue;

      Start(spi);
      do {
        if (1 != sscanf(line + n, "%" SCNx8 "%n", data + ndata, &nconsumed)) {
          fprintf(stderr, "error reading at index %zu: %s", n, line);
          goto exit;
        }
        n += nconsumed;
        ndata++;

        // transfer chunk if we still have data to read and we have SPI_TRANSFER_SIZE ndata
        if ((n < (read-1)) && (ndata >= SPI_TRANSFER_SIZE)) {
          if (FastTransfer(spi, (const char *)data, (char *)data, ndata) ==
              MPSSE_FAIL) {
            fprintf(stderr, "FastTransfer: error transfering data\n");
            goto exit;
          }
          fprintf_buffer(stdout, "%" PRIx8, data, ndata);
          fprintf(stdout, " ");
          ndata = 0;
        }
      } while (n < (read-1));

      //transfer last chunk
      if (FastTransfer(spi, (const char *)data, (char *)data, ndata) ==
          MPSSE_FAIL) {
        fprintf(stderr, "FastTransfer: error transfering data\n");
        goto exit;
      }
      fprintf_buffer(stdout, "%" PRIx8, data, ndata);
      fprintf(stdout, "\n");

      Stop(spi);
    }
    free(line);
    retval = EXIT_SUCCESS;
  } else {
    fprintf(stderr, "Failed to initialize MPSSE: %s\n", ErrorString(spi));
  }

exit:
  Close(spi);
  ftdi_deinit(&ftdic);

  return retval;
}