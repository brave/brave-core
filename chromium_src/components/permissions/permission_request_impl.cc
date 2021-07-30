/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_request_impl.h"

#define PermissionRequestImpl PermissionRequestImpl_ChromiumImpl
#include "../../../../components/permissions/permission_request_impl.cc"
#undef PermissionRequestImpl

namespace permissions {

bool PermissionRequestImpl::SupportsLifetime() const {
  return true;
}

}  // namespace permissions
