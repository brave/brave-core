// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/speech_to_text/features.h"

#include <string>

namespace stt {

BASE_FEATURE(kSttFeature, "speech_to_text", base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<std::string> kSttUrl{&kSttFeature, "web_service_url",
                                              ""};

}  // namespace stt
