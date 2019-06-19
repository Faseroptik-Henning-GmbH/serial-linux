/**
 * Copyright (C) 2018 Dario Dorando (Faseroptik Henning GmbH)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * 
 * @file serial.cpp
 * @brief Code for dealing with serial communication.
 * @author Dario Dorando (Faseroptik Henning GmbH)
 * @date 06/09/2018
 * @date 
 * 
 */

#include "serial.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


/**
 *  @brief Conversion of an integer baud to speed_t baud
 * 
 *  @param num Wanted baud as int
 * 
 *  @return Baud converted to speed_t. (max 460800, min 50)
 */
speed_t FOHSerial::__convBaud(int num) {
	if (num >= 460800) return B460800;
	if (num >= 230400) return B230400;
	if (num >= 115200) return B115200;
	if (num >= 57600) return B57600;
	if (num >= 38400) return B38400;
	if (num >= 19200) return B19200;
	if (num >= 9600) return B9600;
	if (num >= 4800) return B4800;
	if (num >= 2400) return B2400;
	if (num >= 1800) return B1800;
	if (num >= 1200) return B1200;
	if (num >= 600) return B600;
	if (num >= 300) return B300;
	if (num >= 200) return B200;
	if (num >= 150) return B150;
	if (num >= 134) return B134;
	if (num >= 110) return B110;
	if (num >= 75) return B75;
	return B50;
}

/**
 *  @brief Set attributes of a serial interface.
 * 
 *  See https://linux.die.net/man/3/termios for further information on serial attributes.
 * 
 *  @param speed Baudrate in speed_t format
 *  @param clen Byte length (5-8 bits)
 *  @param parityOn Enables/Disables parity
 *  @param parityType Sets the parity type (0: off, 1: even, 2: odd)
 *  @param fctrl Sets the flow control type (0: off, 1: software, 2: hardware, 3: soft- and hardware)
 *  @param stopbx Enables/Disables a second stop bit
 * 
 *	@return New termios structure on success, -1 otherwise.
 */
struct termios FOHSerial::if_attrib_set(speed_t speed, int clen, bool parityOn, int parityType, int fctrl, bool stopbx) {
	struct termios tty;
	struct termios ftty;
	memset(&tty, 0, sizeof tty);
	memset(&ftty, 0, sizeof ftty);

	//Obtain current termios attributes
	if (tcgetattr(_serfd, &tty) != 0) return ftty;

	//Sanity checking
	if (!speed) return ftty;
	if (clen < 5 || clen > 8) return ftty;

	//Set I/O speed
	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);
	tty.c_cflag |= speed;

	//Set byte length
	if (clen == 5)
		tty.c_cflag |= CS5;
	else if (clen == 6)
		tty.c_cflag |= CS6;
	else if (clen == 7)
		tty.c_cflag |= CS7;
	else if (clen == 8)
		tty.c_cflag |= CS8;

	tty.c_cflag |= CLOCAL;
	tty.c_cflag |= CREAD;
	
	tty.c_iflag |= ~IGNBRK;
	tty.c_iflag &= ~BRKINT;

	tty.c_iflag &= ~ICRNL;
	tty.c_iflag &= ~IGNCR;

	tty.c_lflag |= ECHO | ECHOE;
	tty.c_oflag = 0;
	tty.c_cc[VMIN]  = 1;
	tty.c_cc[VTIME] = 5;

	//Set flow control options
	if (fctrl == 0) {
		tty.c_iflag &= ~(IXON | IXOFF | IXANY);
		tty.c_cflag &= ~CRTSCTS;
	} else if (fctrl == 1) {
		tty.c_iflag |= ~(IXON | IXOFF | IXANY);
		tty.c_cflag &= ~CRTSCTS;
	} else if (fctrl == 2) {
		tty.c_iflag &= ~(IXON | IXOFF | IXANY);
		tty.c_cflag |= ~CRTSCTS;
	} else if (fctrl == 3) {
		tty.c_iflag |= ~(IXON | IXOFF | IXANY);
		tty.c_cflag |= ~CRTSCTS;
	} else {
		//Invalid flow control option
		return ftty;
	}

	//Set parity options
	if (parityOn) {
		if (parityType == 1) {
			tty.c_cflag |= ~(PARENB);
			tty.c_iflag &= ~IGNPAR;
			tty.c_iflag |= ~PARMRK;
			tty.c_iflag |= ~INPCK;
		} else if (parityType == 2) {
			tty.c_cflag |= ~(PARENB | PARODD);
			tty.c_iflag &= ~IGNPAR;
			tty.c_iflag |= ~PARMRK;
			tty.c_iflag |= ~INPCK;
		} else {
			//Invalid parity option
			return ftty;
		}
	} else {
		tty.c_cflag &= ~(PARENB | PARODD);
		tty.c_iflag |= ~IGNPAR;
		tty.c_iflag &= ~PARMRK;
		tty.c_iflag &= ~INPCK;
	}

	//Set stop bit option
	if (!stopbx)
		tty.c_cflag &= ~CSTOPB;
	else
		tty.c_cflag |= ~CSTOPB;

	//Apply new termios attributes
	if (tcsetattr(_serfd, TCSANOW, &tty) != 0) return ftty;

	return tty;
}

/**
 *  @brief Send characters on serial port
 * 
 *  @param buf Text buffer
 *  @param size Bytes to be sent
 * 
 *	@return Number of bytes written when successful, -1 otherwise.
 */
