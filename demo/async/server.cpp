// This is a port of the nng demo to nngpp
// See https://github.com/nanomsg/nng/tree/master/demo/async

#include <cstdio>
#include <memory>
#include <nngpp/nngpp.h>
#include <nngpp/protocol/rep0.h>
#include <nngpp/platform/platform.h>

// Parallel is the maximum number of outstanding requests we can handle.
// This is *NOT* the number of threads in use, but instead represents
// outstanding work items.  Select a small number to reduce memory size.
// (Each one of these can be thought of as a request-reply loop.)  Note
// that you will probably run into limitations on the number of open file
// descriptors if you set this too high. (If not for that limit, this could
// be set in the thousands, each context consumes a couple of KB.)
#ifndef PARALLEL
#define PARALLEL 128
#endif

#include "nngpp/workers.h"

// The server runs forever.
void server(const char* url) {
	//  Create the socket.
	auto sock = nng::rep::open();

	std::unique_ptr<repWorker> works[PARALLEL];
	for(int i=0;i<PARALLEL;++i) {
		works[i] = std::make_unique<repWorker>(sock);
	}

	sock.listen(url);

	for(int i=0;i<PARALLEL;++i) 
    {
        works[i]->callback1();
	}

	while(true) 
    {
		nng::msleep(3600000); // neither pause() nor sleep() portable
	}
}

int main(int argc, char** argv) try 
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <url>\n", argv[0]);
		return 1;
	}
	server(argv[1]);
}
catch( const nng::exception& e ) {
	fprintf(stderr, "%s: %s\n", e.who(), e.what());
	return 1;
}
