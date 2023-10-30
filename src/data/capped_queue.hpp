#pragma once
#include <queue>
#include <vector>
#include <Geode/Geode.hpp>

/*
* CappedQueue is a simple queue but when the size exceeds a limit, it removes the oldest element
*/

template <typename T, size_t MaxSize>
class CappedQueue {
public:
    void push(const T& element) {
        if (queue.size() >= MaxSize) {
            queue.pop();
        }

        queue.push(element);
    }

    size_t size() const {
        return queue.size();
    }

    bool empty() const {
        return queue.empty();
    }

    void clear() {
        queue = {}; // queue does not have .clear() for whatever funny reason
    }

    T front() const {
        return queue.front();
    }

    T back() const {
        return queue.back();
    }

    std::vector<T> extract() const {
        std::vector<T> out(queue.size());

        std::queue<T> qcopy(queue);

        while (!qcopy.empty()) {
            out.push_back(qcopy.front());
            qcopy.pop();
        }

        return out;
    }
protected:
    std::queue<T> queue;
};

/*
* CCCappedQueue is CappedQueue but for CCObjects, upon removal
* the removeFromParent() method gets called on the object.
*/

template <size_t MaxSize>
class CCCappedQueue : public CappedQueue<cocos2d::CCObject*, MaxSize> {
public:
    void push(cocos2d::CCObject* element) {
        if (this->queue.size() >= MaxSize) {
            cocos2d::CCObject* obj = this->queue.front();
            obj->release();
            if (cocos2d::CCNode* node = dynamic_cast<cocos2d::CCNode*>(obj)) {
                node->removeFromParent();
            }

            this->queue.pop();
        }

        element->retain();
        this->queue.push(element);
    }

    template <typename Obj>
    Obj* front() const {
        return static_cast<Obj*>(this->front());
    }

    template <typename Obj>
    Obj* back() const {
        return static_cast<Obj*>(this->back());
    }

    template <typename Obj>
    std::vector<Obj*> extract() const {
        std::vector<Obj*> out;

        std::queue<cocos2d::CCObject*> qcopy(this->queue);

        while (!qcopy.empty()) {
            out.push_back(static_cast<Obj*>(qcopy.front()));
            qcopy.pop();
        }
        return out;
    }
};