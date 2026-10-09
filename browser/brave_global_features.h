/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_GLOBAL_FEATURES_H_
#define BRAVE_BROWSER_BRAVE_GLOBAL_FEATURES_H_

#include <memory>

#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
namespace extension_malware_blocklist {
class ExtensionMalwareBlocklist;
}  // namespace extension_malware_blocklist
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

// Owns Brave's globally scoped feature controllers. It is a member of
// GlobalFeatures, so it is also present when tests substitute their own
// GlobalFeatures. Retrieve it via `GlobalFeatures::brave_global_features()`.
class BraveGlobalFeatures {
 public:
  BraveGlobalFeatures();
  ~BraveGlobalFeatures();

  BraveGlobalFeatures(const BraveGlobalFeatures&) = delete;
  BraveGlobalFeatures& operator=(const BraveGlobalFeatures&) = delete;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Null when the kExtensionMalwareBlocklist feature is disabled.
  extension_malware_blocklist::ExtensionMalwareBlocklist*
  extension_malware_blocklist() {
    return extension_malware_blocklist_.get();
  }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

 private:
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<extension_malware_blocklist::ExtensionMalwareBlocklist>
      extension_malware_blocklist_;
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
};

#endif  // BRAVE_BROWSER_BRAVE_GLOBAL_FEATURES_H_
