/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_AUTOPLAY_POLICY_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_AUTOPLAY_POLICY_H_

#define BRAVE_AUTOPLAY_POLICY_H              \
 private:                                    \
  bool IsAutoplayAllowedPerSettings() const; \
                                             \
 public:
#include "../../../../../third_party/blink/renderer/core/html/media/autoplay_policy.h"  // NOLINT

#undef BRAVE_AUTOPLAY_POLICY_H

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_AUTOPLAY_POLICY_H_
