/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_UMA_UTIL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_UMA_UTIL_H_

// clang-format off
#define BRAVE_PERMISSION_REQUEST_TYPES_FOR_UMA \
  PERMISSION_WIDEVINE, \
  PERMISSION_WALLET,
// clang-format on

#include "../../../../components/permissions/permission_uma_util.h"
#undef BRAVE_PERMISSION_REQUEST_TYPES_FOR_UMA

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_UMA_UTIL_H_
