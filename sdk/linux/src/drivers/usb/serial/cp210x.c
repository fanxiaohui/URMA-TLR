/*
 * Silicon Laboratories CP210x USB to RS232 serial adaptor driver
 *
 * Copyright (C) 2005 Craig Shelley (craig@microtron.org.uk)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License version
 *	2 as published by the Free Software Foundation.
 *
 * Support to set flow control line levels using TIOCMGET and TIOCMSET
 * thanks to Karl Hiramoto karl@hiramoto.org. RTSCTS hardware flow
 * control thanks to Munir Nassar nassarmu@real-time.com
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/serial.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/usb.h>
#include <linux/uaccess.h>
#include <linux/usb/serial.h>
#include <linux/workqueue.h>

/*
 * Version Information
 */
#define DRIVER_VERSION "v0.09"
#define DRIVER_DESC "Silicon Labs CP210x RS232 serial adaptor driver"


/* Use this to use embedded events instead of polling status register.
 * NOTE: the status register is stilled polled because of the overrun info */
#define CP210X_USE_EMBED_EVENTS		1

/*
 * Function Prototypes
 */
static int cp210x_open(struct tty_struct *tty, struct usb_serial_port *);
static void cp210x_cleanup(struct usb_serial_port *);
static void cp210x_close(struct usb_serial_port *);
static int cp210x_ioctl(struct tty_struct *tty, struct file *file,
	unsigned int cmd, unsigned long arg);
#if 0
static void cp210x_get_termios(struct tty_struct *,
	struct usb_serial_port *port);
static void cp210x_get_termios_port(struct usb_serial_port *port,
	unsigned int *cflagp, unsigned int *baudp);
#endif
static void cp210x_set_termios(struct tty_struct *, struct usb_serial_port *,
							struct ktermios*);
static int cp210x_tiocmget(struct tty_struct *, struct file *);
static int cp210x_tiocmset(struct tty_struct *, struct file *,
		unsigned int, unsigned int);
static int cp210x_tiocmset_port(struct usb_serial_port *port, struct file *,
		unsigned int, unsigned int);
static void cp210x_break_ctl(struct tty_struct *, int);
static int cp210x_startup(struct usb_serial *);
static void cp210x_release(struct usb_serial *);
static void cp210x_disconnect(struct usb_serial *);
static void cp210x_dtr_rts(struct usb_serial_port *p, int on);
static int cp210x_prepare_write_buffer(struct usb_serial_port *port,
		void *dest, size_t size);
static void cp210x_process_read_urb(struct urb *urb);
static int cp210x_get_serial_stat(struct usb_serial_port *port,
				  bool count_errors);
static void cp210x_stat_poll_set_next(struct usb_serial_port *port);
#ifdef CP210X_USE_EMBED_EVENTS
static int cp210x_enable_events(struct usb_serial_port *port, bool is_enabled);
#endif


static int debug;

