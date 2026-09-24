// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <map>
#include <string>

#include "base/version_info/version_info_values.h"

namespace crash_reporter {
namespace {

// Update the annotations used for crash reports
std::map<std::string, std::string> AddBraveProcessAnnotations(
    std::map<std::string, std::string> annotations) {
  // Chromium uses CFBundleVersion in the main outer bundle but for Brave iOS
  // this is only the patch number which follows Apple versioning practices.
  // On other platforms the version used for backtrace is the major chromium +
  // brave verison (e.g. 151.1.95.123) so replace it to match.
  annotations["ver"] = PRODUCT_VERSION;
  return annotations;
}

}  // namespace
}  // namespace crash_reporter

#include <components/crash/core/app/crashpad_ios.mm>
