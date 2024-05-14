/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/page_info/page_info.h"

#define PageInfo PageInfo_ChromiumImpl

#define BRAVE_PAGE_INFO_SHOULD_SHOW_PERMISSION          \
  if (!delegate_->BraveShouldShowPermission(info.type)) \
    return false;

#include "src/components/page_info/page_info.cc"
#undef BRAVE_PAGE_INFO_SHOULD_SHOW_PERMISSION
#undef PageInfo

std::set<net::SchemefulSite> PageInfo::GetTwoSitePermissionRequesters(
    ContentSettingsType type) {
  if (type == ContentSettingsType::STORAGE_ACCESS) {
    return {};
  }
  return PageInfo_ChromiumImpl::GetTwoSitePermissionRequesters(type);
}
