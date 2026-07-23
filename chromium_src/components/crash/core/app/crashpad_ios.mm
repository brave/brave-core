// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CRASH_CORE_APP_CRASHPAD_IOS_MM_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CRASH_CORE_APP_CRASHPAD_IOS_MM_

#include <map>
#include <string>

#include "base/version_info/version_info_values.h"

namespace crash_reporter {
namespace {

// Update the annotations used for crash reports
std::map<std::string, std::string> GetProcessSimpleAnnotations_BraveImpl(
    std::map<std::string, std::string> annotations) {
  // Defaults to Chrome_iOS in official builds
  annotations["prod"] = "Brave_iOS";
  // Chromium uses CFBundleVersion in the main outer bundle, but this isn't
  // what we expect on backtrace, so replace it to match other platforms
  annotations["ver"] = PRODUCT_VERSION;
  return annotations;
}

}  // namespace
}  // namespace crash_reporter

#include <components/crash/core/app/crashpad_ios.mm>

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CRASH_CORE_APP_CRASHPAD_IOS_MM_
