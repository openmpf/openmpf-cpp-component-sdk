/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2017 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2017 The MITRE Corporation                                       *
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


#ifndef OPENMPF_CPP_COMPONENT_SDK_HELLOWORLD_H
#define OPENMPF_CPP_COMPONENT_SDK_HELLOWORLD_H


#include <string>
#include <MPFDetectionComponent.h>

using namespace MPF;
using namespace COMPONENT;

class HelloWorld : public MPFDetectionComponent {

public:

    bool Init() override;

    bool Close() override;

    MPFDetectionError GetDetections(const MPFVideoJob &job,
                                    std::vector<MPFVideoTrack> &tracks) override;

    MPFDetectionError GetDetections(const MPFImageJob &job,
                                    std::vector<MPFImageLocation> &locations) override;

    MPFDetectionError GetDetections(const MPFAudioJob &job,
                                    std::vector<MPFAudioTrack> &tracks) override;

    MPFDetectionError GetDetections(const MPFGenericJob &job,
                                    std::vector <MPFGenericTrack> &tracks) override;

    bool Supports(MPFDetectionDataType data_type) override;

    std::string GetDetectionType() override;
};


#endif //OPENMPF_CPP_COMPONENT_SDK_HELLOWORLD_H
