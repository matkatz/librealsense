// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

const int QUEUE_MAX_SIZE = 10;
// Simplest implementation of a blocking concurrent queue for thread messaging
template<class T>
class single_consumer_queue
{
    std::deque<T> _q;
    std::mutex _mutex;
    std::condition_variable _cv; // not empty signal
    unsigned int _cap;
    bool _accepting;

    // flush mechanism is required to abort wait on cv
    // when need to stop
    std::atomic<bool> _need_to_flush;
public:
    explicit single_consumer_queue<T>(unsigned int cap = QUEUE_MAX_SIZE)
        : _q(), _mutex(), _cv(), _cap(cap), _need_to_flush(false), _accepting(true)
    {}

    void blocking_enqueue(T&& item)
    {
        auto pred = [this]()->bool { return _q.size() <= _cap; };

        std::unique_lock<std::mutex> lock(_mutex);
        if (_accepting)
        {
            _cv.wait(lock, pred);
            _q.push_back(std::move(item));
        }
        lock.unlock();
        _cv.notify_one();
    }

    void enqueue(T&& item)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_accepting)
        {
            _q.push_back(std::move(item));
            if (_q.size() > _cap)
            {
                _q.pop_front();
            }
        }
        lock.unlock();
        _cv.notify_one();
    }

    bool dequeue(T* item, unsigned int timeout_ms = 5000)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _accepting = true;
        const auto ready = [this]() { return (_q.size() > 0) || _need_to_flush; };
        if (!_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), ready))
        {
            _need_to_flush = false;
            return false;
        }

        if (_q.size() <= 0)
        {
            return false;
        }
        *item = std::move(_q.front());
        _q.pop_front();
        _cv.notify_one();
        return true;
    }

    bool peek(T** item)
    {
        std::unique_lock<std::mutex> lock(_mutex);

        if (_q.size() <= 0)
        {
            return false;
        }
        *item = &_q.front();
        return true;
    }

    bool try_dequeue(T* item)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _accepting = true;
        if (_q.size() > 0)
        {
            auto val = std::move(_q.front());
            _q.pop_front();
            *item = std::move(val);
            _cv.notify_one();
            return true;
        }
        return false;
    }

    void clear()
    {
        std::unique_lock<std::mutex> lock(_mutex);

        _accepting = false;
        _need_to_flush = true;

        while (_q.size() > 0)
        {
            auto item = std::move(_q.front());
            _q.pop_front();
        }
        _cv.notify_all();
    }

    void flush()
    {
        std::unique_lock<std::mutex> lock(_mutex);

        _need_to_flush = true;

        while (_q.size() > 0)
        {
            auto item = std::move(_q.front());
            _q.pop_front();
        }
        _cv.notify_all();
    }

    void start()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _need_to_flush = false;
        _accepting = true;
    }

    size_t size()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _q.size();
    }
};

template<class T>
class single_consumer_frame_queue
{
    single_consumer_queue<T> _queue;

public:
    single_consumer_frame_queue<T>(unsigned int cap = QUEUE_MAX_SIZE) : _queue(cap) {}

    void enqueue(T&& item)
    {
        if (item.is_blocking())
            _queue.blocking_enqueue(std::move(item));
        else
            _queue.enqueue(std::move(item));
    }

    bool dequeue(T* item, unsigned int timeout_ms = 5000)
    {
        return _queue.dequeue(item, timeout_ms);
    }

    bool peek(T** item)
    {
        return _queue.peek(item);
    }

    bool try_dequeue(T* item)
    {
        return _queue.try_dequeue(item);
    }

    void clear()
    {
        _queue.clear();
    }

    void start()
    {
        _queue.start();
    }

    void flush()
    {
        _queue.flush();
    }

    size_t size()
    {
        return _queue.size();
    }
};

class dispatcher
{
public:
    class cancellable_timer
    {
    public:
        cancellable_timer(dispatcher* owner)
            : _owner(owner)
        {}

        bool try_sleep(int ms)
        {
            using namespace std::chrono;

            std::unique_lock<std::mutex> lock(_owner->_was_stopped_mutex);
            auto good = [&]() { return _owner->_was_stopped.load(); };
            return !(_owner->_was_stopped_cv.wait_for(lock, milliseconds(ms), good));
        }

    private:
        dispatcher* _owner;
    };

private:
    friend cancellable_timer;
    single_consumer_queue<std::function<void(cancellable_timer)>> _queue;
    std::thread _thread;
    std::atomic<bool> _is_alive;

    std::atomic<bool> _was_stopped;
    std::condition_variable _was_stopped_cv;
    std::mutex _was_stopped_mutex;

public:

    dispatcher(unsigned int cap) :
        _queue(cap),
        _is_alive(true)
    {
        _thread = std::thread([&]()
        {
            while (_is_alive)
            {
                std::function<void(cancellable_timer)> item;

                if (_queue.dequeue(&item))
                {
                    cancellable_timer time(this);

                    try
                    {
                        item(time);
                    }
                    catch (...) {}
                }
            }
        });
    }

    ~dispatcher()
    {
        stop(true);
        _is_alive = false;
        _thread.join();
    }

    template<class T>
    void invoke(T item, bool blocking = false)
    {
        if (!_was_stopped)
        {
            if (blocking)
                _queue.blocking_enqueue(std::move(item));
            else
                _queue.enqueue(std::move(item));
        }
    }

    void start()
    {
        std::lock_guard<std::mutex> locker(_was_stopped_mutex);
        _was_stopped = false;
        _queue.start();
    }

    void stop(bool clear = true)
    {
        {
            std::lock_guard<std::mutex> locker(_was_stopped_mutex);
            _was_stopped = true;
            _was_stopped_cv.notify_all();
        }
        flush(clear);
    }

    bool flush(bool clear = false, int timeout_ms = 10000)
    {
        std::condition_variable cv;
        bool invoked = false;

        auto cleaner = [&](cancellable_timer t)
        {
            invoked = true;
            cv.notify_one();
        };

        if (clear)
            _queue.flush();

        _queue.enqueue(cleaner);

        std::mutex m;
        std::unique_lock<std::mutex> locker(m);
        return cv.wait_for(locker, std::chrono::milliseconds(timeout_ms), [&]() { return invoked; });
    }

    bool empty()
    {
        return _queue.size() == 0;
    }
};

template<class T = std::function<void(dispatcher::cancellable_timer)>>
class active_object
{
public:
    active_object(T operation)
        : _operation(std::move(operation)), _dispatcher(1), _stopped(true)
    {
    }

    void start()
    {
        _stopped = false;
        _dispatcher.start();

        do_loop();
    }

    void stop()
    {
        _stopped = true;
        _dispatcher.stop();
    }

    ~active_object()
    {
        stop();
    }
private:
    void do_loop()
    {
        _dispatcher.invoke([this](dispatcher::cancellable_timer ct)
        {
            _operation(ct);
            if (!_stopped)
            {
                do_loop();
            }
        });
    }

    T _operation;
    dispatcher _dispatcher;
    std::atomic<bool> _stopped;
};
