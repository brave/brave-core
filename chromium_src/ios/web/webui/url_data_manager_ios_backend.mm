/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/webui/url_data_manager_ios_backend.h"

#import <set>

// Override ShouldDenyXFrameOptions
// to add our own CSP headers here:
// https://source.chromium.org/chromium/chromium/src/+/main:ios/web/webui/url_data_manager_ios_backend.mm;l=292;drc=1379ddb0f0535ff846ce0fbad8ee49af303140c4?q=GetContentSecurityPolicyObjectSrc&ss=chromium%2Fchromium%2Fsrc

#define ShouldDenyXFrameOptions ShouldDenyXFrameOptions());  \
  job->set_content_security_policy_frame_source(             \
      source->source()->GetContentSecurityPolicyBase() +     \
      source->source()->GetContentSecurityPolicyFrameSrc()); \
        void(void

#include "src/ios/web/webui/url_data_manager_ios_backend.mm"

#undef ShouldDenyXFrameOptions
