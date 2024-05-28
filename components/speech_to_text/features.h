// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEECH_TO_TEXT_FEATURES_H_
#define BRAVE_COMPONENTS_SPEECH_TO_TEXT_FEATURES_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace stt {

BASE_DECLARE_FEATURE(kSttFeature);

extern const base::FeatureParam<std::string> kSttUrl;

}  // namespace stt

#endif  // BRAVE_COMPONENTS_SPEECH_TO_TEXT_FEATURES_H_