static struct usb_device_id id_table [] = {
	{ USB_DEVICE(0x045B, 0x0053) }, /* Renesas RX610 RX-Stick */
	{ USB_DEVICE(0x0471, 0x066A) }, /* AKTAKOM ACE-1001 cable */
	{ USB_DEVICE(0x0489, 0xE000) }, /* Pirelli Broadband S.p.A, DP-L10 SIP/GSM Mobile */
	{ USB_DEVICE(0x0745, 0x1000) }, /* CipherLab USB CCD Barcode Scanner 1000 */
	{ USB_DEVICE(0x08e6, 0x5501) }, /* Gemalto Prox-PU/CU contactless smartcard reader */
	{ USB_DEVICE(0x08FD, 0x000A) }, /* Digianswer A/S , ZigBee/802.15.4 MAC Device */
	{ USB_DEVICE(0x0BED, 0x1100) }, /* MEI (TM) Cashflow-SC Bill/Voucher Acceptor */
	{ USB_DEVICE(0x0BED, 0x1101) }, /* MEI series 2000 Combo Acceptor */
	{ USB_DEVICE(0x0FCF, 0x1003) }, /* Dynastream ANT development board */
	{ USB_DEVICE(0x0FCF, 0x1004) }, /* Dynastream ANT2USB */
	{ USB_DEVICE(0x0FCF, 0x1006) }, /* Dynastream ANT development board */
	{ USB_DEVICE(0x10A6, 0xAA26) }, /* Knock-off DCU-11 cable */
	{ USB_DEVICE(0x10AB, 0x10C5) }, /* Siemens MC60 Cable */
	{ USB_DEVICE(0x10B5, 0xAC70) }, /* Nokia CA-42 USB */
	{ USB_DEVICE(0x10C4, 0x0F91) }, /* Vstabi */
	{ USB_DEVICE(0x10C4, 0x1101) }, /* Arkham Technology DS101 Bus Monitor */
	{ USB_DEVICE(0x10C4, 0x1601) }, /* Arkham Technology DS101 Adapter */
	{ USB_DEVICE(0x10C4, 0x800A) }, /* SPORTident BSM7-D-USB main station */
	{ USB_DEVICE(0x10C4, 0x803B) }, /* Pololu USB-serial converter */
	{ USB_DEVICE(0x10C4, 0x8044) }, /* Cygnal Debug Adapter */
	{ USB_DEVICE(0x10C4, 0x804E) }, /* Software Bisque Paramount ME build-in converter */
	{ USB_DEVICE(0x10C4, 0x8053) }, /* Enfora EDG1228 */
	{ USB_DEVICE(0x10C4, 0x8054) }, /* Enfora GSM2228 */
	{ USB_DEVICE(0x10C4, 0x8066) }, /* Argussoft In-System Programmer */
	{ USB_DEVICE(0x10C4, 0x806F) }, /* IMS USB to RS422 Converter Cable */
	{ USB_DEVICE(0x10C4, 0x807A) }, /* Crumb128 board */
	{ USB_DEVICE(0x10C4, 0x80CA) }, /* Degree Controls Inc */
	{ USB_DEVICE(0x10C4, 0x80DD) }, /* Tracient RFID */
	{ USB_DEVICE(0x10C4, 0x80F6) }, /* Suunto sports instrument */
	{ USB_DEVICE(0x10C4, 0x8115) }, /* Arygon NFC/Mifare Reader */
	{ USB_DEVICE(0x10C4, 0x813D) }, /* Burnside Telecom Deskmobile */
	{ USB_DEVICE(0x10C4, 0x813F) }, /* Tams Master Easy Control */
	{ USB_DEVICE(0x10C4, 0x814A) }, /* West Mountain Radio RIGblaster P&P */
	{ USB_DEVICE(0x10C4, 0x814B) }, /* West Mountain Radio RIGtalk */
	{ USB_DEVICE(0x10C4, 0x8156) }, /* B&G H3000 link cable */
	{ USB_DEVICE(0x10C4, 0x815E) }, /* Helicomm IP-Link 1220-DVM */
	{ USB_DEVICE(0x10C4, 0x818B) }, /* AVIT Research USB to TTL */
	{ USB_DEVICE(0x10C4, 0x819F) }, /* MJS USB Toslink Switcher */
	{ USB_DEVICE(0x10C4, 0x81A6) }, /* ThinkOptics WavIt */
	{ USB_DEVICE(0x10C4, 0x81A9) }, /* Multiplex RC Interface */
	{ USB_DEVICE(0x10C4, 0x81AC) }, /* MSD Dash Hawk */
	{ USB_DEVICE(0x10C4, 0x81AD) }, /* INSYS USB Modem */
	{ USB_DEVICE(0x10C4, 0x81C8) }, /* Lipowsky Industrie Elektronik GmbH, Baby-JTAG */
	{ USB_DEVICE(0x10C4, 0x81E2) }, /* Lipowsky Industrie Elektronik GmbH, Baby-LIN */
	{ USB_DEVICE(0x10C4, 0x81E7) }, /* Aerocomm Radio */
	{ USB_DEVICE(0x10C4, 0x81E8) }, /* Zephyr Bioharness */
	{ USB_DEVICE(0x10C4, 0x81F2) }, /* C1007 HF band RFID controller */
	{ USB_DEVICE(0x10C4, 0x8218) }, /* Lipowsky Industrie Elektronik GmbH, HARP-1 */
	{ USB_DEVICE(0x10C4, 0x822B) }, /* Modem EDGE(GSM) Comander 2 */
	{ USB_DEVICE(0x10C4, 0x826B) }, /* Cygnal Integrated Products, Inc., Fasttrax GPS demostration module */
	{ USB_DEVICE(0x10C4, 0x8293) }, /* Telegesys ETRX2USB */
	{ USB_DEVICE(0x10C4, 0x82F9) }, /* Procyon AVS */
	{ USB_DEVICE(0x10C4, 0x8341) }, /* Siemens MC35PU GPRS Modem */
	{ USB_DEVICE(0x10C4, 0x8382) }, /* Cygnal Integrated Products, Inc. */
	{ USB_DEVICE(0x10C4, 0x83A8) }, /* Amber Wireless AMB2560 */
	{ USB_DEVICE(0x10C4, 0x83D8) }, /* DekTec DTA Plus VHF/UHF Booster/Attenuator */
	{ USB_DEVICE(0x10C4, 0x8411) }, /* Kyocera GPS Module */
	{ USB_DEVICE(0x10C4, 0x8418) }, /* IRZ Automation Teleport SG-10 GSM/GPRS Modem */
	{ USB_DEVICE(0x10C4, 0x846E) }, /* BEI USB Sensor Interface (VCP) */
	{ USB_DEVICE(0x10C4, 0x8477) }, /* Balluff RFID */
	{ USB_DEVICE(0x10C4, 0x85EA) }, /* AC-Services IBUS-IF */
	{ USB_DEVICE(0x10C4, 0x85EB) }, /* AC-Services CIS-IBUS */
	{ USB_DEVICE(0x10C4, 0x8664) }, /* AC-Services CAN-IF */
	{ USB_DEVICE(0x10C4, 0x8665) }, /* AC-Services OBD-IF */
	{ USB_DEVICE(0x10C4, 0xEA60) }, /* Silicon Labs factory default */
	{ USB_DEVICE(0x10C4, 0xEA61) }, /* Silicon Labs factory default */
	{ USB_DEVICE(0x10C4, 0xEA70) }, /* Silicon Labs factory default */
	{ USB_DEVICE(0x10C4, 0xEA80) }, /* Silicon Labs factory default */
	{ USB_DEVICE(0x10C4, 0xEA71) }, /* Infinity GPS-MIC-1 Radio Monophone */
	{ USB_DEVICE(0x10C4, 0xF001) }, /* Elan Digital Systems USBscope50 */
	{ USB_DEVICE(0x10C4, 0xF002) }, /* Elan Digital Systems USBwave12 */
	{ USB_DEVICE(0x10C4, 0xF003) }, /* Elan Digital Systems USBpulse100 */
	{ USB_DEVICE(0x10C4, 0xF004) }, /* Elan Digital Systems USBcount50 */
	{ USB_DEVICE(0x10C5, 0xEA61) }, /* Silicon Labs MobiData GPRS USB Modem */
	{ USB_DEVICE(0x10CE, 0xEA6A) }, /* Silicon Labs MobiData GPRS USB Modem 100EU */
	{ USB_DEVICE(0x13AD, 0x9999) }, /* Baltech card reader */
	{ USB_DEVICE(0x1555, 0x0004) }, /* Owen AC4 USB-RS485 Converter */
	{ USB_DEVICE(0x166A, 0x0303) }, /* Clipsal 5500PCU C-Bus USB interface */
	{ USB_DEVICE(0x16D6, 0x0001) }, /* Jablotron serial interface */
	{ USB_DEVICE(0x16DC, 0x0010) }, /* W-IE-NE-R Plein & Baus GmbH PL512 Power Supply */
	{ USB_DEVICE(0x16DC, 0x0011) }, /* W-IE-NE-R Plein & Baus GmbH RCM Remote Control for MARATON Power Supply */
	{ USB_DEVICE(0x16DC, 0x0012) }, /* W-IE-NE-R Plein & Baus GmbH MPOD Multi Channel Power Supply */
	{ USB_DEVICE(0x16DC, 0x0015) }, /* W-IE-NE-R Plein & Baus GmbH CML Control, Monitoring and Data Logger */
	{ USB_DEVICE(0x17A8, 0x0001) }, /* Kamstrup Optical Eye/3-wire */
	{ USB_DEVICE(0x17A8, 0x0005) }, /* Kamstrup M-Bus Master MultiPort 250D */
	{ USB_DEVICE(0x17F4, 0xAAAA) }, /* Wavesense Jazz blood glucose meter */
	{ USB_DEVICE(0x1843, 0x0200) }, /* Vaisala USB Instrument Cable */
	{ USB_DEVICE(0x18EF, 0xE00F) }, /* ELV USB-I2C-Interface */
	{ USB_DEVICE(0x1BE3, 0x07A6) }, /* WAGO 750-923 USB Service Cable */
	{ USB_DEVICE(0x3195, 0xF190) }, /* Link Instruments MSO-19 */
	{ USB_DEVICE(0x413C, 0x9500) }, /* DW700 GPS USB interface */
	{ } /* Terminating Entry */
};

MODULE_DEVICE_TABLE(usb, id_table);

struct cp210x_port_private {
	__u8			bInterfaceNumber;
	__u8			bPartNumber;
#ifdef CP210X_USE_EMBED_EVENTS
	__u8			event_state;	/* embedded event processing state */
#endif
	struct async_icount	icount;		/* counters */
	struct delayed_work	stat_poll;	/* worker for status polling */
	struct usb_serial_port	*port;		/* serial port associated with */
};


static struct usb_driver cp210x_driver = {
	.name		= "cp210x",
	.probe		= usb_serial_probe,
	.disconnect	= usb_serial_disconnect,
	.id_table	= id_table,
	.no_dynamic_id	= 	1,
};

static struct usb_serial_driver cp210x_device = {
	.driver = {
		.owner =	THIS_MODULE,
		.name = 	"cp210x",
	},
	.usb_driver		= &cp210x_driver,
	.id_table		= id_table,
	.num_ports		= 1,
	.open			= cp210x_open,
	.close			= cp210x_close,
	.ioctl			= cp210x_ioctl,
	.break_ctl		= cp210x_break_ctl,
	.set_termios		= cp210x_set_termios,
	.tiocmget 		= cp210x_tiocmget,
	.tiocmset		= cp210x_tiocmset,
	.attach			= cp210x_startup,
	.release		= cp210x_release,
	.disconnect		= cp210x_disconnect,
	.dtr_rts		= cp210x_dtr_rts,
	.prepare_write_buffer	= cp210x_prepare_write_buffer,
	.process_read_urb	= cp210x_process_read_urb,
};

/* Part number definitions */
#define CP2101_PARTNUM		0x01
#define CP2102_PARTNUM		0x02
#define CP2103_PARTNUM		0x03
#define CP2104_PARTNUM		0x04
#define CP2105_PARTNUM		0x05
#define CP2108_PARTNUM		0x08

/* IOCTLs */
#define IOCTL_GPIOGET		0x8000
#define IOCTL_GPIOSET		0x8001

