// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/public/webui/url_data_source_ios.h"

namespace web {

std::string URLDataSourceIOS::GetContentSecurityPolicyBase() const {
  // Default for iOS:
  // kChromeURLContentSecurityPolicyHeaderBase
  return "script-src chrome://resources 'self'; ";
}

std::string URLDataSourceIOS::GetContentSecurityPolicyFrameSrc() const {
  // Default for iOS:
  // https://source.chromium.org/chromium/chromium/src/+/main:ios/web/webui/url_data_manager_ios_backend.mm;l=511?q=set_content_security_policy_frame_source&ss=chromium%2Fchromium%2Fsrc
  return "frame-src 'none';";
}

}  // namespace web

#include "src/ios/web/webui/url_data_source_ios.mm"
