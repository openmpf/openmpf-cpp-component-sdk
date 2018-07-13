/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2018 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2018 The MITRE Corporation                                       *
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


#ifndef SPSCBOUNDEDQUEUE_H
#define SPSCBOUNDEDQUEUE_H
#include <atomic>
#include <vector>
#include <iostream>

#include <opencv2/core.hpp>

namespace MPF { namespace COMPONENT {

// A single producer, single consumer circular queue. The type of
// objects to put into the queue must be default constructible, and
// move assignable.
template <typename T>
class SPSCBoundedQueue {
  public:

    SPSCBoundedQueue() : head_(0), tail_(0), capacity_(0) {}
    explicit SPSCBoundedQueue(size_t c) : head_(0), tail_(0), capacity_(c), buffer_(c) {}

    SPSCBoundedQueue(SPSCBoundedQueue&&) = delete;
    SPSCBoundedQueue(const SPSCBoundedQueue&) = delete;
    SPSCBoundedQueue& operator=(const SPSCBoundedQueue&) = delete;
    SPSCBoundedQueue& operator=(SPSCBoundedQueue&&) = delete;

    void Init(size_t capacity) {
        capacity_ = capacity;
        for (int i = 0; i < capacity_; i++) {
            buffer_.push_back(T());
        }
    }

    bool empty() {
        size_t current_tail = std::atomic_load(&tail_);
        size_t current_head = std::atomic_load(&head_);
        return(current_head == current_tail);
    }

    bool full() {
        size_t current_tail = std::atomic_load(&tail_);
        size_t current_head = std::atomic_load(&head_);
        return((current_head+1)%capacity_ == current_tail);
    }

    //Writes to the head of the circular buffer
    void push(T entry) {
        // The head is read-write here, but this function is the only
        // place where it is written, and since there is only one
        // producer thread, we can use relaxed memory order to read
        // it, but must use release memory order to write it.
        // We want to use acquire memory order here to read the tail
        // to ensure proper memory ordering with respect to the write
        // by the consumer.
        size_t current_head = std::atomic_load_explicit(&head_, std::memory_order_relaxed);
        size_t current_tail = std::atomic_load_explicit(&tail_, std::memory_order_acquire);

        // Move the entry into the buffer. Increment the head.
        buffer_[current_head] = std::move(entry);
        std::atomic_store_explicit(&head_, (current_head+1)%capacity_, std::memory_order_release);
        return;
    }

    // Reads from the tail of the buffer
    T pop() {
        T entry;
        size_t current_head;
        size_t current_tail;
        // The tail is read-write here, but this function is the only
        // place where it is written, and since there is only one
        // consumer thread, we can use relaxed memory order to read
        // it, but must use release memory order to write it.
        // We want to use acquire memory order here to read the head
        // to ensure proper memory ordering with respect to the write
        // by the producer.
        current_tail = std::atomic_load_explicit(&tail_, std::memory_order_relaxed);
        current_head = std::atomic_load_explicit(&head_, std::memory_order_acquire);

        // Move the tail entry out of the queue, and increment the tail index.
        entry = std::move(buffer_[current_tail]);
        std::atomic_store_explicit(&tail_, (current_tail+1)%capacity_, std::memory_order_release);
        return std::move(entry);
    }

  private:
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
    size_t capacity_;
    std::vector<T> buffer_;


};
}}


#endif
