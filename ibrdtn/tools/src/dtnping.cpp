/*
 * dtnping.cpp
 *
 * Copyright (C) 2011 IBR, TU Braunschweig
 *
 * Written-by: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "config.h"
#include "ibrdtn/api/Client.h"
#include "ibrcommon/net/socket.h"
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"
#include <ibrcommon/thread/SignalHandler.h>
#include "ibrcommon/TimeMeasurement.h"

#include <iostream>
#include <stdint.h>
#include <getopt.h>

#define CREATE_CHUNK_SIZE 2048

class EchoClient : public dtn::api::Client
{
public:
	EchoClient(dtn::api::Client::COMMUNICATION_MODE mode, string app, ibrcommon::socketstream &stream)
		: dtn::api::Client(app, stream, mode), _stream(stream)
	{
		seq = 0;
	}

	virtual ~EchoClient()
	{
	}

	const dtn::data::Bundle waitForReply(const int timeout)
	{
		double wait = (timeout * 1000);
		ibrcommon::TimeMeasurement tm;
		while (wait > 0)
		{
			try
			{
				tm.start();
				dtn::data::Bundle b = this->getBundle((int)(wait / 1000));
				tm.stop();
				checkReply(b);
				return b;
			}
			catch (const std::string &errmsg)
			{
				std::cerr << errmsg << std::endl;
			}
			wait = wait - tm.getMilliseconds();
		}
		throw ibrcommon::Exception("timeout is set to zero");
	}

	void echo(EID destination, int size, int lifetime, bool encryption = false, bool sign = false)
	{
		lastdestination = destination.getString();
		seq++;

		// create a bundle
		dtn::data::Bundle b;

		// set bundle destination
		b.destination = destination;

		// enable encryption if requested
		if (encryption)
			b.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_ENCRYPT, true);

		// enable signature if requested
		if (sign)
			b.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_SIGN, true);

		// set lifetime
		b.lifetime = lifetime;

		// create a new blob
		ibrcommon::BLOB::Reference ref = ibrcommon::BLOB::create();

		// append the blob as payload block to the bundle
		b.push_back(ref);

		// open the iostream
		{
			ibrcommon::BLOB::iostream stream = ref.iostream();

			// Add magic seqno
			(*stream).write((char *)&seq, 4);

			if (size > 4)
			{
				size -= 4;

				// create testing pattern, chunkwise to ocnserve memory
				char pattern[CREATE_CHUNK_SIZE];
				for (size_t i = 0; i < sizeof(pattern); ++i)
				{
					pattern[i] = static_cast<char>(static_cast<int>('0') + (i % 10));
				}

				while (size > CREATE_CHUNK_SIZE)
				{
					(*stream).write(pattern, CREATE_CHUNK_SIZE);
					size -= CREATE_CHUNK_SIZE;
				}

				(*stream).write(pattern, size);
			}
		}

		// send the bundle
		(*this) << b;

		// ... flush out
		flush();
	}

	void checkReply(dtn::data::Bundle &bundle)
	{
		size_t reply_seq = 0;
		ibrcommon::BLOB::Reference blob = bundle.find<dtn::data::PayloadBlock>().getBLOB();
		blob.iostream()->read((char *)(&reply_seq), 4);

		if (reply_seq != seq)
		{
			std::stringstream ss;
			ss << "sequence number mismatch, awaited " << seq << ", got " << reply_seq;
			throw ss.str();
		}
		if (bundle.source.getString() != lastdestination)
		{
			throw std::string("ignoring bundle from source " + bundle.source.getString() + " awaited " + lastdestination);
		}
	}

private:
	ibrcommon::socketstream &_stream;
	uint32_t seq;
	string lastdestination;
};

void print_help()
{
	cout << "-- dtnping (IBR-DTN) --" << endl;
	cout << "Syntax: dtnping [options] <dst>" << endl;
	cout << " <dst>            Set the destination eid (e.g. dtn://node/echo)" << endl
		 << endl;
	cout << "* optional parameters *" << endl;
	cout << " -h|--help        Display this text" << endl;
	cout << " --src <name>     Set the source application name (e.g. echo-client)" << endl;
	cout << " --nowait         Do not wait for a reply" << endl;
	cout << " --abortfail      Abort after first packetloss" << endl;
	cout << " --size           The size of the payload" << endl;
	cout << " --count <n>      Send X echo in a row" << endl;
	cout << " --delay <seconds>" << endl;
	cout << "                  Delay between a received response and the next request" << endl;
	cout << " --lifetime <seconds>" << endl;
	cout << "                  Set the lifetime of outgoing bundles; default: 30" << endl;
	cout << " --encrypt        Request encryption on the bundle layer" << endl;
	cout << " --sign           Request signature on the bundle layer" << endl;
	cout << " -U <socket>      Connect to UNIX domain socket API" << endl;
}

size_t _received = 0, _transmitted = 0;
double _min = 0.0, _max = 0.0, _avg = 0.0;
ibrcommon::TimeMeasurement _runtime;
ibrcommon::Conditional __pause;
dtn::api::Client *__client = NULL;

EID _addr;
bool __exit = false;

void print_summary()
{
	_runtime.stop();

	double loss = 0;
	if (_transmitted > 0)
		loss = ((static_cast<double>(_transmitted) - static_cast<double>(_received)) / static_cast<double>(_transmitted)) * 100.0;

	double avg_value = 0;
	if (_received > 0)
		avg_value = (_avg / static_cast<double>(_received));

	std::cout << std::endl
			  << "--- " << _addr.getString() << " echo statistics --- " << std::endl;
	std::cout << _transmitted << " bundles transmitted, " << _received << " received, " << loss << "% bundle loss, time " << _runtime << std::endl;
	std::cout << "rtt min/avg/max = ";
	ibrcommon::TimeMeasurement::format(std::cout, _min) << "/";
	ibrcommon::TimeMeasurement::format(std::cout, avg_value) << "/";
	ibrcommon::TimeMeasurement::format(std::cout, _max) << " ms" << std::endl;
}

void term(int signal)
{
	if (signal >= 1)
	{
		if (!__exit)
		{
			ibrcommon::MutexLock l(__pause);
			if (__client != NULL)
				__client->abort();
			__exit = true;
			__pause.abort();
		}
	}
}

int main(int argc, char *argv[])
{
	char opt = 0;
	// catch process signals
	ibrcommon::SignalHandler sighandler(term);
	sighandler.handle(SIGINT);
	sighandler.handle(SIGTERM);
	sighandler.initialize();

	string ping_destination = "dtn://local/echo";
	string ping_source = "";
	int ping_size = 64;
	unsigned int lifetime = 30;
	bool wait_for_reply = true;
	bool stop_after_first_fail = false;
	bool nonstop = true;
	size_t interval_pause = 1;
	size_t count = 0;
	dtn::api::Client::COMMUNICATION_MODE mode = dtn::api::Client::MODE_BIDIRECTIONAL;
	ibrcommon::File unixdomain;
	bool bundle_encryption = false;
	bool bundle_signed = false;

	if (argc == 1)
	{
		print_help();
		return 0;
	}

	const struct option long_options[]{
		{"help", no_argument, NULL, 'h'},
		{"encrypt", no_argument, NULL, 'e'},
		{"sign", no_argument, NULL, 's'},
		{"nowait", no_argument, NULL, 'n'},
		{"abortfail", no_argument, NULL, 'a'},
		{"src", required_argument, NULL, 'r'},
		{"size", required_argument, NULL, 'i'},
		{"count", required_argument, NULL, 'c'},
		{"delay", required_argument, NULL, 'd'},
		{"lifetime", required_argument, NULL, 'l'}
	};

	const char *short_options ="hU:";
	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
	{
		switch (opt)
		{
		case 'h':
		{
			print_help();
			return 0;
		}
		case 'e':
		{
			bundle_encryption = true;
			break;
		}
		case 's':
		{
			bundle_signed = true;
			break;
		}
		case 'n':
		{
			mode = dtn::api::Client::MODE_SENDONLY;
			wait_for_reply = false;
			break;
		}
		case 'a':
		{
			stop_after_first_fail = true;
			break;
		}
		case 'r':
		{
			ping_source = optarg;
			break;
		}
		case 'i':
		{
			stringstream str_size;
			str_size.str(optarg);
			str_size >> ping_size;
			break;
		}
		case 'c':
		{
			stringstream str_count;
			str_count.str(optarg);
			str_count >> count;
			nonstop = false;
			break;
		}
		case 'd':
		{
			stringstream str_delay;
			str_delay.str(optarg);
			str_delay >> interval_pause;
			break;
		}
		case 'l':
		{
			stringstream data;
			data << optarg;
			data >> lifetime;
			break;
		}
		case 'U':
		{
			unixdomain = ibrcommon::File(optarg);
			break;
		}
		default:
		{
			std::cout << "unknown command" << std::endl;
			return -1;
		}
		}
	}
	// the last parameter is always the destination
	ping_destination = argv[argc - 1];
	// target address
	_addr = EID(ping_destination);
	ibrcommon::TimeMeasurement tm;
	try
	{
		// Create a stream to the server using TCP.
		ibrcommon::clientsocket *sock = NULL;
		// check if the unixdomain socket exists
		if (unixdomain.exists())
		{
			// connect to the unix domain socket
			sock = new ibrcommon::filesocket(unixdomain);
		}
		else
		{
			// connect to the standard local api port
			ibrcommon::vaddress addr("localhost", 4550);
			sock = new ibrcommon::tcpsocket(addr);
		}
		ibrcommon::socketstream conn(sock);
		std::cout << "I got here\n";
		// Initiate a derivated client
		EchoClient client(mode, ping_source, conn);
		// set the global client pointer
		{
			ibrcommon::MutexLock l(__pause);
			__client = &client;
		}
		// Connect to the server. Actually, this function initiate the
		// stream protocol by starting the thread and sending the contact header.
		client.connect();
		std::cout << "ECHO " << _addr.getString() << " " << ping_size << " bytes of data." << std::endl;
		// measure runtime
		_runtime.start();
		try
		{
			for (unsigned int i = 0; (i < count) || nonstop; ++i)
			{
				// set sending time
				tm.start();
				// Call out a ECHO
				client.echo(_addr, ping_size, lifetime, bundle_encryption, bundle_signed);
				_transmitted++;
				if (wait_for_reply)
				{
					try
					{
						try
						{
							dtn::data::Bundle response = client.waitForReply(2 * lifetime);

							// print out measurement result
							tm.stop();

							size_t reply_seq = 0;
							size_t payload_size = 0;

							// check for min/max/avg
							_avg += tm.getMilliseconds();
							if ((_min > tm.getMilliseconds()) || _min == 0)
								_min = static_cast<double>(tm.getMilliseconds());
							if ((_max < tm.getMilliseconds()) || _max == 0)
								_max = static_cast<double>(tm.getMilliseconds());

							{
								ibrcommon::BLOB::Reference blob = response.find<dtn::data::PayloadBlock>().getBLOB();
								blob.iostream()->read((char *)(&reply_seq), 4);
								payload_size = blob.size();
							}

							std::cout << payload_size << " bytes from " << response.source.getString() << ": seq=" << reply_seq << " ttl=" << response.lifetime.toString() << " time=" << tm << std::endl;
							_received++;
						}
						catch (const dtn::api::ConnectionTimeoutException &e)
						{
							if (stop_after_first_fail)
								throw dtn::api::ConnectionTimeoutException();

							std::cout << "Timeout." << std::endl;
						}

						if (interval_pause > 0)
						{
							ibrcommon::MutexLock l(__pause);
							__pause.wait(interval_pause * 1000);
						}
					}
					catch (const ibrcommon::Conditional::ConditionalAbortException &e)
					{
						if (e.reason == ibrcommon::Conditional::ConditionalAbortException::COND_TIMEOUT)
						{
							continue;
						}
						// aborted
						break;
					}
					catch (const dtn::api::ConnectionAbortedException &)
					{
						// aborted... do a clean shutdown
						break;
					}
					catch (const dtn::api::ConnectionTimeoutException &ex)
					{
						if (stop_after_first_fail)
						{
							std::cout << "No response, aborting." << std::endl;
							break;
						}
					}
				}
			}
		}
		catch (const dtn::api::ConnectionException &)
		{
			std::cerr << "Disconnected." << std::endl;
		}
		catch (const ibrcommon::IOException &)
		{
			std::cerr << "Error while receiving a bundle." << std::endl;
		}
		// Shutdown the client connection.
		client.close();
		conn.close();
	}
	catch (const ibrcommon::socket_exception &)
	{
		std::cerr << "Can not connect to the daemon. Does it run?" << std::endl;
		return -1;
	}
	catch (const std::exception &)
	{
		std::cerr << "unknown error" << std::endl;
	}
	print_summary();
	return 0;
}
