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



class work : public workerBase 
{
public:
	enum { INIT, RECV, WAIT, SEND } state = INIT;
	nng::aio aio{ this };
	nng::msg msg;
	nng::ctx ctx;
	explicit work( nng::socket_view sock ) : ctx(sock) {}

	virtual void callback1() 
	{
        uint32_t when;
        switch (this->state) {
        case work::INIT:
            state = work::RECV;
            ctx.recv(aio);
            break;
        case work::RECV:
        {
            {
                auto result = aio.result();
                if (result != nng::error::success) {
                    throw nng::exception(result);
                }
            }
            {
                auto msg_ = aio.release_msg();
                try {
                    when = msg_.body().trim_u32();
                }
                catch (const nng::exception&) {
                    // bad message, just ignore it.
                    ctx.recv(aio);
                    return;
                }
                msg = std::move(msg_);
            }
            state = work::WAIT;
            nng::sleep(when, aio);
        }break;
        case work::WAIT:
        {
            // We could add more data to the message here.
            aio.set_msg(std::move(msg));
            state = work::SEND;
            ctx.send(aio);
        } break;
        case work::SEND:
        {
            auto result = aio.result();
            if (result != nng::error::success) {
                throw nng::exception(result);
            }
            state = work::RECV;
            ctx.recv(aio);
        }
        break;
        default:
            throw nng::exception(nng::error::state);
            break;
        }
	}
};

// The server runs forever.
void server(const char* url) {
	//  Create the socket.
	auto sock = nng::rep::open();

	std::unique_ptr<work> works[PARALLEL];
	for(int i=0;i<PARALLEL;++i) {
		works[i] = std::make_unique<work>(sock);
	}

	sock.listen(url);

	for(int i=0;i<PARALLEL;++i) {
        works[i]->callback1();
	}

	while(true) {
		nng::msleep(3600000); // neither pause() nor sleep() portable
	}
}

int main(int argc, char** argv) try {
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
