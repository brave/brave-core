/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_uma_util.h"

#include "components/permissions/permissions_client.h"

// Since we don't do UMA just reuse an existing UMA type instead of adding one.
#define BRAVE_GET_UMA_VALUE_FOR_REQUEST_TYPE         \
  case RequestType::kWidevine:                       \
  case RequestType::kBraveEthereum:                  \
  case RequestType::kBraveSolana:                    \
  case RequestType::kBraveGoogleSignInPermission:    \
  case RequestType::kBraveLocalhostAccessPermission: \
  case RequestType::kBraveOpenAIChat:                \
    return RequestTypeForUma::PERMISSION_VR;

// These requests may be batched together, so we must handle them explicitly as
// GetUmaValueForRequests expects only a few specific request types to be
// batched
#define BRAVE_GET_UMA_VALUE_FOR_REQUESTS             \
  if (request_type >= RequestType::kBraveMinValue && \
      request_type <= RequestType::kBraveMaxValue) { \
    return GetUmaValueForRequestType(request_type);  \
  }

// We do not record permissions UKM and this can save us from patching
// in RecordPermissionAction for unhandling switch cases for Brave's content
// settings type.
#define GetUkmSourceId             \
  GetSettingsMap(browser_context); \
  if (true)                        \
    return;                        \
  PermissionsClient::Get()->GetUkmSourceId

#define kTpcdGrant                  \
  kRemoteList:                      \
  source_suffix = "FromRemoteList"; \
  break;                            \
  case SettingSource::kTpcdGrant

#include "src/components/permissions/permission_uma_util.cc"
#undef BRAVE_GET_UMA_VALUE_FOR_REQUESTS
#undef BRAVE_GET_UMA_VALUE_FOR_REQUEST_TYPE
#undef GetUkmSourceId
#undef kTpcdGrant
