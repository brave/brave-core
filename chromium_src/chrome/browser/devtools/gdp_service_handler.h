/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DEVTOOLS_GDP_SERVICE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DEVTOOLS_GDP_SERVICE_HANDLER_H_

#include "chrome/browser/devtools/devtools_http_service_handler.h"

// Override to disable GDP service handler requests as it sends them to
// googleapis.com.
#define OAuthScopes                                                    \
  Undefined();                                                         \
  void CanMakeRequest(Profile* profile,                                \
                      base::OnceCallback<void(bool success)> callback) \
      override;                                                        \
  signin::ScopeSet OAuthScopes

#include <chrome/browser/devtools/gdp_service_handler.h>  // IWYU pragma: export

#undef OAuthScopes

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DEVTOOLS_GDP_SERVICE_HANDLER_H_
