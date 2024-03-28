/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PAGE_INFO_PAGE_INFO_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PAGE_INFO_PAGE_INFO_H_

#define PageInfo PageInfo_ChromiumImpl

#define GetTwoSitePermissionRequesters     \
  GetTwoSitePermissionRequesters_Unused(); \
                                           \
 protected:                                \
  virtual std::set<net::SchemefulSite> GetTwoSitePermissionRequesters

#include "src/components/page_info/page_info.h"  // IWYU pragma: export
#undef GetTwoSitePermissionRequesters
#undef PageInfo

class PageInfo : public PageInfo_ChromiumImpl {
 public:
  using PageInfo_ChromiumImpl::PageInfo_ChromiumImpl;
  PageInfo(const PageInfo&) = delete;
  PageInfo& operator=(const PageInfo&) = delete;

  ~PageInfo() override = default;

 private:
  std::set<net::SchemefulSite> GetTwoSitePermissionRequesters(
      ContentSettingsType type) override;
};

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PAGE_INFO_PAGE_INFO_H_
