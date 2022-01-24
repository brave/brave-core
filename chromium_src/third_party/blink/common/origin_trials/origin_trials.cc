/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/common/origin_trials/origin_trials.h"

#include "base/containers/contains.h"

namespace blink {
namespace origin_trials {
bool IsTrialValid_ChromiumImpl(base::StringPiece trial_name);
}  // namespace origin_trials
}  // namespace blink

#define IsTrialValid IsTrialValid_ChromiumImpl
#include "../gen/third_party/blink/common/origin_trials/origin_trials.cc"
#undef IsTrialValid

namespace blink {
namespace origin_trials {

bool IsTrialDisabledInBrave(base::StringPiece trial_name) {
  // When updating also update the array in the overload below.
  // clang-format off
  static const char* const kBraveDisabledTrialNames[] = {
      "ConversionMeasurement",
      "DigitalGoods",
      "Fledge",
      "Parakeet",
      "Prerender2",
      "SignedExchangeSubresourcePrefetch",
      "SubresourceWebBundles",
      "TrustTokens",
  };
  // clang-format on

  if (base::Contains(kBraveDisabledTrialNames, trial_name)) {
    // Check if this is still a valid trial name in Chromium. If not, it needs
    // to be changed as in Chromium or removed.
    DCHECK(IsTrialValid_ChromiumImpl(trial_name));
    return true;
  }

  return false;
}

bool IsTrialDisabledInBrave(OriginTrialFeature feature) {
  // When updating also update the array in the overload above.
  // clang-format off
  static const std::array<OriginTrialFeature, 9> kBraveDisabledTrialFeatures = {
      OriginTrialFeature::kConversionMeasurement,
      OriginTrialFeature::kDigitalGoods,
      OriginTrialFeature::kFledge,
      OriginTrialFeature::kParakeet,
      OriginTrialFeature::kPrerender2,
      OriginTrialFeature::kSignedExchangeSubresourcePrefetch,
      OriginTrialFeature::kSubresourceWebBundles,
      OriginTrialFeature::kTrustTokens,
  };
  // clang-format on

  return base::Contains(kBraveDisabledTrialFeatures, feature);
}

bool IsTrialValid(base::StringPiece trial_name) {
  if (IsTrialDisabledInBrave(trial_name))
    return false;

  return IsTrialValid_ChromiumImpl(trial_name);
}

}  // namespace origin_trials
}  // namespace blink
