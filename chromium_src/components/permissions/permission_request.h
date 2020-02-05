/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_

#if defined(BRAVE_CHROMIUM_BUILD)
#define BRAVE_PERMISSION_REQUEST_TYPES \
  PERMISSION_AUTOPLAY, \
  PERMISSION_WIDEVINE, \
  PERMISSION_WALLET,
#else
#define BRAVE_PERMISSION_REQUEST_TYPES
#endif

#include "../../../../components/permissions/permission_request.h"

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_
