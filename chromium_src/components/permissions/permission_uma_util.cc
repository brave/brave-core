/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/permissions/permission_uma_util.h"

#include "components/permissions/permissions_client.h"

// We do not record permissions UKM and this can save us from patching
// in RecordPermissionAction for unhandled switch cases for Brave's content
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

// Don't let GetPermissionStringForUma hit NOTREACHED for missing Brave types.
#define BRAVE_GET_PERMISSION_STRING_FOR_UMA return "";

#include <components/permissions/permission_uma_util.cc>
#undef BRAVE_GET_PERMISSION_STRING_FOR_UMA
#undef GetUkmSourceId
#undef kTpcdGrant
