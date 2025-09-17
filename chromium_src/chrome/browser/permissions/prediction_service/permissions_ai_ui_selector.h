/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PERMISSIONS_PREDICTION_SERVICE_PERMISSIONS_AI_UI_SELECTOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PERMISSIONS_PREDICTION_SERVICE_PERMISSIONS_AI_UI_SELECTOR_H_

#include "components/permissions/prediction_service/permission_ui_selector.h"

#define IsPermissionRequestSupported          \
  IsPermissionRequestSupported_ChromiumImpl(  \
      permissions::RequestType request_type); \
  bool IsPermissionRequestSupported

#include <chrome/browser/permissions/prediction_service/permissions_ai_ui_selector.h>  // IWYU pragma: export
#undef IsPermissionRequestSupported

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PERMISSIONS_PREDICTION_SERVICE_PERMISSIONS_AI_UI_SELECTOR_H_
