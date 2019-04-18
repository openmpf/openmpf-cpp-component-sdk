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

#include <sstream>
#include <string>
#include <utility>

#include "detectionComponentUtils.h"
#include "frame_transformers/FrameCropper.h"
#include "frame_transformers/FrameFlipper.h"
#include "frame_transformers/FrameRotator.h"
#include "frame_transformers/NoOpFrameTransformer.h"
#include "MPFInvalidPropertyException.h"

#include "frame_transformers/FrameTransformerFactory.h"


using std::string;
using DetectionComponentUtils::GetProperty;

namespace MPF { namespace COMPONENT { namespace FrameTransformerFactory {

namespace {

int GetPercentOfDimension(const std::string &percentString, const int dimension) {

    float percentNum;
    std::istringstream(percentString) >> percentNum;
    if (percentNum < 0.0) {
        return 0;
    }
    else if (percentNum > 100.0) {
        return dimension;
    }
    return (percentNum*dimension)/100.0;
}


cv::Rect GetSearchRegion(const Properties &jobProperties, const cv::Size &frameSize) {

    int position;
    string topLeftX = GetProperty(jobProperties, "SEARCH_REGION_TOP_LEFT_X_DETECTION", string("-1"));
    int regionTopLeftXPos;
    size_t idx = topLeftX.find("%");
    if (idx != string::npos) {
        regionTopLeftXPos = GetPercentOfDimension(topLeftX.substr(0, idx), frameSize.width);
    }
    else {
        std::istringstream(topLeftX) >> position;
        regionTopLeftXPos = (position < 0) ? 0 : position;
    }

    string topLeftY = GetProperty(jobProperties, "SEARCH_REGION_TOP_LEFT_Y_DETECTION", string("-1"));
    int regionTopLeftYPos;
    idx = topLeftY.find("%");
    if (idx != string::npos) {
        regionTopLeftYPos = GetPercentOfDimension(topLeftY.substr(0, idx), frameSize.height);
    }
    else {
        std::istringstream(topLeftY) >> position;
        regionTopLeftYPos = (position < 0) ? 0 : position;
    }

    cv::Point topLeft(regionTopLeftXPos, regionTopLeftYPos);

    string bottomRightX = GetProperty(jobProperties, "SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", string("-1"));
    int regionBottomRightXPos;
    idx = bottomRightX.find("%");
    if (idx != string::npos) {
        regionBottomRightXPos = GetPercentOfDimension(bottomRightX.substr(0, idx), frameSize.width);
    }
    else {
        std::istringstream(bottomRightX) >> position;
        regionBottomRightXPos = (position <= 0) ? frameSize.width : position;
    }

    string bottomRightY = GetProperty(jobProperties, "SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", string("-1"));
    int regionBottomRightYPos;
    idx = bottomRightY.find("%");
    if (idx != string::npos) {
        regionBottomRightYPos = GetPercentOfDimension(bottomRightY.substr(0, idx), frameSize.height);
    }
    else {
        std::istringstream(bottomRightY) >> position;
        regionBottomRightYPos = (position <= 0) ? frameSize.height : position;
    }

    cv::Point bottomRight(regionBottomRightXPos, regionBottomRightYPos);

    return cv::Rect(topLeft, bottomRight);
}


cv::Rect toRect(const MPFImageLocation &imageLocation) {
    return {imageLocation.x_left_upper, imageLocation.y_left_upper, imageLocation.width, imageLocation.height};
}


/**
 *
 * @param feedForwardTrackLocations
 * @return The minimum area rectangle that contains all detections listed in feedForwardTrackLocations
 */
cv::Rect GetSupersetRegion(const std::map<int, MPFImageLocation> &feedForwardTrackLocations) {
    if (feedForwardTrackLocations.empty()) {
        throw std::length_error(
            "FEED_FORWARD_TYPE: SUPERSET_REGION is enabled, but feed forward track was empty.");
    }

    auto it = feedForwardTrackLocations.begin();
    cv::Rect region = toRect(it->second);
    it++;

    for (; it != feedForwardTrackLocations.end(); it++) {
        region |= toRect(it->second);
    }
    return region;
}


bool FeedForwardSupersetRegionIsEnabled(const Properties &jobProperties) {
    return "SUPERSET_REGION" ==
            DetectionComponentUtils::GetProperty(jobProperties, "FEED_FORWARD_TYPE", string());
}


bool FeedForwardExactRegionIsEnabled(const Properties &jobProperties) {
    return "REGION" == DetectionComponentUtils::GetProperty(jobProperties, "FEED_FORWARD_TYPE", string());
}


bool SearchRegionCroppingIsEnabled(const Properties &jobProperties) {
    return DetectionComponentUtils::GetProperty(jobProperties, "SEARCH_REGION_ENABLE_DETECTION", false);
}


void AddFlipperIfNeeded(const Properties &jobProperties, const Properties &mediaProperties,
                        IFrameTransformer::Ptr &currentTransformer) {
    bool shouldFlip;
    if (DetectionComponentUtils::GetProperty(jobProperties, "AUTO_FLIP", false)) {
        shouldFlip = DetectionComponentUtils::GetProperty(mediaProperties, "HORIZONTAL_FLIP", false);
    }
    else {
        shouldFlip = DetectionComponentUtils::GetProperty(jobProperties, "HORIZONTAL_FLIP", false);
    }

    if (shouldFlip) {
        currentTransformer = IFrameTransformer::Ptr(new FrameFlipper(std::move(currentTransformer)));
    }
}


void AddRotatorIfNeeded(const Properties &jobProperties, const Properties &mediaProperties,
                        IFrameTransformer::Ptr &currentTransformer) {
    string rotationKey = "ROTATION";
    int rotation = 0;
    if (DetectionComponentUtils::GetProperty(jobProperties, "AUTO_ROTATE", false)) {
        rotation = DetectionComponentUtils::GetProperty(mediaProperties, rotationKey, 0);
    }
    else {
        rotation = DetectionComponentUtils::GetProperty(jobProperties, rotationKey, 0);
    }

    if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270) {
        throw MPFInvalidPropertyException(rotationKey,
                                          "Rotation degrees must be 0, 90, 180, or 270.",
                                          MPF_INVALID_ROTATION);
    }

