/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2019 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2019 The MITRE Corporation                                       *
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

#include "frame_transformers/FrameTransformerFactory.h"

#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>

#include "detectionComponentUtils.h"
#include "frame_transformers/AffineFrameTransformer.h"
#include "frame_transformers/FrameCropper.h"
#include "frame_transformers/NoOpFrameTransformer.h"
#include "frame_transformers/IFrameTransformer.h"
#include "MPFDetectionObjects.h"



using DetectionComponentUtils::GetProperty;

namespace MPF { namespace COMPONENT { namespace FrameTransformerFactory {


namespace {

    int GetPercentOfDimension(const std::string &percentString, const int dimension) {
        float percentNum;
        std::istringstream(percentString) >> percentNum;
        if (percentNum < 0.0) {
            return 0;
        }
        if (percentNum > 100.0) {
            return dimension;
        }
        return static_cast<int>(percentNum * dimension / 100.0);
    }


    cv::Rect GetSearchRegion(const Properties &jobProperties, const cv::Size &frameSize) {

        int position;
        std::string topLeftX = GetProperty(jobProperties, "SEARCH_REGION_TOP_LEFT_X_DETECTION","-1");
        int regionTopLeftXPos;
        size_t idx = topLeftX.find('%');
        if (idx != std::string::npos) {
            regionTopLeftXPos = GetPercentOfDimension(topLeftX.substr(0, idx), frameSize.width);
        }
        else {
            std::istringstream(topLeftX) >> position;
            regionTopLeftXPos = (position < 0) ? 0 : position;
        }

        std::string topLeftY = GetProperty(jobProperties, "SEARCH_REGION_TOP_LEFT_Y_DETECTION", "-1");
        int regionTopLeftYPos;
        idx = topLeftY.find('%');
        if (idx != std::string::npos) {
            regionTopLeftYPos = GetPercentOfDimension(topLeftY.substr(0, idx), frameSize.height);
        }
        else {
            std::istringstream(topLeftY) >> position;
            regionTopLeftYPos = (position < 0) ? 0 : position;
        }

        cv::Point topLeft(regionTopLeftXPos, regionTopLeftYPos);

        std::string bottomRightX = GetProperty(jobProperties, "SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", "-1");
        int regionBottomRightXPos;
        idx = bottomRightX.find('%');
        if (idx != std::string::npos) {
            regionBottomRightXPos = GetPercentOfDimension(bottomRightX.substr(0, idx), frameSize.width);
        }
        else {
            std::istringstream(bottomRightX) >> position;
            regionBottomRightXPos = (position <= 0) ? frameSize.width : position;
        }

        std::string bottomRightY = GetProperty(jobProperties, "SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", "-1");
        int regionBottomRightYPos;
        idx = bottomRightY.find('%');
        if (idx != std::string::npos) {
            regionBottomRightYPos = GetPercentOfDimension(bottomRightY.substr(0, idx), frameSize.height);
        }
        else {
            std::istringstream(bottomRightY) >> position;
            regionBottomRightYPos = (position <= 0) ? frameSize.height : position;
        }

        cv::Point bottomRight(regionBottomRightXPos, regionBottomRightYPos);

        return cv::Rect(topLeft, bottomRight);
    }


    cv::Rect ToRect(const MPFImageLocation &imageLocation) {
        return {imageLocation.x_left_upper, imageLocation.y_left_upper, imageLocation.width, imageLocation.height};
    }




    bool FeedForwardSupersetRegionIsEnabled(const Properties &jobProperties) {
        return "SUPERSET_REGION" ==
                DetectionComponentUtils::GetProperty(jobProperties, "FEED_FORWARD_TYPE", "");
    }


    bool FeedForwardExactRegionIsEnabled(const Properties &jobProperties) {
        return "REGION" == DetectionComponentUtils::GetProperty(jobProperties, "FEED_FORWARD_TYPE", "");
    }


    bool FeedForwardIsEnabled(const Properties &jobProperties) {
        return FeedForwardSupersetRegionIsEnabled(jobProperties) || FeedForwardExactRegionIsEnabled(jobProperties);
    }

    bool SearchRegionCroppingIsEnabled(const Properties &jobProperties) {
        return DetectionComponentUtils::GetProperty(jobProperties, "SEARCH_REGION_ENABLE_DETECTION", false);
    }



