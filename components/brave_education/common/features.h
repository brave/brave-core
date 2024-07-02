/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_EDUCATION_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_EDUCATION_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/components/brave_education/common/education_content_urls.h"

namespace brave_education::features {

BASE_DECLARE_FEATURE(kShowGettingStartedPage);

inline constexpr base::FeatureParam<EducationContentType>::Option
    kGettingStartedContentOptions[] = {
        {EducationContentType::kGettingStartedAdBlock, "adblock"},
        {EducationContentType::kGettingStartedPrivacy, "privacy"},
        {EducationContentType::kGettingStartedWeb3, "web3"}};

inline constexpr base::FeatureParam<EducationContentType>
    kGettingStartedContentType{&kShowGettingStartedPage,
                               "BraveGettingStartedContentType",
                               EducationContentType::kGettingStartedPrivacy,
                               &kGettingStartedContentOptions};

}  // namespace brave_education::features

#endif  // BRAVE_COMPONENTS_BRAVE_EDUCATION_COMMON_FEATURES_H_
