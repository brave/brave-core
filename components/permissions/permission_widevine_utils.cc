/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_widevine_utils.h"

#include "brave/components/constants/pref_names.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/request_type.h"
#include "components/prefs/pref_service.h"

namespace permissions {

void DontAskWidevineInstall(PrefService* prefs, bool dont_ask) {
  prefs->SetBoolean(kAskWidevineInstall, !dont_ask);
}

bool HasWidevinePermissionRequest(
    const std::vector<permissions::PermissionRequest*>& requests) {
  // When widevine permission is requested, |requests| only includes Widevine
  // permission because it is not a candidate for grouping.
  if (requests.size() == 1 &&
      requests[0]->request_type() == permissions::RequestType::kWidevine) {
    return true;
  }

  return false;
}

}  // namespace permissions
