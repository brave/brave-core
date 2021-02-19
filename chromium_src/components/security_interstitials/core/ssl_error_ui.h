/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_SSL_ERROR_UI_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_SSL_ERROR_UI_H_

#define HandleCommand                                 \
  HandleCommand(SecurityInterstitialCommand command); \
  void HandleCommand_ChromiumImpl

#include "../../../../../components/security_interstitials/core/ssl_error_ui.h"
#undef HandleCommand

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SECURITY_INTERSTITIALS_CORE_SSL_ERROR_UI_H_
