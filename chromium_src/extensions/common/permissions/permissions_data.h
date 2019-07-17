/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "../../../../../extensions/common/permissions/permissions_data.h"

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_PERMISSIONS_PERMISSONS_DATA_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_PERMISSIONS_PERMISSONS_DATA_H_

#include "url/gurl.h"

namespace extensions {

// Returns true if the URL points to a security-critical service.
bool IsBraveProtectedUrl(const GURL& url);

}  // namespace extensions

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_PERMISSIONS_PERMISSONS_DATA_H_