    if (rotation != 0) {
        currentTransformer = IFrameTransformer::Ptr(new FrameRotator(std::move(currentTransformer), rotation));
    }
}


void AddCropperIfNeeded(const cv::Rect &regionOfInterest, const cv::Size &inputVideoSize,
                        IFrameTransformer::Ptr &currentTransformer) {

    bool regionOfInterestIsEntireFrame = cv::Rect({0, 0}, inputVideoSize) == regionOfInterest;
    if (!regionOfInterestIsEntireFrame) {
        currentTransformer = IFrameTransformer::Ptr(
            new SearchRegionFrameCropper(std::move(currentTransformer), regionOfInterest));
    }
}


void AddCropperIfNeeded(const MPFJob &job, const cv::Size &inputVideoSize,
                        const std::map<int, MPFImageLocation> &trackLocations,
                        IFrameTransformer::Ptr &currentTransformer) {

    bool exactRegionCroppingEnabled = FeedForwardExactRegionIsEnabled(job.job_properties);
    bool supersetRegionCroppingEnabled = FeedForwardSupersetRegionIsEnabled(job.job_properties);
    bool searchRegionCroppingEnabled = SearchRegionCroppingIsEnabled(job.job_properties);

    if (!exactRegionCroppingEnabled && !supersetRegionCroppingEnabled && !searchRegionCroppingEnabled) {
        return;
    }

    if ((exactRegionCroppingEnabled || supersetRegionCroppingEnabled) && searchRegionCroppingEnabled) {
        std::cerr << "Both feed forward cropping and search region cropping properties were provided. "
                  << "Only feed forward cropping will occur." << std::endl;
    }


    if (exactRegionCroppingEnabled) {
        currentTransformer = IFrameTransformer::Ptr(
            new FeedForwardFrameCropper(std::move(currentTransformer), trackLocations));
        return;
    }


    cv::Rect regionOfInterest;
    if (supersetRegionCroppingEnabled) {
        regionOfInterest = GetSupersetRegion(trackLocations);
    }
    else {
        regionOfInterest = GetSearchRegion(job.job_properties, inputVideoSize);
    }
    AddCropperIfNeeded(regionOfInterest, inputVideoSize, currentTransformer);
}


IFrameTransformer::Ptr GetTransformer(const MPFJob &job, const cv::Size &inputVideoSize,
                                      const std::map<int, MPFImageLocation> &trackLocations) {

    IFrameTransformer::Ptr transformer(new NoOpFrameTransformer(inputVideoSize));

    AddRotatorIfNeeded(job.job_properties, job.media_properties, transformer);
    AddFlipperIfNeeded(job.job_properties, job.media_properties, transformer);
    AddCropperIfNeeded(job, inputVideoSize, trackLocations, transformer);

    return transformer;
}
}



IFrameTransformer::Ptr GetTransformer(const MPFVideoJob &job, const cv::Size &inputVideoSize) {
    return GetTransformer(job, inputVideoSize, job.feed_forward_track.frame_locations);
}


IFrameTransformer::Ptr GetTransformer(const MPFImageJob &job, const cv::Size &inputVideoSize) {
    return GetTransformer(job, inputVideoSize, { { 0, job.feed_forward_location } });
}


IFrameTransformer::Ptr GetTransformer(const MPFStreamingVideoJob &job, const cv::Size &inputVideoSize) {

    IFrameTransformer::Ptr transformer(new NoOpFrameTransformer(inputVideoSize));

    AddRotatorIfNeeded(job.job_properties, job.media_properties, transformer);
    AddFlipperIfNeeded(job.job_properties, job.media_properties, transformer);

    if (SearchRegionCroppingIsEnabled(job.job_properties)) {
        AddCropperIfNeeded(GetSearchRegion(job.job_properties, inputVideoSize),
                           inputVideoSize, transformer);
    }
    return transformer;
}
}}}
