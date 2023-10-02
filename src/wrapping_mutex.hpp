/*
* WrappingMutex is a class that mimics Rust's std::sync::Mutex,
* it holds a value and allows you to synchronously access it from multiple threads,
* without having to manually lock/release it.
*/

#pragma once

#include <mutex>
#include <memory>

template <typename T>
class WrappingMutex {
public:
    WrappingMutex(): data_(std::make_shared<T>()), mutex_() {}
    WrappingMutex(T obj) : data_(std::make_shared(obj)), mutex_() {}
    
    class Guard {
    public:
        Guard(std::shared_ptr<T> data, std::mutex& mutex) : mutex_(mutex), data_(data) {
            mutex_.lock();
        }
        ~Guard() {
            if (!alreadyUnlocked)
                mutex_.unlock();
        }
        T& operator* () {
            return *data_;
        }
        T* operator->() {
            return data_.get();
        }
        Guard& operator=(const T& rhs) {
            *data_ = rhs;
            return *this;
        }
        // Calling unlock and trying to use the guard afterwards is undefined behavior!
        void unlock() {
            if (!alreadyUnlocked) {
                mutex_.unlock();
                alreadyUnlocked = true;
            }
        }

    private:
        std::shared_ptr<T> data_;
        std::mutex& mutex_;
        bool alreadyUnlocked = false;
    };

    Guard lock() {
        return Guard(data_, mutex_);
    }

private:
    std::shared_ptr<T> data_;
    std::mutex mutex_;
};

