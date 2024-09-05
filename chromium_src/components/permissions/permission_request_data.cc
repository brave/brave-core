/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_request_data.h"

#include <optional>

#include "components/permissions/permission_context_base.h"

namespace permissions {

std::optional<RequestType> ContentSettingsTypeToRequestTypeIfExists_BraveImpl(
    ContentSettingsType content_settings_type) {
  switch (content_settings_type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
      return RequestType::kBraveEthereum;
    case ContentSettingsType::BRAVE_SOLANA:
      return RequestType::kBraveSolana;
    case ContentSettingsType::BRAVE_GOOGLE_SIGN_IN:
      return RequestType::kBraveGoogleSignInPermission;
    case ContentSettingsType::BRAVE_LOCALHOST_ACCESS:
      return RequestType::kBraveLocalhostAccessPermission;
    case ContentSettingsType::BRAVE_OPEN_AI_CHAT:
      return RequestType::kBraveOpenAIChat;
    default:
      return ContentSettingsTypeToRequestTypeIfExists(content_settings_type);
  }
}

}  // namespace permissions

#define PermissionContextBase PermissionContextBase_ChromiumImpl

#define ContentSettingsTypeToRequestTypeIfExists \
  ContentSettingsTypeToRequestTypeIfExists_BraveImpl

#include "src/components/permissions/permission_request_data.cc"

#undef ContentSettingsTypeToRequestTypeIfExists
#undef PermissionContextBase