int FOHSerial::_serialPut(char** buf, size_t size) {
	if (this->_isValid == false)
		return -1;

	size_t i;
	ssize_t written;

	for (i = 0; i < size; i++) {
		written = write(_serfd, buf[i], 1);
		if (written < 0) break;
	}

	std::cout << i << std::endl;
	//Indicate we stopped earlier. Something with write() went wrong!
	if (i > size) exit(40);//i = -1;
	
	return i;
}

/**
 *  @brief Setup serial port
 * 
 *  @param portname Location of the block device (at /dev/)
 *  @param speed The wanted baud rate (as int)
 * 
 *	@return 0 on success, -1 otherwise.
 */
int FOHSerial::setupSerialPort(const char* portname, int speed) {
	//Open _serfd
	_serfd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);

	//Is _serfd valid?
	if (_serfd < 0) return -1;

	//Set attributes
	struct termios a;
	a = if_attrib_set(__convBaud(speed), 8, false, 0, 0, 0);
	if (a.c_cflag == 0) return -1;

	struct timeval tv;
	tv.tv_sec = 20;
	tv.tv_usec = 0;
	setsockopt(_serfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	//Flush port (prevent read bugs)
	usleep(10000);
	tcflush(_serfd, TCIOFLUSH);

	return 0;
}

/**
 *  @brief Wrapper for _serialPut()
 * 
 *  @param buf Text buffer
 *  @param size Bytes to be sent
 * 
 *	@return Number of bytes written when successful, -1 otherwise.
 */
int FOHSerial::writeToSerialPort(char** buf, size_t size) {
	if (this->_isValid == false)
		return -1;

	int ret = _serialPut(buf, size);

	//Flush buffer
	usleep(10000);
	tcflush(_serfd, TCIOFLUSH);

	return size;
}

/**
 *  @brief Read from a serial port
 * 
 *  @param buf Text buffer
 *  @param size Buffer size
 * 
 *	@return Number of bytes read when successful, -1 otherwise.
 */
int FOHSerial::readFromSerialPort(char** buf, size_t size) {
	if (this->_isValid == false)
		return -1;

	int n = 0;

	//Empty the buffer
	usleep(10000);
	tcflush(_serfd, TCIOFLUSH);

	//Wait for a newline byte (assume we're on a CRNL system)
	//char tmp = NULL;
//	while (tmp != '\n') {
//		tcflush(_serfd, TCIOFLUSH);
//		n = read(_serfd, &tmp, 1);
//		if (n == -1) return -1;
//	}

	char _cbuf = NULL;
	size_t _read = 0;
	while (_cbuf != '\n' && _read < size) {
		n = read(_serfd, &_cbuf, 1);
		if (n == -1) return -1;

		_read++;

		//Write only to our output if we don't have to deal with delimiters
		if (_cbuf != '\n' || _cbuf != '\r')
			strncat(*buf, &_cbuf, sizeof(char));
	}

	_read--;

	return _read;
}

/**
 * @brief Main constructor
 *
 * @param port Block device @ /dev
 * @param speed Desired speed
 * @param param Parameters (see serialParameters)
 *
 * @return Sets valid boolean
 */
FOHSerial::FOHSerial(const char* port, int speed, uint8_t param) {
	 int returns = 0;
	 struct termios returnsb = {0};

	 returns = setupSerialPort(port, speed);
	 if (returns == -1)
		 goto fail_end;

	 //Decode param
	 int clen, parityType, fctrl;
	 bool parityOn, stopbx;

	 /*
	  * clen: bits 0 + 1
	  * 	  00: 5 bits
	  * 	  01: 6 bits
	  * 	  10: 7 bits
	  * 	  11: 8 bits
	  */
	 switch(param & 3) {
		case 0: clen = 5; break;
		case 1: clen = 6; break;
		case 2: clen = 7; break;
		case 3: clen = 8; break;
		default: goto fail_end;
	 }

	 /*
	  * parityOn: bit 2
	  * parityType: bit 3
	  * 		00: Off
	  * 		01: Off
	  * 		10: even
	  * 		11: off
	  */
	 switch(param & 12) {
		 case 0:
		 case 4:  parityOn = false; parityType = 0; break;
		 case 8:  parityOn = true;  parityType = 1; break;
		 case 12: parityOn = true;  parityType = 2; break;
		 default: goto fail_end;
	 }

	 /*
	  * fctrl: bits 4 + 5
	  * 	   00: off
	  * 	   01: software
	  * 	   10: hardware
	  * 	   11: soft- & hardware
	  */
	 switch(param & 48) {
		 case 0:  fctrl = 0; break;
		 case 16: fctrl = 1; break;
		 case 32: fctrl = 2; break;
		 case 48: fctrl = 3; break;
		 default: goto fail_end;
	 }

	 /*
	  * stopbx: bit 6
	  * 	    0: 1 stop bit
	  * 	    1: 2 stop bits
	  */
	 if (stopbx & 64)
		 stopbx = true;
	 else
		 stopbx = false;

	 struct termios nterm;
	 struct termios ntref;
	 memset(&nterm, 0, sizeof(nterm));
	 memset(&ntref, 0, sizeof(ntref));

	 nterm = if_attrib_set(__convBaud(speed), clen, parityOn, parityType, fctrl, stopbx);
	 if (memcmp(&nterm, &ntref, sizeof(nterm)) == 0)
		goto fail_end;

	 _isValid = true;
	 return;

fail_end:
	 _isValid = false;
	 return;
}
