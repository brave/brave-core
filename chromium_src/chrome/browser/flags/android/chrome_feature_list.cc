/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_features.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "third_party/blink/public/common/features.h"

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/components/brave_wallet/common/features.h"
#endif

namespace chrome {
namespace android {

namespace {

const base::Feature* kBraveFeaturesExposedToJava[] = {
    &features::kBraveRewards,
    &blink::features::kForceWebContentsDarkMode,
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
    &brave_wallet::features::kNativeBraveWalletFeature,
#endif
};

const base::Feature* BraveFindFeatureExposedToJava(
    const std::string& feature_name) {
  for (const base::Feature* feature : kBraveFeaturesExposedToJava) {
    if (feature->name == feature_name)
      return feature;
  }

  return nullptr;
}

}  // namespace

}  // namespace android
}  // namespace chrome

#define BRAVE_FIND_FEATURE_EXPOSED_TO_JAVA                                    \
  const base::Feature* feature = BraveFindFeatureExposedToJava(feature_name); \
  if (feature)                                                                \
    return feature;

#include "../../../../../../chrome/browser/flags/android/chrome_feature_list.cc"
#undef BRAVE_FIND_FEATURE_EXPOSED_TO_JAVA
