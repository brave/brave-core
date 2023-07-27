/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_request.h"

#include <vector>

#include "base/containers/contains.h"
#include "build/build_config.h"
#include "components/grit/brave_components_strings.h"
#include "components/strings/grit/components_strings.h"
#include "third_party/widevine/cdm/buildflags.h"

#define PermissionRequest PermissionRequest_ChromiumImpl
#define IsDuplicateOf IsDuplicateOf_ChromiumImpl

// `kWidevine` handled by an override in `WidevinePermissionRequest` and the
// Brave Ethereum/Solana permission has its own permission request prompt.
#define BRAVE_ENUM_ITEMS_FOR_SWITCH \
  case RequestType::kBraveEthereum: \
  case RequestType::kBraveSolana:   \
    NOTREACHED();                   \
    return std::u16string();        \
  case RequestType::kWidevine:      \
    NOTREACHED();                   \
    return std::u16string();

// For permission strings that we also need on Android, we need to use
// a string that has a placeholder ($1) in it.
#define BRAVE_ENUM_ITEMS_FOR_SWITCH_DESKTOP                \
  BRAVE_ENUM_ITEMS_FOR_SWITCH                              \
  case RequestType::kBraveGoogleSignInPermission:          \
    message_id = IDS_GOOGLE_SIGN_IN_PERMISSION_FRAGMENT;   \
    break;                                                 \
  case RequestType::kBraveLocalhostAccessPermission:       \
    message_id = IDS_LOCALHOST_ACCESS_PERMISSION_FRAGMENT; \
    break;

#define BRAVE_ENUM_ITEMS_FOR_SWITCH_ANDROID          \
  BRAVE_ENUM_ITEMS_FOR_SWITCH                        \
  case RequestType::kBraveGoogleSignInPermission:    \
    message_id = IDS_GOOGLE_SIGN_IN_INFOBAR_TEXT;    \
    break;                                           \
  case RequestType::kBraveLocalhostAccessPermission: \
    message_id = IDS_LOCALHOST_ACCESS_INFOBAR_TEXT;  \
    break;

namespace {
#if BUILDFLAG(IS_ANDROID)
const unsigned int IDS_VR_INFOBAR_TEXT_OVERRIDE = IDS_VR_INFOBAR_TEXT;
#else
const unsigned int IDS_VR_PERMISSION_FRAGMENT_OVERRIDE =
    IDS_VR_PERMISSION_FRAGMENT;
#endif
}  // namespace

#if BUILDFLAG(IS_ANDROID)
// For PermissionRequest::GetDialogMessageText
#undef IDS_VR_INFOBAR_TEXT
#define IDS_VR_INFOBAR_TEXT     \
  IDS_VR_INFOBAR_TEXT_OVERRIDE; \
  break;                        \
  BRAVE_ENUM_ITEMS_FOR_SWITCH_ANDROID
#else
// For PermissionRequest::GetMessageTextFragment
#undef IDS_VR_PERMISSION_FRAGMENT
#define IDS_VR_PERMISSION_FRAGMENT     \
  IDS_VR_PERMISSION_FRAGMENT_OVERRIDE; \
  break;                               \
  BRAVE_ENUM_ITEMS_FOR_SWITCH_DESKTOP
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
#if BUILDFLAG(IS_ANDROID)
    RequestType::kProtectedMediaIdentifier,
#else
    RequestType::kRegisterProtocolHandler,
    RequestType::kSecurityAttestation,
    RequestType::kU2fApiRequest,
#endif  // BUILDFLAG(IS_ANDROID)
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

void PermissionRequest::SetDontAskAgain(bool dont_ask_again) {
  dont_ask_again_ = dont_ask_again;
}

bool PermissionRequest::GetDontAskAgain() const {
  return dont_ask_again_;
}

bool PermissionRequest::IsDuplicateOf(PermissionRequest* other_request) const {
  return PermissionRequest_ChromiumImpl::IsDuplicateOf_ChromiumImpl(
      other_request);
}

base::WeakPtr<PermissionRequest> PermissionRequest::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace permissions
