/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/permissions/permission_request_impl.h"

#define GetIconId GetIconId_ChromiumImpl
#define GetMessageTextFragment GetMessageTextFragment_ChromiumImpl
#include "../../../../../chrome/browser/permissions/permission_request_impl.cc"
#undef GetMessageTextFragment
#undef GetIconId

PermissionRequest::IconId PermissionRequestImpl::GetIconId() const {
#if !defined(OS_ANDROID)
  switch (content_settings_type_) {
    case ContentSettingsType::AUTOPLAY:
      return vector_icons::kPlayArrowIcon;
    default:
      break;
  }
#endif
  return GetIconId_ChromiumImpl();
}

base::string16 PermissionRequestImpl::GetMessageTextFragment() const {
  int message_id;
  switch (content_settings_type_) {
    case ContentSettingsType::AUTOPLAY:
      message_id = IDS_AUTOPLAY_PERMISSION_FRAGMENT;
      break;
    default:
      return GetMessageTextFragment_ChromiumImpl();
  }
  return l10n_util::GetStringUTF16(message_id);
}
