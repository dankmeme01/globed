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

    private:
        std::shared_ptr<T> data_;
        std::mutex& mutex_;
    };

    Guard lock() {
        return Guard(data_, mutex_);
    }

private:
    std::shared_ptr<T> data_;
    std::mutex mutex_;
};

