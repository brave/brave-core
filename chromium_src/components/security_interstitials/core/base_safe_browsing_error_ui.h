/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_BASE_SAFE_BROWSING_ERROR_UI_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_BASE_SAFE_BROWSING_ERROR_UI_H_

#define CanShowEnhancedProtectionMessage               \
  CanShowEnhancedProtectionMessage() { return false; } \
  bool CanShowEnhancedProtectionMessage_ChromiumImpl

#include "../../../../../components/security_interstitials/core/base_safe_browsing_error_ui.h"
#undef CanShowEnhancedProtectionMessage

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_BASE_SAFE_BROWSING_ERROR_UI_H_
