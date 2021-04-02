/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_

#include "base/time/time.h"

#define PermissionRequest PermissionRequest_ChromiumImpl
#include "../../../../components/permissions/permission_request.h"
#undef PermissionRequest

namespace permissions {

class PermissionRequest : public PermissionRequest_ChromiumImpl {
 public:
  PermissionRequest();
  ~PermissionRequest() override;

  virtual bool SupportsLifetime() const;
  void SetLifetime(base::Optional<base::TimeDelta> lifetime);
  const base::Optional<base::TimeDelta>& GetLifetime() const;

 private:
  base::Optional<base::TimeDelta> lifetime_;
};

}  // namespace permissions

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_REQUEST_H_