/* Config request types */
#define REQTYPE_HOST_TO_INTERFACE	0x41
#define REQTYPE_INTERFACE_TO_HOST	0xc1
#define REQTYPE_HOST_TO_DEVICE	0x40
#define REQTYPE_DEVICE_TO_HOST	0xc0

/* Config request codes */
#define CP210X_IFC_ENABLE	0x00
#define CP210X_SET_BAUDDIV	0x01
#define CP210X_GET_BAUDDIV	0x02
#define CP210X_SET_LINE_CTL	0x03
#define CP210X_GET_LINE_CTL	0x04
#define CP210X_SET_BREAK	0x05
#define CP210X_IMM_CHAR		0x06
#define CP210X_SET_MHS		0x07
#define CP210X_GET_MDMSTS	0x08
#define CP210X_SET_XON		0x09
#define CP210X_SET_XOFF		0x0A
#define CP210X_SET_EVENTMASK	0x0B
#define CP210X_GET_EVENTMASK	0x0C
#define CP210X_SET_CHAR		0x0D
#define CP210X_GET_CHARS	0x0E
#define CP210X_GET_PROPS	0x0F
#define CP210X_GET_COMM_STATUS	0x10
#define CP210X_RESET		0x11
#define CP210X_PURGE		0x12
#define CP210X_SET_FLOW		0x13
#define CP210X_GET_FLOW		0x14
#define CP210X_EMBED_EVENTS	0x15
#define CP210X_GET_EVENTSTATE	0x16
#define CP210X_SET_CHARS	0x19
#define CP210X_GET_BAUDRATE	0x1D
#define CP210X_SET_BAUDRATE	0x1E
#define CP210X_VENDOR_SPECIFIC	0xFF

/* CP210X_IFC_ENABLE */
#define UART_ENABLE		0x0001
#define UART_DISABLE		0x0000

/* CP210x_VENDOR_SPECIFIC */
#define CP210X_WRITE_LATCH	0x37E1
#define CP210X_READ_LATCH	0x00C2
#define CP210X_GET_PARTNUM	0x370B

/* CP210X_(SET|GET)_BAUDDIV */
#define BAUD_RATE_GEN_FREQ	0x384000

/* CP210X_(SET|GET)_LINE_CTL */
#define BITS_DATA_MASK		0X0f00
#define BITS_DATA_5		0X0500
#define BITS_DATA_6		0X0600
#define BITS_DATA_7		0X0700
#define BITS_DATA_8		0X0800
#define BITS_DATA_9		0X0900

#define BITS_PARITY_MASK	0x00f0
#define BITS_PARITY_NONE	0x0000
#define BITS_PARITY_ODD		0x0010
#define BITS_PARITY_EVEN	0x0020
#define BITS_PARITY_MARK	0x0030
#define BITS_PARITY_SPACE	0x0040

#define BITS_STOP_MASK		0x000f
#define BITS_STOP_1		0x0000
#define BITS_STOP_1_5		0x0001
#define BITS_STOP_2		0x0002

/* CP210X_SET_BREAK */
#define BREAK_ON		0x0001
#define BREAK_OFF		0x0000

/* CP210X_(SET_MHS|GET_MDMSTS) */
#define CONTROL_DTR		0x0001
#define CONTROL_RTS		0x0002
#define CONTROL_CTS		0x0010
#define CONTROL_DSR		0x0020
#define CONTROL_RING		0x0040
#define CONTROL_DCD		0x0080
#define CONTROL_WRITE_DTR	0x0100
#define CONTROL_WRITE_RTS	0x0200

/* CP210X_EMBED_EVENTS
 * Events are coming together with the data stream. Each event starts with an
 * escape character (can be set programmatically, if that's set to 0, events are
 * disabled).
 * Event types:
 *   <ESCCHAR><0x00>:		Indicates that the <ESCCHAR> itself appeared in
 *				the input stream.
 *
 *   <ESCCHAR><0x01><lsrval><dataval>:	Indicates that a line status register
 *				change occurred when a data byte was waiting to
 *				be received. <lsrval> is the new line status
 *				register value; <dataval> is the next data
 *				character.
 *
 *   <ESCCHAR><0x02><lsrval>:	Indicates that a line status register change
 *				occurred when no data byte was waiting to be
 *				received. <lsrval> is the new line status
 *				register value.
 *   <ESCCHAR><0x03><msrval>:	Indicates that a modem status register change
 *				has occurred. <msrval> is the new value.
 */
#define EMBED_ESC_CHAR		0xA9	/* Used escape character */
#define EMBED_EVENT_ITSELF	0x00	/* The data byte is the escape character
					   (0 byte will follow) */
#define EMBED_EVENT_LSR_DATA	0x01	/* Line status register with pending
					   data byte (2 bytes will follow) */
#define EMBED_EVENT_LSR		0x02	/* Line status register (1 byte will
					   follow) */
#define EMBED_EVENT_MSR		0x03	/* Modem status register (1 byte will
					   follow) */

/* Line status register event flags */
#define LSR_EVENT_DATA_READY	0x01	/* Data ready */
#define LSR_EVENT_HW_OVERRUN	0x02	/* Receiver hardware overrun */
#define LSR_EVENT_PARITY_ERR	0x04	/* Parity error */
#define LSR_EVENT_FRAMING_ERR	0x08	/* Framing error */
#define LSR_EVENT_BREAK		0x10	/* Break */

/* Modem status register event flags */
#define MSR_EVENT_DELTA_CTS	0x01	/* CTS line state changed */
#define MSR_EVENT_DELTA_DSR	0x02	/* DSR line state changed */
#define MSR_EVENT_FALLING_RI	0x04	/* Falling edge of RI occurred */
#define MSR_EVENT_DELTA_DCD	0x08	/* DCD line state changed */
#define MSR_EVENT_CTS		0x10	/* Current state of CTS */
#define MSR_EVENT_DSR		0x20	/* Current state of DSR */
#define MSR_EVENT_RI		0x40	/* Current state of RI */
#define MSR_EVENT_DCD		0x80	/* Current state of DCD */

/* CP210X Serial status - ulErrors*/
#define SERIAL_STAT_ULERRORS_BREAK		0x0001
#define SERIAL_STAT_ULERRORS_FRAMING_ERROR	0x0002
#define SERIAL_STAT_ULERRORS_HW_OVERRUN		0x0004
#define SERIAL_STAT_ULERRORS_QUEUE_OVERRUN	0x0008
#define SERIAL_STAT_ULERRORS_PARITY_ERROR	0x0010
#define SERIAL_STAT_ULERRORS_ALL		0x001F

/* Polling period (msec) for statistics */
#define STAT_POLLING_PERIOD_MS			10

/* Table 8. Serial Status Response */
struct cp210x_serial_stat {
	__u32	ulErrors;
	__u32	ulHoldReasons;
	__u32	ulAmountIn_InQueue;
	__u32	ulAmountIn_OutQueue;
	__u8	bEofReceived;
	__u8	bWaitForImmediate;
	__u8	bReserved;
};


/*
 * cp210x_get_config
 * Reads from the CP210x configuration registers
 * 'size' is specified in bytes.
 * 'data' is a pointer to a pre-allocated array of integers large
 * enough to hold 'size' bytes (with 4 bytes to each integer)
 */
