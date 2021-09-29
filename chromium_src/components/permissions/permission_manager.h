/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_MANAGER_H_

// In case this fails compilation in the future, simply apply to any
// other private member, more obscure name is better so it doesn't
// affect other included header files from this header file.
#define devtools_global_overrides_origin_ \
  devtools_global_overrides_origin_;      \
  friend class BravePermissionManager

#define GetCanonicalOrigin virtual GetCanonicalOrigin
#include "../../../../components/permissions/permission_manager.h"
#undef GetCanonicalOrigin

#undef devtools_global_overrides_origin_

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_MANAGER_H_
