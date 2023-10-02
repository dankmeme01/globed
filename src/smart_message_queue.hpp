/*
* SmartMessageQueue is a utility wrapper around std::queue,
* that allows you to synchronously push/pop messages from multiple threads,
* and additionally even block the thread until new messages are available.
*/

#pragma once
#include <condition_variable>
#include <queue>

template <typename T>
class SmartMessageQueue {
public:
    SmartMessageQueue() {}
    void waitForMessages() {
        std::unique_lock lock(_mtx);
        if (!_iq.empty()) return;

        _cvar.wait(lock, [this](){ return !_iq.empty(); });
    }

    void waitForMessages(std::chrono::seconds timeout) {
        std::unique_lock lock(_mtx);
        if (!_iq.empty()) return;
        
        _cvar.wait_for(lock, timeout, [this](){ return !_iq.empty(); });
    }

    bool empty() {
        std::lock_guard lock(_mtx);
        return _iq.empty();
    }

    T pop() {
        std::lock_guard lock(_mtx);
        auto val = _iq.front();
        _iq.pop();
        return val;
    }

    std::vector<T> popAll() {
        std::vector<T> out;
        std::lock_guard lock(_mtx);
        
        while (!_iq.empty()) {
            out.push_back(_iq.front());
            _iq.pop();
        }

        return out;
    }

    void push(T msg, bool notify = true) {
        std::lock_guard lock(_mtx);
        _iq.push(msg);
        if (notify)
            _cvar.notify_all();
    }

    template <typename Iter>
    void pushAll(const Iter& iterable, bool notify = true) {
        std::lock_guard lock(_mtx);
        std::copy(std::begin(iterable), std::end(iterable), std::back_inserter(_iq));

        if (notify)
            _cvar.notify_all();
    }
private:
    std::queue<T> _iq;
    std::mutex _mtx;
    std::condition_variable _cvar;
};