static int cp210x_get_config(struct usb_serial_port *port, u8 requestType,
		u8 request, int value, unsigned int *data, int size)
{
	struct usb_serial *serial = port->serial;
	struct cp210x_port_private *port_priv = usb_get_serial_port_data(port);
	__le32 *buf;
	int result, i, length;

	/* Number of integers required to contain the array */
	length = (((size - 1) | 3) + 1)/4;

	buf = kcalloc(length, sizeof(__le32), GFP_KERNEL);
	if (!buf) {
		dev_err(&port->dev, "%s - out of memory.\n", __func__);
		return -ENOMEM;
	}

	/* Issue the request, attempting to read 'size' bytes */
	result = usb_control_msg(serial->dev, usb_rcvctrlpipe(serial->dev, 0),
				request, requestType, value,
				port_priv->bInterfaceNumber, buf, size, USB_CTRL_GET_TIMEOUT);

	/* Convert data into an array of integers */
	for (i = 0; i < length; i++)
		data[i] = le32_to_cpu(buf[i]);

	/* On CP2108 the GET_LINE_CTL answer bytes (2 bytes) are swapped */
	if ( (request == CP210X_GET_LINE_CTL) && data &&
		(port_priv->bPartNumber == CP2108_PARTNUM) ) {
		data[0] = __swab16(data[0]);
	}

	kfree(buf);

	if (result != size) {
		dbg("%s - Unable to send config request, "
				"request=0x%x size=%d result=%d\n",
				__func__, request, size, result);
		return -EPROTO;
	}

	return 0;
}

/*
 * cp210x_set_config
 * Writes to the CP210x configuration registers
 * Values less than 16 bits wide are sent directly
 * 'size' is specified in bytes.
 */
static int cp210x_set_config(struct usb_serial_port *port, u8 requestType,
		u8 request, int value, unsigned int *data, int size)
{
	struct usb_serial *serial = port->serial;
	struct cp210x_port_private *port_priv = usb_get_serial_port_data(port);
	__le32 *buf = NULL;
	int result, i, length = 0;

	if (size)
	{
		/* Number of integers required to contain the array */
		length = (((size - 1) | 3) + 1)/4;

		buf = kmalloc(length * sizeof(__le32), GFP_KERNEL);
		if (!buf) {
			dev_err(&port->dev, "%s - out of memory.\n",
					__func__);
			return -ENOMEM;
		}

		/* Array of integers into bytes */
		for (i = 0; i < length; i++)
			buf[i] = cpu_to_le32(data[i]);
	}

	result = usb_control_msg(serial->dev,
			usb_sndctrlpipe(serial->dev, 0),
			request, requestType, value,
			port_priv->bInterfaceNumber, buf, size, USB_CTRL_SET_TIMEOUT);


	if (buf)
		kfree(buf);

	if (result != size) {
		dbg("%s - Unable to send request, "
				"request=0x%x size=%d result=%d\n",
				__func__, request, size, result);
		return -EPROTO;
	}

	return 0;
}

/*
 * cp210x_quantise_baudrate
 * Quantises the baud rate as per AN205 Table 1
 */
static unsigned int cp210x_quantise_baudrate(unsigned int baud) {
	if (baud <= 300)
		baud = 300;
	else if (baud <= 600)      baud = 600;
	else if (baud <= 1200)     baud = 1200;
	else if (baud <= 1800)     baud = 1800;
	else if (baud <= 2400)     baud = 2400;
	else if (baud <= 4000)     baud = 4000;
	else if (baud <= 4803)     baud = 4800;
	else if (baud <= 7207)     baud = 7200;
	else if (baud <= 9612)     baud = 9600;
	else if (baud <= 14428)    baud = 14400;
	else if (baud <= 16062)    baud = 16000;
	else if (baud <= 19250)    baud = 19200;
	else if (baud <= 28912)    baud = 28800;
	else if (baud <= 38601)    baud = 38400;
	else if (baud <= 51558)    baud = 51200;
	else if (baud <= 56280)    baud = 56000;
	else if (baud <= 58053)    baud = 57600;
	else if (baud <= 64111)    baud = 64000;
	else if (baud <= 77608)    baud = 76800;
	else if (baud <= 117028)   baud = 115200;
	else if (baud <= 129347)   baud = 128000;
	else if (baud <= 156868)   baud = 153600;
	else if (baud <= 237832)   baud = 230400;
	else if (baud <= 254234)   baud = 250000;
	else if (baud <= 273066)   baud = 256000;
	else if (baud <= 491520)   baud = 460800;
	else if (baud <= 567138)   baud = 500000;
	else if (baud <= 670254)   baud = 576000;
	else if (baud <= 1053257)  baud = 921600;
	else if (baud <= 1474560)  baud = 1228800;
	else if (baud <= 2457600)  baud = 1843200;
	else                       baud = 3686400;
	return baud;
}

static int cp210x_open(struct tty_struct *tty, struct usb_serial_port *port)
{
	struct usb_serial *serial = port->serial;
	int result;

	dbg("%s - port %d", __func__, port->number);

	if (cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_IFC_ENABLE, UART_ENABLE, NULL, 0)) {
		dev_err(&port->dev, "%s - Unable to enable UART\n",
				__func__);
		return -EPROTO;
	}

	/* Start reading from the device */
	usb_fill_bulk_urb(port->read_urb, serial->dev,
			usb_rcvbulkpipe(serial->dev,
			port->bulk_in_endpointAddress),
			port->read_urb->transfer_buffer,
			port->read_urb->transfer_buffer_length,
			serial->type->read_bulk_callback,
			port);
	result = usb_submit_urb(port->read_urb, GFP_KERNEL);
	if (result) {
		dev_err(&port->dev, "%s - failed resubmitting read urb, "
				"error %d\n", __func__, result);
		return result;
	}

	/* Set termios settings */
	if ( tty ) {
		struct ktermios old_termios;

		old_termios.c_cflag = ~tty->termios->c_cflag;
		old_termios.c_iflag = ~tty->termios->c_iflag;
		old_termios.c_ispeed = ~tty->termios->c_ispeed;
		old_termios.c_ospeed = ~tty->termios->c_ospeed;
		cp210x_set_termios(tty, port, &old_termios);
	}

	/* Read out status register to clear errors happened while the port was
	 * closed. */
	cp210x_get_serial_stat(port, false);

	/* Enable status register polling */
	cp210x_stat_poll_set_next(port);

#ifdef CP210X_USE_EMBED_EVENTS
	/* The events are automatically disabled in the chip, when the interface
	 * gets disabled (see cp210x_close() function does -> IFC_ENABLE(0)).
	 * Enable embedded event */
	cp210x_enable_events(port, true);
#endif

	return 0;
}

static void cp210x_cleanup(struct usb_serial_port *port)
{
	struct usb_serial *serial = port->serial;

	dbg("%s - port %d", __func__, port->number);

	if (serial->dev) {
		/* shutdown any bulk reads that might be going on */
		if (serial->num_bulk_out)
			usb_kill_urb(port->write_urb);
		if (serial->num_bulk_in)
			usb_kill_urb(port->read_urb);
	}
}

static void cp210x_close(struct usb_serial_port *port)
{
	struct cp210x_port_private *port_priv = usb_get_serial_port_data(port);

	dbg("%s - port %d", __func__, port->number);

	/* Cancel status register polling work */
	dbg("%s - stopping status reg polling work", __func__);
	cancel_delayed_work_sync(&port_priv->stat_poll);

	/* shutdown our urbs */
	dbg("%s - shutting down urbs", __func__);
	usb_kill_urb(port->write_urb);
	usb_kill_urb(port->read_urb);

	mutex_lock(&port->serial->disc_mutex);
	if (!port->serial->disconnected)
		cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_IFC_ENABLE, UART_DISABLE, NULL, 0);
	mutex_unlock(&port->serial->disc_mutex);
}

