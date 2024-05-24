/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2024 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2024 The MITRE Corporation                                       *
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

#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>

#include <opencv2/core.hpp>

#include "detectionComponentUtils.h"
#include "frame_transformers/AffineFrameTransformer.h"
#include "frame_transformers/FrameCropper.h"
#include "frame_transformers/NoOpFrameTransformer.h"
#include "frame_transformers/IFrameTransformer.h"
#include "frame_transformers/SearchRegion.h"
#include "MPFDetectionException.h"
#include "MPFDetectionObjects.h"
#include "MPFRotatedRect.h"



using DetectionComponentUtils::GetProperty;

namespace MPF::COMPONENT::FrameTransformerFactory {


namespace {

    bool SearchRegionCroppingIsEnabled(const Properties &jobProperties) {
        return DetectionComponentUtils::GetProperty(jobProperties, "SEARCH_REGION_ENABLE_DETECTION", false);
    }


    RegionEdge::resolve_region_edge_t GetRegionEdge(const Properties &props, const std::string& property) {
        try {
            std::string propVal = GetProperty(props, property, "-1");
            if (propVal.find('%') != std::string::npos) {
                return RegionEdge::Percentage(std::stod(propVal));
            }
            return RegionEdge::Absolute(std::stoi(propVal));
        }
        catch (const std::invalid_argument&) {
            // Failed to convert property to number
            return RegionEdge::Default();
        }
    }


    SearchRegion GetSearchRegion(const Properties &props) {
        if (!SearchRegionCroppingIsEnabled(props)) {
            return { };
        }
        return {
                GetRegionEdge(props, "SEARCH_REGION_TOP_LEFT_X_DETECTION"),
                GetRegionEdge(props, "SEARCH_REGION_TOP_LEFT_Y_DETECTION"),
                GetRegionEdge(props, "SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION"),
                GetRegionEdge(props, "SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION"),
        };

    }


    cv::Rect ToRect(const MPFImageLocation &imageLocation) {
        return {imageLocation.x_left_upper, imageLocation.y_left_upper, imageLocation.width, imageLocation.height};
    }


    bool FeedForwardSupersetRegionIsEnabled(const Properties &jobProperties) {
        return boost::iequals("SUPERSET_REGION",
                              GetProperty(jobProperties, "FEED_FORWARD_TYPE", ""));
    }


    bool FeedForwardExactRegionIsEnabled(const Properties &jobProperties) {
        return boost::iequals("REGION",
                              GetProperty(jobProperties, "FEED_FORWARD_TYPE", ""));
    }


    bool FeedForwardRegionIsEnabled(const Properties &jobProperties) {
        return FeedForwardSupersetRegionIsEnabled(jobProperties) || FeedForwardExactRegionIsEnabled(jobProperties);
    }


    cv::Scalar GetFillColor(const Properties &props) {
        auto fillColorName = GetProperty(props, "ROTATION_FILL_COLOR", "BLACK");
        if (boost::iequals("BLACK", fillColorName)) {
            return {0, 0, 0};
        }
        else if (boost::iequals("WHITE", fillColorName)) {
            return {255, 255, 255};
        }
        else {
            throw MPFDetectionException(
                    MPFDetectionError::MPF_INVALID_PROPERTY,
                    "Expected the \"ROTATION_FILL_COLOR\" property to be either \"BLACK\""
                    " or \"WHITE\", but it was set to \"" + fillColorName + "\".");
        }
    }


    std::optional<double> GetJobRotation(const Properties &jobProperties,
                                         const Properties &mediaProperties) {
        if (auto optJobRotation = GetProperty<double>(jobProperties, "ROTATION");
                    optJobRotation) {
            return optJobRotation;
        }
        else if (GetProperty(jobProperties, "AUTO_ROTATE", true)) {
            return GetProperty<double>(mediaProperties, "ROTATION");
        }
        else {
            return {};
        }
    }

    std::optional<bool> GetJobFlip(const Properties &jobProperties,
                                   const Properties &mediaProperties) {
        if (auto optFlip = GetProperty<bool>(jobProperties, "HORIZONTAL_FLIP");
                optFlip) {
            return optFlip;
        }
        else if (GetProperty(jobProperties, "AUTO_FLIP", true)) {
            return GetProperty<bool>(mediaProperties, "HORIZONTAL_FLIP");
        }
        else {
            return {};
        }
    }


