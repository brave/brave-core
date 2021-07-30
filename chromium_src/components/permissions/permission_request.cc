/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_request.h"

#define PermissionRequest PermissionRequest_ChromiumImpl
#define IsDuplicateOf IsDuplicateOf_ChromiumImpl
#include "../../../../components/permissions/permission_request.cc"
#undef IsDuplicateOf
#undef PermissionRequest

namespace permissions {

PermissionRequest::PermissionRequest() = default;

PermissionRequest::~PermissionRequest() = default;

bool PermissionRequest::SupportsLifetime() const {
  return false;
}

void PermissionRequest::SetLifetime(absl::optional<base::TimeDelta> lifetime) {
  DCHECK(SupportsLifetime());
  lifetime_ = std::move(lifetime);
}

const absl::optional<base::TimeDelta>& PermissionRequest::GetLifetime() const {
  DCHECK(SupportsLifetime());
  return lifetime_;
}

bool PermissionRequest::IsDuplicateOf(PermissionRequest* other_request) const {
  return PermissionRequest_ChromiumImpl::IsDuplicateOf_ChromiumImpl(
      other_request);
}

}  // namespace permissions
