/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/page_info/page_info.h"

#define PageInfo PageInfo_ChromiumImpl

#define BRAVE_PAGE_INFO_KPERMISSIONTYPE \
  ContentSettingsType::JAVASCRIPT_OPTIMIZER,

#define BRAVE_PAGE_INFO_SHOULD_SHOW_PERMISSION         \
  int _brave_should_show_permission =                  \
      delegate_->BraveShouldShowPermission(info.type); \
  if (_brave_should_show_permission != -1)             \
    return _brave_should_show_permission;

#include <components/page_info/page_info.cc>
#undef BRAVE_PAGE_INFO_SHOULD_SHOW_PERMISSION
#undef BRAVE_PAGE_INFO_KPERMISSIONTYPE
#undef PageInfo

std::set<net::SchemefulSite> PageInfo::GetTwoSitePermissionRequesters(
    ContentSettingsType type) {
  if (type == ContentSettingsType::STORAGE_ACCESS) {
    return {};
  }
  return PageInfo_ChromiumImpl::GetTwoSitePermissionRequesters(type);
}
