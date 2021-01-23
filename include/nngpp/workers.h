#pragma once

#include "aio.h"

class repWorker : public workerBase
{
public:
    WorkStatus state{ WorkStatus::INIT };
    nng::aio aio{ this };
    nng::msg msg;
    nng::ctx ctx;
    explicit repWorker(nng::socket_view sock) : ctx(sock) {}

    virtual void sendData(nng::msg msg) override
    {

    }

    virtual void callback1()
    {
        uint32_t when;
        switch (this->state) {
        case WorkStatus::INIT:
        {
            state = WorkStatus::RECV;
            ctx.recv(aio);
        }break;
        case WorkStatus::RECV:
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
            state = WorkStatus::WAIT;
            nng::sleep(when, aio);
        }break;
        case WorkStatus::WAIT:
        {
            // We could add more data to the message here.
            aio.set_msg(std::move(msg));
            state = WorkStatus::SEND;
            ctx.send(aio);
        } break;
        case WorkStatus::SEND:
        {
            auto result = aio.result();
            if (result != nng::error::success) {
                throw nng::exception(result);
            }
            state = WorkStatus::RECV;
            ctx.recv(aio);
        }
        break;
        default:
            throw nng::exception(nng::error::state);
            break;
        }
    }
};

