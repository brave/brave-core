/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"

#include "src/base/test/scoped_feature_list.cc"

namespace base::test {

void ScopedFeatureList::InitWithFeaturesAndDisable(
    const FeatureRef& feature_to_disable,
    const std::vector<FeatureRef>& enabled_features,
    const std::vector<FeatureRef>& disabled_features) {
  std::vector all_disabled_features = disabled_features;
  all_disabled_features.push_back(feature_to_disable);
  InitWithFeatures(enabled_features, all_disabled_features);
}

}  // namespace base::test
