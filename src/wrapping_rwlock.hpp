#pragma once

#include <shared_mutex>
#include <memory>

template <typename T>
class WrappingRwLock {
public:
    WrappingRwLock() : data_(std::make_shared<T>()), rw_mutex_() {}
    WrappingRwLock(T obj) : data_(std::make_shared<T>(obj)), rw_mutex_() {}

    class ReadGuard {
    public:
        ReadGuard(std::shared_ptr<T> data, std::shared_mutex& rw_mutex) : rw_mutex_(rw_mutex), data_(data) {
            rw_mutex_.lock_shared();
        }

        ~ReadGuard() {
            rw_mutex_.unlock_shared();
        }

        const T& operator*() const {
            return *data_;
        }

        const T* operator->() const {
            return data_.get();
        }

    private:
        std::shared_ptr<T> data_;
        std::shared_mutex& rw_mutex_;
    };

    class WriteGuard {
    public:
        WriteGuard(std::shared_ptr<T> data, std::shared_mutex& rw_mutex) : rw_mutex_(rw_mutex), data_(data) {
            rw_mutex_.lock();
        }

        ~WriteGuard() {
            rw_mutex_.unlock();
        }

        T& operator*() {
            return *data_;
        }

        T* operator->() {
            return data_.get();
        }

    private:
        std::shared_ptr<T> data_;
        std::shared_mutex& rw_mutex_;
    };

    ReadGuard read() {
        return ReadGuard(data_, rw_mutex_);
    }

    WriteGuard write() {
        return WriteGuard(data_, rw_mutex_);
    }

private:
    std::shared_ptr<T> data_;
    mutable std::shared_mutex rw_mutex_;
};