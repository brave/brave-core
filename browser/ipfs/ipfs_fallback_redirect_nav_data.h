/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_FALLBACK_REDIRECT_NAV_DATA_H_
#define BRAVE_BROWSER_IPFS_IPFS_FALLBACK_REDIRECT_NAV_DATA_H_

#include "base/supports_user_data.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "url/gurl.h"

namespace ipfs {

class IpfsFallbackRedirectNavigationData : public base::SupportsUserData::Data {
 public:
  IpfsFallbackRedirectNavigationData() = default;
  explicit IpfsFallbackRedirectNavigationData(const GURL& url);
  IpfsFallbackRedirectNavigationData(const GURL& url,
                                     const bool block_auto_redirect,
                                     const bool remove_this_entry_at_the_end);
  ~IpfsFallbackRedirectNavigationData() override;

  static IpfsFallbackRedirectNavigationData* GetOrCreate(
      content::NavigationEntry* entry);
  static IpfsFallbackRedirectNavigationData* Create(
      content::NavigationEntry* entry,
      std::unique_ptr<base::SupportsUserData::Data> data);
  static IpfsFallbackRedirectNavigationData* GetFallbackData(
      content::NavigationEntry* entry);
  static IpfsFallbackRedirectNavigationData* FindFallbackData(
      content::WebContents* web_contents);

  static void CleanAll(content::WebContents* web_contents);

  GURL GetOriginalUrl() const;
  bool IsAutoRedirectBlocked() const;
  bool GetRemoveFlag() const;

  void SetOriginalUrl(const GURL& url);
  void SetAutoRedirectBlock(const bool new_val);
  void SetRemoveFlag(const bool new_val);

  std::unique_ptr<base::SupportsUserData::Data> Clone() override;

  std::string ToDebugString() const;

 private:
  FRIEND_TEST_ALL_PREFIXES(IpfsFallbackRedirectNavigationDataUnitTest,
                           CleanUserDataForAllNavEntries);
  FRIEND_TEST_ALL_PREFIXES(IpfsFallbackRedirectNavigationDataUnitTest,
                           IsSameIpfsLink);
  FRIEND_TEST_ALL_PREFIXES(IpfsFallbackRedirectNavigationDataUnitTest,
                           GetFallbackDataFromRedirectChain);
  FRIEND_TEST_ALL_PREFIXES(IpfsFallbackRedirectNavigationDataUnitTest,
                           IsAutoRedirectBlocked);

  GURL original_url_;
  bool block_auto_redirect_{false};
  bool remove_this_entry_at_the_end_{false};
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_FALLBACK_REDIRECT_NAV_DATA_H_
