/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_feature_unittest_util.h"

#include <map>
#include <string>

#include "base/check_op.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

void ForceInlineContentAdServingVersionForTesting(const int version) {
  std::map<std::string, std::string> params;
  params["version"] = base::NumberToString(version);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(
      {{kInlineContentAdServingFeature, params}}, {});

  CHECK_EQ(version, kInlineContentAdServingVersion.Get());
}

}  // namespace brave_ads
