/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/permission_type.h"

#define BRAVE_PERMISSION_TYPES \
    case PermissionType::AUTOPLAY:
#include "../../../../android_webview/browser/aw_permission_manager.cc"
#undef BRAVE_PERMISSION_TYPES