    void AddTransformersIfNeeded(const Properties &jobProperties, const Properties &mediaProperties,
                                 const cv::Size &inputVideoSize, IFrameTransformer::Ptr &currentTransformer) {
        double rotation = GetJobRotation(jobProperties, mediaProperties).value_or(0);

        double rotationThreshold = GetProperty(jobProperties, "ROTATION_THRESHOLD", 0.1);
        bool rotationRequired = !DetectionComponentUtils::RotationAnglesEqual(
                rotation, 0, rotationThreshold);
        if (!rotationRequired) {
            rotation = 0;
        }

        bool flipRequired = GetJobFlip(jobProperties, mediaProperties).value_or(false);

        SearchRegion searchRegion = GetSearchRegion(jobProperties);

        if (rotationRequired || flipRequired) {
            currentTransformer = IFrameTransformer::Ptr(
                    new AffineFrameTransformer(rotation, flipRequired, GetFillColor(jobProperties),
                                               searchRegion, std::move(currentTransformer)));
        }
        else {
            cv::Rect frameRect(cv::Point(0, 0), inputVideoSize);
            cv::Rect searchRegionRect = searchRegion.GetRect(inputVideoSize);
            if (frameRect != searchRegionRect) {
                currentTransformer = IFrameTransformer::Ptr(
                        new SearchRegionFrameCropper(searchRegionRect, std::move(currentTransformer)));
            }
        }
    }


    cv::Rect GetSupersetRegionNoRotation(const std::vector<MPFRotatedRect> &regions) {
        if (regions.empty()) {
            throw std::length_error(
                    "FEED_FORWARD_TYPE: SUPERSET_REGION is enabled, but feed forward track was empty.");
        }

        auto it = regions.begin();
        cv::Rect2d region = it->GetBoundingRect();
        ++it;

        for (; it != regions.end(); ++it) {
            region |= it->GetBoundingRect();
        }
        return region;
    }


    void AddFeedForwardRegionTransformersIfNeeded(const Properties &jobProperties, const Properties &mediaProperties,
                                                  const Properties &trackProperties,
                                                  const std::map<int, MPFImageLocation> &detections,
                                                  IFrameTransformer::Ptr &currentTransformer) {
        if (SearchRegionCroppingIsEnabled(jobProperties)) {
            std::cerr << "Both feed forward cropping and search region cropping properties were provided. "
                      << "Only feed forward cropping will occur." << std::endl;
        }

        bool hasJobLevelRotation = false;
        double jobLevelRotation = 0;
        if (auto optRotation = GetJobRotation(jobProperties, mediaProperties); optRotation) {
            hasJobLevelRotation = true;
            jobLevelRotation = *optRotation;
        }

        bool hasJobLevelFlip = false;
        bool jobLevelFlip = false;
        if (auto optFlip = GetJobFlip(jobProperties, mediaProperties); optFlip) {
            hasJobLevelFlip = true;
            jobLevelFlip = *optFlip;
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

        double rotationThreshold = DetectionComponentUtils::GetProperty(
                jobProperties, "ROTATION_THRESHOLD", 0.1);
        bool anyDetectionRequiresRotationOrFlip = false;
        bool isExactRegionMode = FeedForwardExactRegionIsEnabled(jobProperties);

        std::vector<MPFRotatedRect> regions;
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
            bool currentDetectionRequiresRotation = !DetectionComponentUtils::RotationAnglesEqual(
                    rotation, 0, rotationThreshold);
            if (!currentDetectionRequiresRotation) {
                rotation = 0;
            }


            bool hasDetectionLevelFlip = detection.detection_properties.count("HORIZONTAL_FLIP");

            bool currentDetectionRequiresFlip = false;
            if (hasDetectionLevelFlip) {
                currentDetectionRequiresFlip = DetectionComponentUtils::GetProperty(
                        detection.detection_properties, "HORIZONTAL_FLIP", false);
            }
            else if (hasTrackLevelFlip) {
                currentDetectionRequiresFlip = trackLevelFlip;
            }
            else if (hasJobLevelFlip && isExactRegionMode) {
                currentDetectionRequiresFlip = jobLevelFlip;
            }

            if (currentDetectionRequiresFlip || currentDetectionRequiresRotation) {
                anyDetectionRequiresRotationOrFlip = true;
            }

            regions.emplace_back(
                    detection.x_left_upper, detection.y_left_upper, detection.width, detection.height,
                    rotation, currentDetectionRequiresFlip);
        }

        if (isExactRegionMode) {
            if (anyDetectionRequiresRotationOrFlip) {
                currentTransformer = IFrameTransformer::Ptr(
                        new FeedForwardExactRegionAffineTransformer(
                                regions, GetFillColor(jobProperties),
                                std::move(currentTransformer)));
            }
            else {
                currentTransformer = IFrameTransformer::Ptr(
                        new FeedForwardFrameCropper(detections, std::move(currentTransformer)));
            }
        }
        else {
            if (anyDetectionRequiresRotationOrFlip) {
                currentTransformer = IFrameTransformer::Ptr(
                        new AffineFrameTransformer(regions, jobLevelRotation, jobLevelFlip,
                                                   GetFillColor(jobProperties),
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

        if (FeedForwardRegionIsEnabled(job.job_properties)) {
            if (trackLocations.empty()) {
                throw std::length_error(
                        "Feed forward is enabled, but feed forward track was empty.");
            }
            AddFeedForwardRegionTransformersIfNeeded(job.job_properties, job.media_properties,
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
} // End MPF::COMPONENT::FrameTransformerFactory