    void AddTransformersIfNeeded(const Properties &jobProperties, const Properties &mediaProperties,
                                 const cv::Size &inputVideoSize, IFrameTransformer::Ptr &currentTransformer) {
        static const std::string rotationKey = "ROTATION";

        double rotation = 0;
        if (DetectionComponentUtils::GetProperty(jobProperties, "AUTO_ROTATE", false)) {
            rotation = DetectionComponentUtils::GetProperty(mediaProperties, rotationKey, 0.0);
        }
        else {
            rotation = DetectionComponentUtils::GetProperty(jobProperties, rotationKey, 0.0);
        }
        rotation = DetectionComponentUtils::NormalizeAngle(rotation);
        bool rotationRequired = !DetectionComponentUtils::RotationAnglesEqual(rotation, 0);

        bool flipRequired;
        if (DetectionComponentUtils::GetProperty(jobProperties, "AUTO_FLIP", false)) {
            flipRequired = DetectionComponentUtils::GetProperty(mediaProperties, "HORIZONTAL_FLIP", false);
        }
        else {
            flipRequired = DetectionComponentUtils::GetProperty(jobProperties, "HORIZONTAL_FLIP", false);
        }

        bool cropRequired;
        cv::Rect region;
        cv::Rect frameRect(cv::Point(0, 0), inputVideoSize);
        if (SearchRegionCroppingIsEnabled(jobProperties)) {
            region = GetSearchRegion(jobProperties, inputVideoSize);
            cropRequired = region != frameRect;
        }
        else {
            region = frameRect;
            cropRequired = false;
        }

        if (!rotationRequired && !flipRequired && !cropRequired) {
            return;
        }


        if (cropRequired) {
            if (flipRequired || rotationRequired) {
                currentTransformer = IFrameTransformer::Ptr(
                        new AffineFrameTransformer(region, rotation, flipRequired, std::move(currentTransformer)));
            }
            else {
                currentTransformer = IFrameTransformer::Ptr(
                        new SearchRegionFrameCropper(region, std::move(currentTransformer)));
            }
        }
        else {
            currentTransformer = IFrameTransformer::Ptr(
                    new AffineFrameTransformer(rotation, flipRequired, std::move(currentTransformer)));
        }
    }



    cv::Rect GetSupersetRegionNoRotation(const std::vector<std::tuple<cv::Rect, double, bool>> &regions) {
        if (regions.empty()) {
            throw std::length_error(
                    "FEED_FORWARD_TYPE: SUPERSET_REGION is enabled, but feed forward track was empty.");
        }

        auto it = regions.begin();
        cv::Rect region = std::get<0>(*it);
        ++it;

        for (; it != regions.end(); ++it) {
            region |= std::get<0>(*it);
        }
        return region;
    }


