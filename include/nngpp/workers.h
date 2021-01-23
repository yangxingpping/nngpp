#pragma once

#include "aio.h"

class repWorker : public workerBase
{
public:
    WorkStatus _state{ WorkStatus::INIT };
    nng::aio _aio{ this };
    nng::msg _msg;
    nng::ctx _ctx;
    explicit repWorker(nng::socket_view sock) : _ctx(sock)
    {
        _state = WorkStatus::RECV;
        _ctx.recv(_aio);
    }

    virtual void sendData(nng::msg msg) override
    {
        _aio.begin();
        _msg = msg;
        _ctx.send(_aio);
        _aio.finish();
    }

    virtual void cancelCallback()
    {
    }

    virtual void callback1()
    {
        switch (_state)
        {
        case WorkStatus::INIT:
            break;
        case WorkStatus::RECV:
        {
            {
                auto result = _aio.result();
                if (result != nng::error::success) {
                    throw nng::exception(result);
                }
            }
            {
                auto msg = _aio.release_msg();
                try {
                   
                }
                catch (const nng::exception&) {
                    _ctx.recv(_aio);
                    return;
                }
                _msg = std::move(msg);
                _aio.defer(&_cancelCallback, this);
            }
           
        }break;
        case WorkStatus::WAIT:
            break;
        case WorkStatus::SEND:
            break;
        default:
            break;
        }
    }
};

