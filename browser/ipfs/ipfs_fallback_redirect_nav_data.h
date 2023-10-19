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
                                     const bool valid);
  ~IpfsFallbackRedirectNavigationData() override;

  static IpfsFallbackRedirectNavigationData* GetOrCreate(
      content::NavigationEntry* entry);
  static IpfsFallbackRedirectNavigationData* GetFallbackData(
      content::NavigationEntry* entry);
  static IpfsFallbackRedirectNavigationData* GetFallbackDataFromRedirectChain(
      content::WebContents* web_contents);
  static bool IsAutoRedirectBlocked(content::WebContents* web_contents,
                                    const GURL& current_page_url,
                                    const bool remove_from_history);

  static bool IsSameIpfsLink(content::WebContents* web_contents,
                             const GURL& current_page_url);
  static void CleanAll(content::WebContents* web_contents);

  GURL GetOriginalUrl() const;
  bool IsAutoRedirectBlocked() const;
  bool IsValid() const;

  void SetOriginalUrl(const GURL& url);
  void SetAutoRedirectBlock(const bool new_val);
  void SetValid(const bool new_val);

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
  bool valid_{true};
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_FALLBACK_REDIRECT_NAV_DATA_H_
