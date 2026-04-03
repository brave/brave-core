/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_LOCAL_AI_CORE_FEATURES_H_
#define BRAVE_COMPONENTS_LOCAL_AI_CORE_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace local_ai::features {

COMPONENT_EXPORT(LOCAL_AI_FEATURES)
BASE_DECLARE_FEATURE(kBraveHistoryEmbeddings);
COMPONENT_EXPORT(LOCAL_AI_FEATURES)
BASE_DECLARE_FEATURE(kBraveOnDeviceSpeechRecognition);

}  // namespace local_ai::features

#endif  // BRAVE_COMPONENTS_LOCAL_AI_CORE_FEATURES_H_