static int cp210x_ioctl(struct tty_struct *tty, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	struct usb_serial_port *port = tty->driver_data;
	struct cp210x_port_private *port_priv = usb_get_serial_port_data(port);
	int result = 0;
	unsigned long latch_buf = 0;

	switch (cmd) {

	case IOCTL_GPIOGET:
		if ((port_priv->bPartNumber == CP2103_PARTNUM) ||
			(port_priv->bPartNumber == CP2104_PARTNUM)) {
			result = cp210x_get_config(port,
					REQTYPE_DEVICE_TO_HOST,
					CP210X_VENDOR_SPECIFIC,
					CP210X_READ_LATCH,
					(unsigned int*)&latch_buf, 1);
			if (result != 0)
				return result;
			if (copy_to_user((unsigned int*)arg, &latch_buf, 1))
				return -EFAULT;
			return 0;
		}
		else if (port_priv->bPartNumber == CP2105_PARTNUM) {
			result = cp210x_get_config(port,
					REQTYPE_INTERFACE_TO_HOST,
					CP210X_VENDOR_SPECIFIC,
					CP210X_READ_LATCH,
					(unsigned int*)&latch_buf, 1);
			if (result != 0)
				return result;
			if (copy_to_user((unsigned int*)arg, &latch_buf, 1))
				return -EFAULT;
			return 0;
		}
		else if (port_priv->bPartNumber == CP2108_PARTNUM) {
			result = cp210x_get_config(port,
					REQTYPE_DEVICE_TO_HOST,
					CP210X_VENDOR_SPECIFIC,
					CP210X_READ_LATCH,
					(unsigned int*)&latch_buf, 2);
			if (result != 0)
				return result;
			if (copy_to_user((unsigned int*)arg, &latch_buf, 2))
				return -EFAULT;
			return 0;
		}
		else {
			return -ENOTSUPP;
		}
		break;

	case IOCTL_GPIOSET:
		if ((port_priv->bPartNumber == CP2103_PARTNUM) ||
			(port_priv->bPartNumber == CP2104_PARTNUM)) {
			if (copy_from_user(&latch_buf, (unsigned int*)arg, 2))
				return -EFAULT;
			result = usb_control_msg(port->serial->dev,
					usb_sndctrlpipe(port->serial->dev, 0),
					CP210X_VENDOR_SPECIFIC,
					REQTYPE_HOST_TO_DEVICE,
					CP210X_WRITE_LATCH,
					latch_buf,
					NULL, 0, USB_CTRL_SET_TIMEOUT);
			if (result != 0)
				return result;
			return 0;
		}
		else if (port_priv->bPartNumber == CP2105_PARTNUM) {
			if (copy_from_user(&latch_buf, (unsigned int*)arg, 2))
				return -EFAULT;
			return cp210x_set_config(port,
					REQTYPE_HOST_TO_INTERFACE,
					CP210X_VENDOR_SPECIFIC,
					CP210X_WRITE_LATCH,
					(unsigned int*)&latch_buf, 2);
		}
		else if (port_priv->bPartNumber == CP2108_PARTNUM) {
			if (copy_from_user(&latch_buf, (unsigned int*)arg, 4))
				return -EFAULT;
			return cp210x_set_config(port, REQTYPE_HOST_TO_DEVICE,
					CP210X_VENDOR_SPECIFIC,
					CP210X_WRITE_LATCH,
					(unsigned int*)&latch_buf, 4);
		}
		else {
			return -ENOTSUPP;
		}
		break;

	case TIOCGICOUNT:
	{
		struct serial_icounter_struct icount;
		struct async_icount cnow = port_priv->icount;
		memset(&icount, 0, sizeof(icount));
		icount.cts = cnow.cts;
		icount.dsr = cnow.dsr;
		icount.rng = cnow.rng;
		icount.dcd = cnow.dcd;
		icount.rx = cnow.rx;
		icount.tx = cnow.tx;
		icount.frame = cnow.frame;
		icount.overrun = cnow.overrun;
		icount.parity = cnow.parity;
		icount.brk = cnow.brk;
		icount.buf_overrun = cnow.buf_overrun;
		if (copy_to_user((unsigned int*)arg, &icount, sizeof(icount)))
			return -EFAULT;
		return 0;
	}

	default:
		break;
	}

	return -ENOIOCTLCMD;
}

#if 0
/*
 * cp210x_get_termios
 * Reads the baud rate, data bits, parity, stop bits and flow control mode
 * from the device, corrects any unsupported values, and configures the
 * termios structure to reflect the state of the device
 */
static void cp210x_get_termios(struct tty_struct *tty,
	struct usb_serial_port *port)
{
	unsigned int baud;

	if (tty) {
		cp210x_get_termios_port(tty->driver_data,
			&tty->termios->c_cflag, &baud);
		tty_encode_baud_rate(tty, baud, baud);
	}

	else {
		unsigned int cflag;
		cflag = 0;
		cp210x_get_termios_port(port, &cflag, &baud);
	}
}

/*
 * cp210x_get_termios_port
 * This is the heart of cp210x_get_termios which always uses a &usb_serial_port.
 */
static void cp210x_get_termios_port(struct usb_serial_port *port,
	unsigned int *cflagp, unsigned int *baudp)
{
	unsigned int cflag, modem_ctl[4];
	unsigned int baud;
	unsigned int bits;

	dbg("%s - port %d", __func__, port->number);

	cp210x_get_config(port, REQTYPE_INTERFACE_TO_HOST,
			CP210X_GET_BAUDRATE, 0, &baud, 4);

	dbg("%s - baud rate = %d", __func__, baud);
	*baudp = baud;

	cflag = *cflagp;

	cp210x_get_config(port, REQTYPE_INTERFACE_TO_HOST,
			CP210X_GET_LINE_CTL, 0, &bits, 2);
	cflag &= ~CSIZE;
	switch (bits & BITS_DATA_MASK) {
	case BITS_DATA_5:
		dbg("%s - data bits = 5", __func__);
		cflag |= CS5;
		break;
	case BITS_DATA_6:
		dbg("%s - data bits = 6", __func__);
		cflag |= CS6;
		break;
	case BITS_DATA_7:
		dbg("%s - data bits = 7", __func__);
		cflag |= CS7;
		break;
	case BITS_DATA_8:
		dbg("%s - data bits = 8", __func__);
		cflag |= CS8;
		break;
	case BITS_DATA_9:
		dbg("%s - data bits = 9 (not supported, using 8 data bits)",
								__func__);
		cflag |= CS8;
		bits &= ~BITS_DATA_MASK;
		bits |= BITS_DATA_8;
		cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0);
		break;
	default:
		dbg("%s - Unknown number of data bits, using 8", __func__);
		cflag |= CS8;
		bits &= ~BITS_DATA_MASK;
		bits |= BITS_DATA_8;
		cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0);
		break;
	}

	switch (bits & BITS_PARITY_MASK) {
	case BITS_PARITY_NONE:
		dbg("%s - parity = NONE", __func__);
		cflag &= ~PARENB;
		break;
	case BITS_PARITY_ODD:
		dbg("%s - parity = ODD", __func__);
		cflag |= (PARENB|PARODD);
		break;
	case BITS_PARITY_EVEN:
		dbg("%s - parity = EVEN", __func__);
		cflag &= ~PARODD;
		cflag |= PARENB;
		break;
	case BITS_PARITY_MARK:
		dbg("%s - parity = MARK (not supported, disabling parity)",
				__func__);
		cflag &= ~PARENB;
		bits &= ~BITS_PARITY_MASK;
		cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0);
		break;
	case BITS_PARITY_SPACE:
		dbg("%s - parity = SPACE (not supported, disabling parity)",
				__func__);
		cflag &= ~PARENB;
		bits &= ~BITS_PARITY_MASK;
		cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0);
		break;
	default:
		dbg("%s - Unknown parity mode, disabling parity", __func__);
		cflag &= ~PARENB;
		bits &= ~BITS_PARITY_MASK;
		cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0);
		break;
	}

	cflag &= ~CSTOPB;
	switch (bits & BITS_STOP_MASK) {
	case BITS_STOP_1:
		dbg("%s - stop bits = 1", __func__);
		break;
	case BITS_STOP_1_5:
		dbg("%s - stop bits = 1.5 (not supported, using 1 stop bit)",
								__func__);
		bits &= ~BITS_STOP_MASK;
		cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0);
		break;
	case BITS_STOP_2:
		dbg("%s - stop bits = 2", __func__);
		cflag |= CSTOPB;
		break;
	default:
		dbg("%s - Unknown number of stop bits, using 1 stop bit",
								__func__);
		bits &= ~BITS_STOP_MASK;
		cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0);
		break;
	}

	cp210x_get_config(port, REQTYPE_INTERFACE_TO_HOST,
			CP210X_GET_FLOW, 0, modem_ctl, 16);
	if (modem_ctl[0] & 0x0008) {
		dbg("%s - flow control = CRTSCTS", __func__);
		cflag |= CRTSCTS;
	} else {
		dbg("%s - flow control = NONE", __func__);
		cflag &= ~CRTSCTS;
	}

	*cflagp = cflag;
}
#endif