    void AddFeedForwardTransformsIfNeeded(const Properties &jobProperties, const Properties &mediaProperties,
                                          const Properties &trackProperties,
                                          const std::map<int, MPFImageLocation> &detections,
                                          IFrameTransformer::Ptr &currentTransformer) {
        if (SearchRegionCroppingIsEnabled(jobProperties)) {
            std::cerr << "Both feed forward cropping and search region cropping properties were provided. "
                      << "Only feed forward cropping will occur." << std::endl;
        }

        bool hasJobLevelRotation;
        double jobLevelRotation = 0;
        {
            bool autoRotate = DetectionComponentUtils::GetProperty(jobProperties, "AUTO_ROTATE", false);
            const auto &props = autoRotate ? mediaProperties : jobProperties;
            auto rotationIter = props.find("ROTATION");
            hasJobLevelRotation = rotationIter != props.end();
            if (hasJobLevelRotation) {
                jobLevelRotation = DetectionComponentUtils::NormalizeAngle(std::stod(rotationIter->second));
            }
        }

        bool hasJobLevelFlip;
        bool jobLevelFlip = false;
        {
            bool autoFlip = DetectionComponentUtils::GetProperty(jobProperties, "AUTO_FLIP", false);
            const auto &props = autoFlip ? mediaProperties : jobProperties;
            hasJobLevelFlip = props.count("HORIZONTAL_FLIP") == 1;
            if (hasJobLevelFlip) {
                jobLevelFlip = DetectionComponentUtils::GetProperty(props, "HORIZONTAL_FLIP", false);
            }
        }


        bool hasTrackLevelRotation;
        double trackRotation = 0;
        {
            auto trackRotationIter = trackProperties.find("ROTATION");
            hasTrackLevelRotation = trackRotationIter != trackProperties.end();
            if (hasTrackLevelRotation) {
                trackRotation = DetectionComponentUtils::NormalizeAngle(std::stod(trackRotationIter->second));
            }
        }

        bool hasTrackLevelFlip;
        bool trackLevelFlip = false;
        {
            hasTrackLevelFlip = trackProperties.count("HORIZONTAL_FLIP") == 1;
            if (hasTrackLevelFlip) {
                trackLevelFlip = DetectionComponentUtils::GetProperty(trackProperties, "HORIZONTAL_FLIP", false);
            }
        }

        bool requiresRotationOrFlip = false;
        bool isExactRegionMode = FeedForwardExactRegionIsEnabled(jobProperties);

        std::vector<std::tuple<cv::Rect, double, bool>> regions;
        regions.reserve(detections.size());

        for (const auto &detectionPair : detections) {
            const auto &detection = detectionPair.second;
            auto detectionRotationIter = detection.detection_properties.find("ROTATION");
            bool hasDetectionLevelRotation = detectionRotationIter != detection.detection_properties.end();

            double rotation = 0;
            if (hasDetectionLevelRotation) {
                rotation = DetectionComponentUtils::NormalizeAngle(std::stod(detectionRotationIter->second));
            }
            else if (hasTrackLevelRotation) {
                rotation = trackRotation;
            }
            else if (hasJobLevelRotation && isExactRegionMode) {
                rotation = jobLevelRotation;
            }

            bool hasDetectionLevelFlip = detection.detection_properties.count("HORIZONTAL_FLIP");

            bool flip = false;
            if (hasDetectionLevelFlip) {
                flip = DetectionComponentUtils::GetProperty(detection.detection_properties, "HORIZONTAL_FLIP", false);
            }
            else if (hasTrackLevelFlip) {
                flip = trackLevelFlip;
            }
            else if (hasJobLevelFlip && isExactRegionMode) {
                flip = jobLevelFlip;
            }

            if (flip || !DetectionComponentUtils::RotationAnglesEqual(0, rotation)) {
                requiresRotationOrFlip = true;
            }

            regions.emplace_back(ToRect(detection), rotation, flip);
        }

        if (isExactRegionMode) {
            if (requiresRotationOrFlip) {
                currentTransformer = IFrameTransformer::Ptr(
                        new FeedForwardExactRegionAffineTransformer(regions, std::move(currentTransformer)));
            }
            else {
                currentTransformer = IFrameTransformer::Ptr(
                        new FeedForwardFrameCropper(detections, std::move(currentTransformer)));
            }
        }
        else {
            if (requiresRotationOrFlip) {
                currentTransformer = IFrameTransformer::Ptr(
                        new AffineFrameTransformer(regions, jobLevelRotation, jobLevelFlip,
                                                   std::move(currentTransformer)));
            }
            else {
                cv::Rect supersetRegion = GetSupersetRegionNoRotation(regions);
                currentTransformer = IFrameTransformer::Ptr(
                        new SearchRegionFrameCropper(supersetRegion, std::move(currentTransformer)));
            }
        }
    }


    IFrameTransformer::Ptr GetTransformer(const MPFJob &job, const cv::Size &inputVideoSize,
                                          const std::map<int, MPFImageLocation> &trackLocations,
                                          const Properties &trackProperties = {}) {

        IFrameTransformer::Ptr transformer(new NoOpFrameTransformer(inputVideoSize));

        if (FeedForwardIsEnabled(job.job_properties)) {
            if (trackLocations.empty()) {
                throw std::length_error(
                        "Feed forward is enabled, but feed forward track was empty.");
            }
            AddFeedForwardTransformsIfNeeded(job.job_properties, job.media_properties,
                                             trackProperties, trackLocations, transformer);
        }
        else {
            AddTransformersIfNeeded(job.job_properties, job.media_properties, inputVideoSize, transformer);
        }

        return transformer;
    }
} // End anonymous namespace



IFrameTransformer::Ptr GetTransformer(const MPFVideoJob &job, const cv::Size &inputVideoSize) {
    return GetTransformer(job, inputVideoSize, job.feed_forward_track.frame_locations,
                          job.feed_forward_track.detection_properties);
}


IFrameTransformer::Ptr GetTransformer(const MPFImageJob &job, const cv::Size &inputVideoSize) {
    return GetTransformer(job, inputVideoSize, { { 0, job.feed_forward_location } });
}


IFrameTransformer::Ptr GetTransformer(const MPFStreamingVideoJob &job, const cv::Size &inputVideoSize) {

    IFrameTransformer::Ptr transformer(new NoOpFrameTransformer(inputVideoSize));
    AddTransformersIfNeeded(job.job_properties, job.media_properties, inputVideoSize, transformer);
    return transformer;
}
}}} // End MPF::COMPONENT::FrameTransformerFactory
