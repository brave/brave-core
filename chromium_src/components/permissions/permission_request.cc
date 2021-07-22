/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_request.h"

#include <vector>

#include "base/containers/contains.h"


#define PermissionRequest PermissionRequest_ChromiumImpl
#define IsDuplicateOf IsDuplicateOf_ChromiumImpl

 // Handled by an override in `WidevinePermissionRequest`.
#define BRAVE_PERMISSION_REQUEST_GET_MESSAGE_TEXT_FRAGMENT \
  case RequestType::kWidevine:                             \
    NOTREACHED();                                          \
    return std::u16string();

#include "../../../../components/permissions/permission_request.cc"
#undef BRAVE_PERMISSION_REQUEST_GET_MESSAGE_TEXT_FRAGMENT
#undef IsDuplicateOf
#undef PermissionRequest

namespace permissions {

PermissionRequest::PermissionRequest(
    const GURL& requesting_origin,
    RequestType request_type,
    bool has_gesture,
    PermissionDecidedCallback permission_decided_callback,
    base::OnceClosure delete_callback)
    : PermissionRequest_ChromiumImpl(requesting_origin,
                                     request_type,
                                     has_gesture,
                                     std::move(permission_decided_callback),
                                     std::move(delete_callback)) {}

PermissionRequest::~PermissionRequest() = default;

bool PermissionRequest::SupportsLifetime() const {
  static const std::vector<RequestType> excluded_types = {
    RequestType::kDiskQuota,
    RequestType::kMultipleDownloads,
#if defined(OS_ANDROID)
    RequestType::kProtectedMediaIdentifier,
#else
    RequestType::kRegisterProtocolHandler,
    RequestType::kSecurityAttestation,
#endif  // defined(OS_ANDROID)
#if BUILDFLAG(ENABLE_WIDEVINE)
    RequestType::kWidevine
#endif  // BUILDFLAG(ENABLE_WIDEVINE)
  };
  if (base::Contains(excluded_types, request_type())) {
    return false;
  }
  return true;
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
