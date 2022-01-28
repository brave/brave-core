/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_request.h"

#include <vector>

#include "base/containers/contains.h"
#include "components/strings/grit/components_strings.h"
#include "third_party/widevine/cdm/buildflags.h"

#define PermissionRequest PermissionRequest_ChromiumImpl
#define IsDuplicateOf IsDuplicateOf_ChromiumImpl

// `kWidevine` handled by an override in `WidevinePermissionRequest` and the
// Brave Ethereum permission has its own permission request prompt on desktop.
// We hardcode it with Ethereum on Android to prevent assert for empty String.
#if defined(OS_ANDROID)
#define BRAVE_ENUM_ITEMS_FOR_SWITCH     \
  case RequestType::kBraveEthereum:     \
    return std::u16string(u"Ethereum"); \
  case RequestType::kWidevine:          \
    NOTREACHED();                       \
    return std::u16string();
#else
#define BRAVE_ENUM_ITEMS_FOR_SWITCH \
  case RequestType::kBraveEthereum: \
    NOTREACHED();                   \
    return std::u16string();        \
  case RequestType::kWidevine:      \
    NOTREACHED();                   \
    return std::u16string();
#endif

namespace {
#if defined(OS_ANDROID)
const unsigned int IDS_VR_INFOBAR_TEXT_OVERRIDE = IDS_VR_INFOBAR_TEXT;
#else
const unsigned int IDS_VR_PERMISSION_FRAGMENT_OVERRIDE =
    IDS_VR_PERMISSION_FRAGMENT;
#endif
}  // namespace

#if defined(OS_ANDROID)
// For PermissionRequest::GetDialogMessageText
#undef IDS_VR_INFOBAR_TEXT
#define IDS_VR_INFOBAR_TEXT     \
  IDS_VR_INFOBAR_TEXT_OVERRIDE; \
  break;                        \
  BRAVE_ENUM_ITEMS_FOR_SWITCH
#else
// For PermissionRequest::GetMessageTextFragment
#undef IDS_VR_PERMISSION_FRAGMENT
#define IDS_VR_PERMISSION_FRAGMENT     \
  IDS_VR_PERMISSION_FRAGMENT_OVERRIDE; \
  break;                               \
  BRAVE_ENUM_ITEMS_FOR_SWITCH
#endif

#include "src/components/permissions/permission_request.cc"
#undef IDS_VR_INFOBAR_TEXT
#undef IDS_VR_PERMISSION_FRAGMENT
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
  const RequestType kExcludedTypes[] = {
    RequestType::kDiskQuota,
    RequestType::kMultipleDownloads,
#if defined(OS_ANDROID)
    RequestType::kProtectedMediaIdentifier,
#else
    RequestType::kRegisterProtocolHandler,
    RequestType::kSecurityAttestation,
    RequestType::kU2fApiRequest,
#endif  // defined(OS_ANDROID)
#if BUILDFLAG(ENABLE_WIDEVINE)
    RequestType::kWidevine
#endif  // BUILDFLAG(ENABLE_WIDEVINE)
  };
  return !base::Contains(kExcludedTypes, request_type());
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
