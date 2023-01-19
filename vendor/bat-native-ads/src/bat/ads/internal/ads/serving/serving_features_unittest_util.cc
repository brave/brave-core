/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/serving_features_unittest_util.h"

#include <map>
#include <string>

#include "base/check_op.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ads/serving/serving_features.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::features {

void ForceServingVersion(const int version) {
  std::map<std::string, std::string> serving_parameters;
  serving_parameters["ad_serving_version"] = base::NumberToString(version);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{kServing, serving_parameters}}, {});

  CHECK_EQ(version, GetServingVersion());
}

}  // namespace ads::features
