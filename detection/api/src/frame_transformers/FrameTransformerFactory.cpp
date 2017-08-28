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

namespace MPF { namespace COMPONENT { namespace FrameTransformerFactory {

    namespace {

        cv::Rect GetSearchRegion(const Properties &jobProperties, const cv::Size &frameSize) {

            int regionTopLeftXPos = DetectionComponentUtils::GetProperty(
                    jobProperties, "SEARCH_REGION_TOP_LEFT_X_DETECTION", -1);
            if (regionTopLeftXPos < 0) {
                regionTopLeftXPos = 0;
            }

            int regionTopLeftYPos = DetectionComponentUtils::GetProperty(
                    jobProperties, "SEARCH_REGION_TOP_LEFT_Y_DETECTION", -1);
            if (regionTopLeftYPos < 0) {
                regionTopLeftYPos = 0;
            }

            cv::Point topLeft(regionTopLeftXPos, regionTopLeftYPos);

            int regionBottomRightXPos = DetectionComponentUtils::GetProperty(
                    jobProperties, "SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", -1);
            if (regionBottomRightXPos <= 0) {
                regionBottomRightXPos = frameSize.width;
            }

            int regionBottomRightYPos = DetectionComponentUtils::GetProperty(
                    jobProperties, "SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", -1);
            if (regionBottomRightYPos <= 0) {
                regionBottomRightYPos = frameSize.height;
            }
            cv::Point bottomRight(regionBottomRightXPos, regionBottomRightYPos);

            return cv::Rect(topLeft, bottomRight);
        }


        cv::Rect toRect(const MPFImageLocation &imageLocation) {
            return {imageLocation.x_left_upper, imageLocation.y_left_upper, imageLocation.width, imageLocation.height};
        }


        cv::Rect GetFeedForwardRegion(const std::map<int, MPFImageLocation> &trackLocations) {
            if (trackLocations.empty()) {
                throw std::length_error(
                        "FEED_FORWARD_TYPE: SUPERSET_REGION is enabled, but feed forward track was empty.");
            }

            auto it = trackLocations.begin();
            cv::Rect region = toRect(it->second);
            it++;

            for (; it != trackLocations.end(); it++) {
                region |= toRect(it->second);
            }
            return region;
        }


        bool FeedForwardCroppingIsEnabled(const Properties &jobProperties) {
            return "SUPERSET_REGION" ==
                    DetectionComponentUtils::GetProperty(jobProperties, "FEED_FORWARD_TYPE", string());
        }


        bool SearchRegionCroppingIsEnabled(const Properties &jobProperties) {
            return DetectionComponentUtils::GetProperty(jobProperties, "SEARCH_REGION_ENABLE_DETECTION", false);
        }


        void AddFlipperIfNeeded(const MPFJob &job, IFrameTransformer::Ptr &currentTransformer) {
            bool shouldFlip;
            if (DetectionComponentUtils::GetProperty(job.job_properties, "AUTO_FLIP", false)) {
                shouldFlip = DetectionComponentUtils::GetProperty(job.media_properties, "HORIZONTAL_FLIP", false);
            }
            else {
                shouldFlip = DetectionComponentUtils::GetProperty(job.job_properties, "HORIZONTAL_FLIP", false);
            }

            if (shouldFlip) {
                currentTransformer = IFrameTransformer::Ptr(new FrameFlipper(std::move(currentTransformer)));
            }
        }


        void AddRotatorIfNeeded(const MPFJob &job, IFrameTransformer::Ptr &currentTransformer) {
            string rotationKey = "ROTATION";
            int rotation = 0;
            if (DetectionComponentUtils::GetProperty(job.job_properties, "AUTO_ROTATE", false)) {
                rotation = DetectionComponentUtils::GetProperty(job.media_properties, rotationKey, 0);
            }
            else {
                rotation = DetectionComponentUtils::GetProperty(job.job_properties, rotationKey, 0);
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


        void AddCropperIfNeeded(const MPFJob &job, const cv::Size &inputVideoSize,
                                const std::map<int, MPFImageLocation> &trackLocations,
                                IFrameTransformer::Ptr &currentTransformer) {

            cv::Rect regionOfInterest;
            if (FeedForwardCroppingIsEnabled(job.job_properties)) {
                regionOfInterest = GetFeedForwardRegion(trackLocations);
            }
            else if (SearchRegionCroppingIsEnabled(job.job_properties)) {
                regionOfInterest = GetSearchRegion(job.job_properties, inputVideoSize);
            }
            else {
                return;
            }

            bool regionOfInterestIsEntireFrame = cv::Rect({0, 0}, inputVideoSize) == regionOfInterest;

            if (!regionOfInterestIsEntireFrame) {
                currentTransformer = IFrameTransformer::Ptr(
                        new FrameCropper(std::move(currentTransformer), regionOfInterest));
            }
        }


        IFrameTransformer::Ptr GetTransformer(const MPFJob &job, const cv::Size &inputVideoSize,
                                              const std::map<int, MPFImageLocation> &trackLocations) {

            IFrameTransformer::Ptr transformer(new NoOpFrameTransformer(inputVideoSize));

            AddRotatorIfNeeded(job, transformer);
            AddFlipperIfNeeded(job, transformer);
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


}}}
