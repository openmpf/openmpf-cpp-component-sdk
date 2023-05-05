/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2023 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2023 The MITRE Corporation                                       *
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
            : max_size_(max_size)
            , halt_(false)
            , adding_complete_(false)
    {
    }

    /**
     * Adds a copy of item to the queue. Blocks if queue is already full.
     * @param item object to be copied in to queue
     */
    void push(const T& item) {
        auto lock = acquire_lock();
        wait_until_can_add(lock);
        queue_.push(item);
        cond_.notify_all();
    }

    /**
     * Moves item in to the queue. Blocks if queue is already full.
     * @param item object to be moved in to queue
     */
    void push(T&& item) {
        auto lock = acquire_lock();
        wait_until_can_add(lock);
        queue_.push(std::move(item));
        cond_.notify_all();
    }

    /**
     * Constructs an instance of T in-place in the queue. Blocks if queue is already full.
     * @param args Arguments to be passed to T's constructor
     */
    template <typename... Args>
    void emplace(Args&&... args) {
        auto lock = acquire_lock();
        wait_until_can_add(lock);
        queue_.emplace(std::forward<Args>(args)...);
        cond_.notify_all();
    }


    /**
     * Moves an item out of the queue and returns it. Blocks if the queue is empty.
     * @return The object from the front of the queue
     */
    T pop() {
        auto lock = acquire_lock();
        wait_until_can_remove(lock);
        T result = std::move(queue_.front());
        queue_.pop();
        cond_.notify_all();
        return result;
    }

    /**
     * Indicates that both producers and consumers should stop processing even if there are items
     * in the queue. Any future adds or removes will cause a QueueHaltedException to be thrown.
     * This can be used to prevent one side from blocking indefinitely when the other side fails.
     */
    void halt() {
        auto lock = acquire_lock();
        halt_ = true;
        cond_.notify_all();
    }

    /**
     * Indicates that no more items will be added to the queue. Consumers should finish processing
     * items already in the queue. Any attempt to add items after this is called will cause a
     * QueueHaltedException to be thrown. This can be used to prevent consumers from blocking
     * indefinitely when a producer finishes.
     *
     * Loosely based on C#'s BlockingCollection<T>.CompleteAdding method
     * (https://docs.microsoft.com/en-us/dotnet/api/system.collections.concurrent.blockingcollection-1.completeadding?view=net-5.0#System_Collections_Concurrent_BlockingCollection_1_CompleteAdding).
     */
    void complete_adding() {
        auto lock = acquire_lock();
        adding_complete_ = true;
        cond_.notify_all();
    }

    /**
     * @return true if adding complete. Queue may or may not be empty.
     */
    bool is_adding_complete() {
        auto lock = acquire_lock();
        return halt_ || adding_complete_;
    }

    /**
     * The queue is "completed" if it has been halted or if a producer called complete_adding()
     * and the remaining items have all been removed from the queue.
     * @return true if items should not be added or removed from queue.
     */
    bool is_completed() {
        auto lock = acquire_lock();
        return halt_ || (adding_complete_ && queue_.empty());
    }


private:
    int max_size_;
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool halt_;
    bool adding_complete_;


    void wait_until_can_remove(std::unique_lock<std::mutex> &lock) {
        cond_.wait(lock, [this] { return halt_ || adding_complete_ || !queue_.empty(); });
        if (halt_ || (queue_.empty() && adding_complete_)) {
            throw QueueHaltedException();
        }
    }

    void wait_until_can_add(std::unique_lock<std::mutex> &lock) {
        if (max_size_ > 0) {
            cond_.wait(lock, [this] {
                return halt_ || adding_complete_ || queue_.size() < max_size_;
            });
        }
        if (halt_ || adding_complete_) {
            throw QueueHaltedException();
        }
    }

    std::unique_lock<std::mutex> acquire_lock() {
        return std::unique_lock<std::mutex>(mutex_);
    }
};
}}


#endif
