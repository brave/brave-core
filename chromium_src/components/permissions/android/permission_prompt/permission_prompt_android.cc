/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/permissions/android/permission_prompt/permission_prompt_android.h"

#include "base/feature_list.h"
#include "components/permissions/features.h"

#define PermissionPromptAndroid PermissionPromptAndroid_ChromiumImpl
#include "src/components/permissions/android/permission_prompt/permission_prompt_android.cc"
#undef PermissionPromptAndroid

namespace permissions {

int PermissionPromptAndroid::GetIconId() const {
  const std::vector<raw_ptr<PermissionRequest, VectorExperimental>>& requests =
      delegate()->Requests();
  if (requests.size() == 1) {
    if (requests[0]->request_type() == RequestType::kStorageAccess) {
      return permissions::GetIconId(requests[0]->request_type());
    }
  }
  return PermissionPromptAndroid_ChromiumImpl::GetIconId();
}

bool PermissionPromptAndroid::ShouldUseRequestingOriginFavicon() const {
  return false;
}

}  // namespace permissions
