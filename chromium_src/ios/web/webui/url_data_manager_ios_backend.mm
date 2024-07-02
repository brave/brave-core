/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/webui/url_data_manager_ios_backend.h"

#define ShouldDenyXFrameOptions ShouldDenyXFrameOptions());   \
  job->set_add_content_security_policy(                       \
      source->source()->ShouldAddContentSecurityPolicy());    \
  job->set_content_security_policy_object_source(             \
      source->source()->GetContentSecurityPolicyObjectSrc()); \
  job->set_content_security_policy_frame_source(              \
      source->source()->GetContentSecurityPolicyFrameSrc());  \
        void(void

#include "src/ios/web/webui/url_data_manager_ios_backend.mm"

#undef ShouldDenyXFrameOptions
