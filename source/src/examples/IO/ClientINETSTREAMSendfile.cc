//                              -*- Mode: C++ -*- 
// 
// uC++ Version 6.1.0, Copyright (C) Peter A. Buhr 2007
// 
// ClientINETSTREAMSendfile.cc -- Client for INET/stream/sendfile socket test. Client reads file name from standard
//     input, writes the file name the server, reads the file data from the server, and writes that data to standard output.
// 
// Author           : Peter A. Buhr
// Created On       : Sun Oct 14 10:52:27 2007
// Last Modified By : Peter A. Buhr
// Last Modified On : Thu Dec  8 17:41:30 2011
// Update Count     : 29
// 

#include <uSocket.h>
#include <iostream>
using std::cin;
using std::cout;
using std::cerr;
using std::osacquire;
using std::endl;
#include <string>
using std::string;

// minimum buffer size is 2, 1 character and string terminator, '\0'
enum { BufferSize = 65 };
unsigned int rcnt = 0, wcnt = 0;

_Task Reader {
	uSocketClient &client;

	void main() {
		char buf[BufferSize];
		int len;

		for ( ;; ) {
			len = client.read( buf, sizeof(buf) );
		  if ( len == 0 ) break;
			rcnt += len;
			cout.write( buf, len );
		} // for
	} // Reader::main
  public:
	Reader( uSocketClient &client ) : client ( client ) {
	} // Reader::Reader
}; // Reader

_Task Writer {
	uSocketClient &client;

	void main() {
		string fileName;
		struct stat info;

		for ( ;; ) {
			getline( cin, fileName );
		  if ( cin.eof() ) break;
			uFile file( fileName.c_str() );
			file.status( info );
			wcnt = info.st_size;
			client.write( fileName.c_str(), fileName.length() );
		} // for
	} // Writer::main
  public:
	Writer( uSocketClient &client ) : client( client ) {
	} // Writer::Writer
}; // Writer

void uMain::main() {
	switch ( argc ) {
	  case 2:
		break;
	  default:
		cerr << "Usage: " << argv[0] << " port-number" << endl;
		exit( EXIT_FAILURE );
	} // switch

	uSocketClient client( atoi( argv[1] ) );			// connection to server
	{
		Reader rd( client );							// emit worker to read from server and write to output
		Writer wr( client );							// emit worker to read from input and write to server
	}
	if ( wcnt != rcnt ) {
		uAbort( "Error: client not all data transfered, wcnt:%d rcnt:%d", wcnt, rcnt );
	} // if
} // uMain::main

// Local Variables: //
// tab-width: 4 //
// compile-command: "u++-work ClientINETSTREAMSendfile.cc -o Client" //
// End: //
