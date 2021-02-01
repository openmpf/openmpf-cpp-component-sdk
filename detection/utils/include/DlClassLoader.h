/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2021 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2021 The MITRE Corporation                                       *
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

#ifndef OPENMPF_CPP_COMPONENT_SDK_DLCLASSLOADER_H
#define OPENMPF_CPP_COMPONENT_SDK_DLCLASSLOADER_H

#include <dlfcn.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <MPFDetectionException.h>


namespace MPF { namespace COMPONENT {

    template<typename... Ts>
    struct all_standard_layout;

    template<typename Head, typename... Tail>
    struct all_standard_layout<Head, Tail...> {
        static const bool value = std::is_standard_layout<Head>::value && all_standard_layout<Tail...>::value;
    };

    template<>
    struct all_standard_layout<> {
        static const bool value = true;
    };


    class DlError : public MPFDetectionException {
    public:
        explicit DlError(const std::string &what)
            : MPFDetectionException(MPF_DETECTION_NOT_INITIALIZED, what) {
        }
    };


    template <typename DlClass>
    class DlClassLoader {
    public:

        template <typename... CreatorArgs>
        DlClassLoader(
                const std::string &lib_path,
                const std::string &creator_func_name,
                const std::string &deleter_func_name,
                CreatorArgs&&... creator_args)
            : lib_handle_(dlopen(lib_path.c_str(), RTLD_NOW), dlclose)
            , instance_(LoadClass(lib_handle_.get(), lib_path, creator_func_name, deleter_func_name,
                                  std::forward<CreatorArgs>(creator_args)...))
        {
            static_assert(all_standard_layout<typename std::decay<CreatorArgs>::type...>::value,
                          "One or more creator_args do not use standard layout so they cannot be used with a function loaded using dlopen.");
        }

        DlClass* operator->() const noexcept {
            return instance_.operator->();
        }


    private:
        using instance_ptr_t = std::unique_ptr<DlClass, void(*)(DlClass*)>;

        std::unique_ptr<void, decltype(&dlclose)> lib_handle_;

        instance_ptr_t instance_;

        // std::decay is used here to convert references to their non-reference type. We can't pass references
        // to functions loaded through dlopen, because the functions are declared with C linkage (extern "C").
        // References cannot be passed to functions with C linkage.
        template <typename... Args>
        using creator_func_t = DlClass*(typename std::decay<Args>::type...);


        template <typename... CreatorArgs>
        static instance_ptr_t LoadClass(
                    void* lib_handle,
                    const std::string &lib_path,
                    const std::string &creator_func_name,
                    const std::string &deleter_func_name,
                    CreatorArgs&&... args) {
            if (lib_handle == nullptr) {
                throw DlError("Failed to open \"" + lib_path + "\" due to: " + dlerror());
            }

            auto create_instance_fn = LoadFunction<creator_func_t<CreatorArgs...>>(lib_handle, creator_func_name);
            auto delete_instance_fn = LoadFunction<void(DlClass*)>(lib_handle, deleter_func_name);

            instance_ptr_t instance(nullptr, delete_instance_fn);
            try {
                instance = { create_instance_fn(std::forward<CreatorArgs>(args)...), delete_instance_fn };
            }
            catch (const MPFDetectionException &ex) {
                throw ex;
            }
            catch (const std::exception &ex) {
                throw std::runtime_error(
                        "Failed to load class from \"" + lib_path + "\" because the creator function, \""
                        + creator_func_name + "\", threw an exception: " + ex.what());
            }

            if (instance == nullptr) {
                throw std::runtime_error(
                        "Failed to load class from \"" + lib_path + "\" because the creator function, \""
                        + creator_func_name + "\", returned null.");
            }
            return instance;
        }


        template <typename TFunc>
        static TFunc* LoadFunction(void* lib_handle, const std::string &symbol_name) {
            auto result = reinterpret_cast<TFunc*>(dlsym(lib_handle, symbol_name.c_str()));
            if (result == nullptr) {
                throw DlError("dlsym failed for " + symbol_name + ": " + dlerror());
            }
            return result;
        }
    };
}}



#endif //OPENMPF_CPP_COMPONENT_SDK_DLCLASSLOADER_H
