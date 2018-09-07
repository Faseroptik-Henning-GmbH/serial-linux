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
 * @file serial.h
 * @brief Header for dealing with serial communication.
 * @author Dario Dorando (Faseroptik Henning GmbH)
 * @date 06/09/2018
 * 
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 *  @brief Class for defining a serial port in software
 */
class serial {
public:
	serial() { }

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
	struct termios if_attrib_set(speed_t speed, int clen, bool parityOn, int parityType, int fctrl, bool stopbx);

	/**
	 *  @brief Setup serial port
	 * 
	 *  @param portname Location of the block device (at /dev/)
	 *  @param speed The wanted baud rate (as int)
	 * 
	 *	@return 0 on success, -1 otherwise.
	 */
	int setupSerialPort(const char* portname, int speed);

	/**
	 *  @brief Wrapper for _serialPut()
	 *  @see _serialPut()
	 * 
	 *  @param buf Text buffer
	 *  @param size Bytes to be sent
	 * 
	 *	@return Number of bytes written when successful, -1 otherwise.
	 */
	int writeToSerialPort(char** buf, size_t size);

	/**
	 *  @brief Read from a serial port
	 * 
	 *  @param buf Text buffer
	 *  @param size Buffer size
	 * 
	 *	@return Number of bytes read when successful, -1 otherwise.
	 */
	int readFromSerialPort(char** buf, size_t size);

private:
	/**
	 *  @brief Conversion of an integer baud to speed_t baud
	 * 
	 *  @param num Wanted baud as int
	 * 
	 *  @return Baud converted to speed_t. (max 460800, min 50)
	 */
	speed_t __convBaud(int num);

	/**
	 *  @brief Send characters on serial port
	 * 
	 *  @param buf Text buffer
	 *  @param size Bytes to be sent
	 * 
	 *	@return Number of bytes written when successful, -1 otherwise.
	 */
	int _serialPut(char** buf, size_t size);

	int _serfd; /**< Serial fd */
};
