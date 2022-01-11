/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/prediction_based_permission_ui_selector.h"

#define IsPermissionRequestSupported IsPermissionRequestSupported_ChromiumImpl
#include "src/chrome/browser/permissions/prediction_based_permission_ui_selector.cc"
#undef IsPermissionRequestSupported

bool PredictionBasedPermissionUiSelector::IsPermissionRequestSupported(
    permissions::RequestType request_type) {
  return false;
}
