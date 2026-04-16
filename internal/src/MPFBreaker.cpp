/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2026 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2026 The MITRE Corporation                                       *
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

#include "MPFBreaker.h"

#include <atomic>
#include <iostream>
#include <string>
#include <exception>

namespace MPF::COMPONENT {
    namespace {
        std::atomic<bool> STOP_REQUESTED{false};
    }


    void MPFBreaker::requestStop() {
        std::cerr << "MPFBreaker: Setting stop requested flag to true.\n";
        STOP_REQUESTED.store(true, std::memory_order::memory_order_release);
    }

    bool MPFBreaker::stopHasBeenRequested() {
        return STOP_REQUESTED.load(std::memory_order::memory_order_acquire);
    }


    void MPFBreaker::check() {
        if (!stopHasBeenRequested()) {
            return;
        }

        auto e_ptr = std::current_exception();
        if (!e_ptr) {
            throw StopRequestedException{"Stop requested."};
        }

        try {
            std::rethrow_exception(e_ptr);
        }
        catch (const StopRequestedException&) {
            throw;
        }
        catch (const std::exception& e) {
            throw StopRequestedException{
                std::string{"Stop requested. Suppressed the following exception: "}
                + e.what()};
        }
        catch (...) {
            throw StopRequestedException{
                "Stop requested. Suppressed a thrown non-exception object."};
        }
    }
} // namespace MPF::COMPONENT