static void cp210x_set_termios(struct tty_struct *tty,
		struct usb_serial_port *port, struct ktermios *old_termios)
{
	unsigned int cflag, old_cflag;
	unsigned int baud = 0, bits;
	unsigned int modem_ctl[4];

	dbg("%s - port %d", __func__, port->number);

	if (!tty)
		return;

	tty->termios->c_cflag &= ~CMSPAR;
	cflag = tty->termios->c_cflag;
	old_cflag = old_termios->c_cflag;
	baud = cp210x_quantise_baudrate(tty_get_baud_rate(tty));

	dbg("%s - Setting baud rate to %d baud", __func__,
			baud);
	if (cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
			CP210X_SET_BAUDRATE, 0, &baud, sizeof(baud))) {
		dbg("Baud rate requested not supported by device\n");
		baud = tty_termios_baud_rate(old_termios);
	}
	/* Report back the resulting baud rate */
	tty_encode_baud_rate(tty, baud, baud);

	/* If the number of data bits is to be updated */
	if ((cflag & CSIZE) != (old_cflag & CSIZE)) {
		cp210x_get_config(port, REQTYPE_INTERFACE_TO_HOST,
				CP210X_GET_LINE_CTL, 0, &bits, 2);
		bits &= ~BITS_DATA_MASK;
		switch (cflag & CSIZE) {
		case CS5:
			bits |= BITS_DATA_5;
			dbg("%s - data bits = 5", __func__);
			break;
		case CS6:
			bits |= BITS_DATA_6;
			dbg("%s - data bits = 6", __func__);
			break;
		case CS7:
			bits |= BITS_DATA_7;
			dbg("%s - data bits = 7", __func__);
			break;
		case CS8:
			bits |= BITS_DATA_8;
			dbg("%s - data bits = 8", __func__);
			break;
		/*case CS9:
			bits |= BITS_DATA_9;
			dbg("%s - data bits = 9", __func__);
			break;*/
		default:
			dbg("cp210x driver does not "
					"support the number of bits requested,"
					" using 8 bit mode\n");
				bits |= BITS_DATA_8;
				break;
		}
		if (cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0))
			dbg("Number of data bits requested "
					"not supported by device\n");
	}

	if ((cflag & (PARENB|PARODD)) != (old_cflag & (PARENB|PARODD))) {
		cp210x_get_config(port, REQTYPE_INTERFACE_TO_HOST,
				CP210X_GET_LINE_CTL, 0, &bits, 2);
		bits &= ~BITS_PARITY_MASK;
		if (cflag & PARENB) {
			if (cflag & PARODD) {
				bits |= BITS_PARITY_ODD;
				dbg("%s - parity = ODD", __func__);
			} else {
				bits |= BITS_PARITY_EVEN;
				dbg("%s - parity = EVEN", __func__);
			}
		}
		if (cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0))
			dbg("Parity mode not supported "
					"by device\n");
	}

	if ((cflag & CSTOPB) != (old_cflag & CSTOPB)) {
		cp210x_get_config(port, REQTYPE_INTERFACE_TO_HOST,
				CP210X_GET_LINE_CTL, 0, &bits, 2);
		bits &= ~BITS_STOP_MASK;
		if (cflag & CSTOPB) {
			bits |= BITS_STOP_2;
			dbg("%s - stop bits = 2", __func__);
		} else {
			bits |= BITS_STOP_1;
			dbg("%s - stop bits = 1", __func__);
		}
		if (cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_LINE_CTL, bits, NULL, 0))
			dbg("Number of stop bits requested "
					"not supported by device\n");
	}

	if ((cflag & CRTSCTS) != (old_cflag & CRTSCTS)) {
		cp210x_get_config(port, REQTYPE_INTERFACE_TO_HOST,
				CP210X_GET_FLOW, 0, modem_ctl, 16);
		dbg("%s - read modem controls = 0x%.4x 0x%.4x 0x%.4x 0x%.4x",
				__func__, modem_ctl[0], modem_ctl[1],
				modem_ctl[2], modem_ctl[3]);

		if (cflag & CRTSCTS) {
			modem_ctl[0] &= ~0x7B;
			modem_ctl[0] |= 0x09;
			modem_ctl[1] = 0x80;
			dbg("%s - flow control = CRTSCTS", __func__);
		} else {
			modem_ctl[0] &= ~0x7B;
			modem_ctl[0] |= 0x01;
			modem_ctl[1] = 0x40;
			dbg("%s - flow control = NONE", __func__);
		}

		dbg("%s - write modem controls = 0x%.4x 0x%.4x 0x%.4x 0x%.4x",
				__func__, modem_ctl[0], modem_ctl[1],
				modem_ctl[2], modem_ctl[3]);
		cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				CP210X_SET_FLOW, 0, modem_ctl, 16);
	}

}

static int cp210x_tiocmset (struct tty_struct *tty, struct file *file,
		unsigned int set, unsigned int clear)
{
	struct usb_serial_port *port = tty->driver_data;

	/* Don't set RTS line, if HW flow control is enabled */
	if (tty->termios->c_cflag & CRTSCTS) {
		set &= ~TIOCM_RTS;
		clear &= ~TIOCM_RTS;
	}

	return cp210x_tiocmset_port(port, file, set, clear);
}

static int cp210x_tiocmset_port(struct usb_serial_port *port, struct file *file,
		unsigned int set, unsigned int clear)
{
	unsigned int control = 0;

	dbg("%s - port %d", __func__, port->number);

	if (set & TIOCM_RTS) {
		control |= CONTROL_RTS;
		control |= CONTROL_WRITE_RTS;
	}
	if (set & TIOCM_DTR) {
		control |= CONTROL_DTR;
		control |= CONTROL_WRITE_DTR;
	}
	if (clear & TIOCM_RTS) {
		control &= ~CONTROL_RTS;
		control |= CONTROL_WRITE_RTS;
	}
	if (clear & TIOCM_DTR) {
		control &= ~CONTROL_DTR;
		control |= CONTROL_WRITE_DTR;
	}

	dbg("%s - control = 0x%.4x", __func__, control);

	return cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
			CP210X_SET_MHS, control, NULL, 0);
}

