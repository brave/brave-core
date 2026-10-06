/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_global_features.h"

#include <memory>

#include "base/check_op.h"
#include "chrome/browser/global_features.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "base/feature_list.h"
#include "brave/components/extension_malware_blocklist/browser/extension_malware_blocklist.h"
#include "brave/components/extension_malware_blocklist/common/features.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

namespace {

// Lets code reach the features without knowing whether GlobalFeatures is
// really a BraveGlobalFeatures, which is not the case in every test.
BraveGlobalFeatures* g_instance = nullptr;

}  // namespace

BraveGlobalFeatures::BraveGlobalFeatures() {
  CHECK(!g_instance);
  g_instance = this;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (base::FeatureList::IsEnabled(
          extension_malware_blocklist::features::kExtensionMalwareBlocklist)) {
    extension_malware_blocklist_ = std::make_unique<
        extension_malware_blocklist::ExtensionMalwareBlocklist>();
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
}

BraveGlobalFeatures::~BraveGlobalFeatures() {
  CHECK_EQ(g_instance, this);
  g_instance = nullptr;
}

BraveGlobalFeatures* BraveGlobalFeatures::FromGlobalFeatures(
    GlobalFeatures* global_features) {
  return static_cast<BraveGlobalFeatures*>(global_features);
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
// static
extension_malware_blocklist::ExtensionMalwareBlocklist*
BraveGlobalFeatures::GetExtensionMalwareBlocklist() {
  return g_instance ? g_instance->extension_malware_blocklist() : nullptr;
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

GlobalFeatures* CreateBraveGlobalFeatures() {
  return new BraveGlobalFeatures();
}
