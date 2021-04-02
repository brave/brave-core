/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_IMPL_H_

#define PermissionRequestImpl PermissionRequestImpl_ChromiumImpl
#include "../../../../components/permissions/permission_request_impl.h"
#undef PermissionRequestImpl

namespace permissions {

// Make it final to protect from accidental permission lifetime handling issues.
class PermissionRequestImpl final : public PermissionRequestImpl_ChromiumImpl {
 public:
  using PermissionRequestImpl_ChromiumImpl::PermissionRequestImpl_ChromiumImpl;

  bool SupportsLifetime() const override;
};

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_IMPL_H_