static void cp210x_dtr_rts(struct usb_serial_port *p, int on)
{
	unsigned int modem_ctl[4];
	int flow_enabled;

	/* Check if HW flow control is enabled */
	cp210x_get_config(p, REQTYPE_INTERFACE_TO_HOST,
			CP210X_GET_FLOW, 0, modem_ctl, 16);
	dbg("%s - read modem controls = 0x%.4x 0x%.4x 0x%.4x 0x%.4x",
			__func__, modem_ctl[0], modem_ctl[1],
			modem_ctl[2], modem_ctl[3]);

	flow_enabled = modem_ctl[0] & 0x0008;

	if (on) {
		unsigned int value;

		/* Don't set RTS if HW flow control is enabled */
		value = TIOCM_DTR |
			(flow_enabled ? 0 : TIOCM_RTS);
		cp210x_tiocmset_port(p, NULL,  value, 0);
	}
	else {
		/* If HW flow control is enabled, disable HW RTS control to be
		 * able to lower the RTS line */
		if (flow_enabled) {
			modem_ctl[1] &= ~0xC0;
			modem_ctl[1] |= 0x40;
			dbg("%s - disabling HW RTS", __func__);

			dbg("%s - write modem controls = 0x%.4x 0x%.4x 0x%.4x 0x%.4x",
					__func__, modem_ctl[0], modem_ctl[1],
					modem_ctl[2], modem_ctl[3]);
			cp210x_set_config(p, REQTYPE_HOST_TO_INTERFACE,
					CP210X_SET_FLOW, 0, modem_ctl, 16);
		}

		cp210x_tiocmset_port(p, NULL,  0, TIOCM_DTR|TIOCM_RTS);
	}
}

static int cp210x_tiocmget (struct tty_struct *tty, struct file *file)
{
	struct usb_serial_port *port = tty->driver_data;
	unsigned int control;
	int result;

	dbg("%s - port %d", __func__, port->number);

	cp210x_get_config(port, REQTYPE_INTERFACE_TO_HOST,
			CP210X_GET_MDMSTS, 0, &control, 1);

	result = ((control & CONTROL_DTR) ? TIOCM_DTR : 0)
		|((control & CONTROL_RTS) ? TIOCM_RTS : 0)
		|((control & CONTROL_CTS) ? TIOCM_CTS : 0)
		|((control & CONTROL_DSR) ? TIOCM_DSR : 0)
		|((control & CONTROL_RING)? TIOCM_RI  : 0)
		|((control & CONTROL_DCD) ? TIOCM_CD  : 0);

	dbg("%s - control = 0x%.2x", __func__, control);

	return result;
}

static void cp210x_break_ctl (struct tty_struct *tty, int break_state)
{
	struct usb_serial_port *port = tty->driver_data;
	unsigned int state;

	dbg("%s - port %d", __func__, port->number);
	if (break_state == 0)
		state = BREAK_OFF;
	else
		state = BREAK_ON;
	dbg("%s - turning break %s", __func__,
			state == BREAK_OFF ? "off" : "on");
	cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
			CP210X_SET_BREAK, state, NULL, 0);
}

static int cp210x_prepare_write_buffer(struct usb_serial_port *port,
		void *dest, size_t size)
{
	int count;
	struct cp210x_port_private *port_priv = usb_get_serial_port_data(port);

	count = kfifo_out_locked(&port->write_fifo, dest, size, &port->lock);

	port_priv->icount.tx += count;

	return count;
}

#ifdef CP210X_USE_EMBED_EVENTS
/* Embedded event processing states */
enum {
	EMBED_EVENT_STATE_ESCAPE = 0,	/* Waiting for escape character */
	EMBED_EVENT_STATE_EVENT,	/* Waiting for event byte */
	EMBED_EVENT_STATE_LSR_NO_DATA,	/* Line status register, no
					   data byte */
	EMBED_EVENT_STATE_LSR_LSR,	/* Line status register, line
					   status register value */
	EMBED_EVENT_STATE_LSR_DATA,	/* Line status register, data */
	EMBED_EVENT_STATE_MSR		/* Modem status register */
};

static unsigned int cp210x_process_embed_event(
	struct cp210x_port_private *port_priv, unsigned char *buffer,
	unsigned int length)
{
	struct async_icount *icount = &port_priv->icount;
	unsigned char *wr_ptr;
	unsigned char *event_state = &port_priv->event_state;
	unsigned int new_length = 0;

	/* Pull out embedded events from the stream, and process them */
	wr_ptr = buffer;
	for ( ; length > 0; --length, ++buffer) {

		if (likely(*event_state == EMBED_EVENT_STATE_ESCAPE)) {
			if (likely(*buffer != EMBED_ESC_CHAR)) {
				/* Copy data if not escape character */
				*wr_ptr++ = *buffer;
				++new_length;
			}
			else {
				/* Found an embedded event, let's see, what it
				 * is */
				*event_state = EMBED_EVENT_STATE_EVENT;
			}

			continue;
		}

		switch (*event_state) {
		/* Got an event, processing command byte */
		case EMBED_EVENT_STATE_EVENT:
			switch (*buffer) {
			case EMBED_EVENT_ITSELF:
				/* Escape of escape character.
				 * Put escape character back into buffer as
				 * data byte. */
				*wr_ptr++ = EMBED_ESC_CHAR;
				++new_length;
				*event_state = EMBED_EVENT_STATE_ESCAPE;

				break;

			case EMBED_EVENT_LSR_DATA:
				/* Received line status register with pending
				 * data.
				 * LSR value comes first */
				*event_state = EMBED_EVENT_STATE_LSR_LSR;

				break;

			case EMBED_EVENT_LSR:
				/* Line status register without pending data */
				*event_state = EMBED_EVENT_STATE_LSR_NO_DATA;

				break;

			case EMBED_EVENT_MSR:
				/* Modem status register */
				*event_state = EMBED_EVENT_STATE_MSR;

				break;

			default:
				/* Unknown event */
				dbg("%s - err unknown event 0x%x", __func__,
					*buffer);

				/* Don't know better, than go back waiting for
				 * escape character */
				*event_state = EMBED_EVENT_STATE_ESCAPE;

				break;
			}

			break;

		/* Processing line status register byte */
		case EMBED_EVENT_STATE_LSR_LSR:
		case EMBED_EVENT_STATE_LSR_NO_DATA:
		{
			uint8_t lsr = *buffer;

			/* Count HW overrun here. The HW receiver of the CP2108
			 * has a 4-byte buffer, with a 75% watermark-level. If
			 * that fills, the CP2108 generates a HW overrun error.
			 * It also has a 2k buffer, which the CP2108 queues
			 * data. Should poll for serial status register to
			 * check for this 2k buffer overrun. */
			/* Break takes precedence over parity, which takes
			 * precedence over framing errors */
			if (lsr & LSR_EVENT_BREAK)
				++icount->brk;
			else if (lsr & LSR_EVENT_PARITY_ERR)
				++icount->parity;
			else if (lsr & LSR_EVENT_FRAMING_ERR)
				++icount->frame;

			if (lsr & LSR_EVENT_HW_OVERRUN)
				++icount->overrun;

			if (*event_state == EMBED_EVENT_STATE_LSR_LSR)
				*event_state = EMBED_EVENT_STATE_LSR_DATA;
			else
				*event_state = EMBED_EVENT_STATE_ESCAPE;
		}
			break;

		/* Processing data byte after line status register byte */
		case EMBED_EVENT_STATE_LSR_DATA:
			/* I don't need this data. Just go back waiting for
			 * escape character */
			*event_state = EMBED_EVENT_STATE_ESCAPE;

			break;

		/* Processing modem status register byte */
		case EMBED_EVENT_STATE_MSR:
		{
			uint8_t msr = *buffer;

			if (msr & MSR_EVENT_DELTA_CTS)
				++icount->cts;
			if (msr & MSR_EVENT_DELTA_DSR)
				++icount->dsr;
			if (msr & MSR_EVENT_DELTA_DCD)
				++icount->dcd;

			*event_state = EMBED_EVENT_STATE_ESCAPE;
		}
			break;

		default:
			/* Should not happen */
			dbg("%s - err unknown event state 0x%x", __func__,
			    *event_state);

			*event_state = EMBED_EVENT_STATE_ESCAPE;

			break;
		}
	}

	return new_length;
}
#endif

