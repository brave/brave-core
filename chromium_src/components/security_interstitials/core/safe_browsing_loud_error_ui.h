/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_SAFE_BROWSING_LOUD_ERROR_UI_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_SAFE_BROWSING_LOUD_ERROR_UI_H_

#include "components/security_interstitials/core/base_safe_browsing_error_ui.h"

#define HandleCommand                                              \
  HandleCommand_ChromiumImpl(SecurityInterstitialCommand command); \
  void HandleCommand

#include "../../../../../components/security_interstitials/core/safe_browsing_loud_error_ui.h"
#undef HandleCommand

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_SAFE_BROWSING_LOUD_ERROR_UI_H_
