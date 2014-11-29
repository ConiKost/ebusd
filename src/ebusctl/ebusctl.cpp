/*
 * Copyright (C) Roland Jax 2012-2014 <ebusd@liwest.at>
 *
 * This file is part of ebusd.
 *
 * ebusd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ebusd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ebusd. If not, see http://www.gnu.org/licenses/.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "appl.h"
#include "port.h"
#include "data.h"
#include "tcpsocket.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unistd.h>

using namespace std;

Appl& A = Appl::Instance(true);

void define_args()
{
	A.setVersion("ebusctl is part of """PACKAGE_STRING"");

	A.addText(" 'help' show server commands\n\n"
		  " 'feed' sends a dump file to a local serial device (pts)\n"
		  "       (hint: socat -d -d pty,raw,echo=0 pty,raw,echo=0)\n\n"
		  "Options:\n");

	A.addOption("device", "d", OptVal("/dev/ttyUSB60"), dt_string, ot_mandatory,
		    "virtual serial device (/dev/ttyUSB60)");

	A.addOption("file", "f", OptVal("/tmp/ebus_dump.bin"),dt_string, ot_mandatory,
		    "dump file name (/tmp/ebus_dump.bin)");

	A.addOption("time", "t", OptVal(10000), dt_long, ot_mandatory,
		    "delay between 2 bytes in 'us' (10000)\n");

	A.addOption("server", "s", OptVal("localhost"), dt_string, ot_mandatory,
		    "name or ip (localhost)");

	A.addOption("port", "p", OptVal(8888), dt_int, ot_mandatory,
		    "port (8888)\n");
}

int main(int argc, char* argv[])
{
	// define arguments and application variables
	define_args();

	// parse arguments
	A.parseArgs(argc, argv);

	if (strcasecmp(A.getArg(0).c_str(), "feed") == 0) {
		string dev(A.getOptVal<const char*>("device"));
		Port port(dev, true, false, NULL, false, "", 1);

		port.open();
		if(port.isOpen() == true) {
			cout << "openPort successful." << endl;

			fstream file(A.getOptVal<const char*>("file"), ios::in | ios::binary);

			if (file.is_open() == true) {

				while (file.eof() == false) {
					unsigned char byte = file.get();
					cout << hex << setw(2) << setfill('0')
					     << static_cast<unsigned>(byte) << endl;

					port.send(&byte, 1);
					usleep(A.getOptVal<long>("time"));
				}

				file.close();
			}
			else
				cout << "error opening file " << A.getOptVal<const char*>("file") << endl;

			port.close();
			if(port.isOpen() == false)
				cout << "closePort successful." << endl;
		} else
			cout << "error opening device " << A.getOptVal<const char*>("device") << endl;
	}
	else {

		TCPClient* client = new TCPClient();
		TCPSocket* socket = client->connect(A.getOptVal<const char*>("server"), A.getOptVal<int>("port"));

		if (socket != NULL) {
			// build message
			string message(A.getArg(0));
			for (int i = 1; i < A.numArgs(); i++) {
				message += " ";
				message += A.getArg(i);
			}

			socket->send(message.c_str(), message.size());

			char data[1024];
			size_t datalen;

			datalen = socket->recv(data, sizeof(data)-1);
			data[datalen] = '\0';

			cout << data;

			delete socket;
		}
		else
			cout << "error connecting to " << A.getOptVal<const char*>("server")
			     << ":" << A.getOptVal<int>("port") << endl;

		delete client;
	}

	return 0;
}

