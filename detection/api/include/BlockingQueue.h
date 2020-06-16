/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2020 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2020 The MITRE Corporation                                       *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *    http://www.apache.org/licenses/LICENSE-2.0                              *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 ******************************************************************************/


#ifndef BLOCKING_QUEUE_H
#define BLOCKING_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>


namespace MPF { namespace COMPONENT {


class QueueHaltedException : public std::runtime_error {
public:
    QueueHaltedException()
            : std::runtime_error("Queue has been halted.")
    {
    }
};


template <typename T>
class BlockingQueue {

public:
    explicit BlockingQueue(int max_size=-1)
            : max_size_(max_size), halt_(false)
    {
    }

    void push(const T& item) {
        auto lock = acquire_lock();
        await_free_space(lock);
        queue_.push(item);
        cond_.notify_all();
    }

    void push(T&& item) {
        auto lock = acquire_lock();
        await_free_space(lock);
        queue_.push(std::move(item));
        cond_.notify_all();
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        auto lock = acquire_lock();
        await_free_space(lock);
        queue_.emplace(std::forward<Args>(args)...);
        cond_.notify_all();
    }

    T pop() {
        auto lock = acquire_lock();
        await_non_empty(lock);
        T result = std::move(queue_.front());
        queue_.pop();
        cond_.notify_all();
        return result;
    }

    void halt() {
        auto lock = acquire_lock();
        halt_ = true;
        cond_.notify_all();
    }

private:
    int max_size_;
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool halt_;

    void await_non_empty(std::unique_lock<std::mutex> &lock) {
        cond_.wait(lock, [this] { return (halt_ || !queue_.empty()); });
        if (halt_) {
            throw QueueHaltedException();
        }
    }

    void await_free_space(std::unique_lock<std::mutex> &lock) {
        if (max_size_ > 0) {
            cond_.wait(lock, [this] { return (halt_ || queue_.size() < max_size_); });
        }
        if (halt_) {
            throw QueueHaltedException();
        }
    }

    std::unique_lock<std::mutex> acquire_lock() {
        return std::unique_lock<std::mutex>(mutex_);
    }
};
}}


#endif