static void cp210x_process_read_urb(struct urb *urb)
{
	struct usb_serial_port *port = urb->context;
	struct tty_struct *tty;
	struct cp210x_port_private *port_priv = usb_get_serial_port_data(port);
	char *ch = (char *)urb->transfer_buffer;
	unsigned int packet_length = urb->actual_length;

	if (!packet_length)
		return;

	tty = tty_port_tty_get(&port->port);
	if (!tty)
		return;

#ifdef CP210X_USE_EMBED_EVENTS
	packet_length = cp210x_process_embed_event(port_priv, ch, packet_length);
	if (!packet_length)
		goto out;
#endif

	port_priv->icount.rx += packet_length;

	/* The per character mucking around with sysrq path it too slow for
	   stuff like 3G modems, so shortcircuit it in the 99.9999999% of cases
	   where the USB serial is not a console anyway */
	if (!port->port.console || !port->sysrq)
		tty_insert_flip_string(tty, ch, packet_length);
	else {
		do {
			if (!usb_serial_handle_sysrq_char(port, *ch))
				tty_insert_flip_char(tty, *ch, TTY_NORMAL);
			++ch;
		} while (--packet_length);
	}
	tty_flip_buffer_push(tty);
out:
	tty_kref_put(tty);
}

static int cp210x_get_serial_stat(struct usb_serial_port *port,
				  bool count_errors)
{
	int ret;
	unsigned int buffer[5];
	struct cp210x_serial_stat *stat = (struct cp210x_serial_stat *)buffer;

	ret = cp210x_get_config(port, REQTYPE_INTERFACE_TO_HOST,
				CP210X_GET_COMM_STATUS, 0, buffer, 0x13);
	if (ret != 0) {
		dbg("%s - err %d", __func__, ret);

		return -EIO;
	}

#if 0
	dbg("%s - ulErrors=0x%04x, ulHoldReasons=0x%04x, ulAmountInQueue=0x%04x, "
	    "ulAmountInOutQueue=0x%04x, bEofReceived=0x%x, "
	    "bWaitForImmediate=0x%x", __func__, stat->ulErrors,
	    stat->ulHoldReasons, stat->ulAmountIn_InQueue,
	    stat->ulAmountIn_OutQueue, stat->bEofReceived,
	    stat->bWaitForImmediate);
#endif

	if (count_errors) {
		struct cp210x_port_private *port_priv =
			usb_get_serial_port_data(port);
		struct async_icount *icount = &port_priv->icount;
		unsigned int ulErrors = stat->ulErrors;

#ifndef CP210X_USE_EMBED_EVENTS
		/* Break takes precedence over parity, which takes precedence over
		* framing errors */
		if (ulErrors & SERIAL_STAT_ULERRORS_BREAK)
			++icount->brk;
		else if (ulErrors & SERIAL_STAT_ULERRORS_PARITY_ERROR)
			++icount->parity;
		else if (ulErrors & SERIAL_STAT_ULERRORS_FRAMING_ERROR)
			++icount->frame;
#endif

		/* Count overrun always */
		if (ulErrors & SERIAL_STAT_ULERRORS_QUEUE_OVERRUN)
			++icount->overrun;
	}

	return 0;
}

static void cp210x_stat_poll_work(struct work_struct *w)
{
	struct delayed_work *dw = to_delayed_work(w);
	struct cp210x_port_private *port_priv =
		container_of(dw, struct cp210x_port_private, stat_poll);
	struct usb_serial_port *port = port_priv->port;

	cp210x_get_serial_stat(port, true);

	cp210x_stat_poll_set_next(port);
}

static void cp210x_stat_poll_set_next(struct usb_serial_port *port)
{
	struct cp210x_port_private *port_priv = usb_get_serial_port_data(port);

	schedule_delayed_work(&port_priv->stat_poll,
			      msecs_to_jiffies(STAT_POLLING_PERIOD_MS));
}

#ifdef CP210X_USE_EMBED_EVENTS
static int cp210x_enable_events(struct usb_serial_port *port, bool is_enabled)
{
	struct cp210x_port_private *port_priv = usb_get_serial_port_data(port);
	int ret;

	/* Reset the event state */
	port_priv->event_state = EMBED_EVENT_STATE_ESCAPE;

	ret = cp210x_set_config(port, REQTYPE_HOST_TO_INTERFACE,
				 (is_enabled ? CP210X_EMBED_EVENTS : 0),
				 EMBED_ESC_CHAR, NULL, 0);
	if (ret)
		dbg("%s - cannot %s events", __func__,
		    is_enabled ? "enable" : "disable");

	return ret;
}
#endif

static int cp210x_startup(struct usb_serial *serial)
{
	struct cp210x_port_private *port_priv;
	int i;
	unsigned int partNum;

	for (i = 0; i < serial->num_ports; i++) {
		port_priv = kzalloc(sizeof(*port_priv), GFP_KERNEL);
		if (!port_priv)
			return -ENOMEM;

		memset(port_priv, 0x00, sizeof(*port_priv));
		port_priv->bInterfaceNumber =
		    serial->interface->cur_altsetting->desc.bInterfaceNumber;

		usb_set_serial_port_data(serial->port[i], port_priv);

		/* Get the 1-byte part number of the cp210x device */
		cp210x_get_config(serial->port[i],
			REQTYPE_DEVICE_TO_HOST, CP210X_VENDOR_SPECIFIC,
			CP210X_GET_PARTNUM, &partNum, 1);
		port_priv->bPartNumber = partNum & 0xFF;

		port_priv->port = serial->port[i];

		/* Init work to poll overrun status */
		INIT_DELAYED_WORK(&port_priv->stat_poll, cp210x_stat_poll_work);
	}

	return 0;
}

static void cp210x_release(struct usb_serial *serial)
{
	struct cp210x_port_private *port_priv;
	int i;

	for (i = 0; i < serial->num_ports; i++) {
		port_priv = usb_get_serial_port_data(serial->port[i]);

		/* Canceled status register polling works */
		cancel_delayed_work_sync(&port_priv->stat_poll);

		kfree(port_priv);
		usb_set_serial_port_data(serial->port[i], NULL);
	}
}

static void cp210x_disconnect(struct usb_serial *serial)
{
	int i;

	dbg("%s", __func__);

	/* Stop reads and writes on all ports */
	for (i = 0; i < serial->num_ports; ++i)
		cp210x_cleanup(serial->port[i]);
}

static int __init cp210x_init(void)
{
	int retval;

	retval = usb_serial_register(&cp210x_device);
	if (retval)
		return retval; /* Failed to register */

	retval = usb_register(&cp210x_driver);
	if (retval) {
		/* Failed to register */
		usb_serial_deregister(&cp210x_device);
		return retval;
	}

	/* Success */
	printk(KERN_INFO KBUILD_MODNAME ": " DRIVER_VERSION ":"
	       DRIVER_DESC "\n");
	return 0;
}

static void __exit cp210x_exit(void)
{
	usb_deregister(&cp210x_driver);
	usb_serial_deregister(&cp210x_device);
}

module_init(cp210x_init);
module_exit(cp210x_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Enable verbose debugging messages");
