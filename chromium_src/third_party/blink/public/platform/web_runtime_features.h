/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RUNTIME_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RUNTIME_FEATURES_H_

#define EnableCLSScrollAnchoring  \
  EnableCLSScrollAnchoring(bool); \
  BLINK_PLATFORM_EXPORT static void EnableNavigatorPluginsFixed

#include "../../../../../../third_party/blink/public/platform/web_runtime_features.h"

#undef EnableCLSScrollAnchoring

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_RUNTIME_FEATURES_H_
