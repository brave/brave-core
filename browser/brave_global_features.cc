/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_global_features.h"

#include <memory>

#include "chrome/browser/global_features.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "base/feature_list.h"
#include "brave/components/extension_malware_blocklist/browser/extension_malware_blocklist.h"
#include "brave/components/extension_malware_blocklist/common/features.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

BraveGlobalFeatures::BraveGlobalFeatures() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (base::FeatureList::IsEnabled(
          extension_malware_blocklist::features::kExtensionMalwareBlocklist)) {
    extension_malware_blocklist_ = std::make_unique<
        extension_malware_blocklist::ExtensionMalwareBlocklist>();
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
}

BraveGlobalFeatures::~BraveGlobalFeatures() = default;

BraveGlobalFeatures* BraveGlobalFeatures::FromGlobalFeatures(
    GlobalFeatures* global_features) {
  return static_cast<BraveGlobalFeatures*>(global_features);
}

GlobalFeatures* CreateBraveGlobalFeatures() {
  return new BraveGlobalFeatures();
}
