// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_NEWS_DIRECT_FEED_FETCHER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_NEWS_DIRECT_FEED_FETCHER_DELEGATE_IMPL_H_

#include "brave/components/brave_news/browser/direct_feed_fetcher.h"
#include "url/gurl.h"

namespace https_upgrade_exceptions {
class HttpsUpgradeExceptionsService;
}  // namespace https_upgrade_exceptions

class HostContentSettingsMap;

namespace brave_news {

class DirectFeedFetcherDelegateImpl : public DirectFeedFetcher::Delegate {
 public:
  explicit DirectFeedFetcherDelegateImpl(
      HostContentSettingsMap* host_content_settings_map);
  ~DirectFeedFetcherDelegateImpl() override;

  DirectFeedFetcherDelegateImpl(const DirectFeedFetcherDelegateImpl&) = delete;
  DirectFeedFetcherDelegateImpl& operator=(
      const DirectFeedFetcherDelegateImpl&) = delete;

  // Must be called on UI thread
  DirectFeedFetcher::Delegate::HTTPSUpgradeInfo GetURLHTTPSUpgradeInfo(
      const GURL& url) override;

  base::WeakPtr<DirectFeedFetcher::Delegate> AsWeakPtr() override;

 private:
  raw_ptr<HostContentSettingsMap> host_content_settings_map_;
  raw_ptr<https_upgrade_exceptions::HttpsUpgradeExceptionsService>
      https_upgrade_exceptions_service_;

  base::WeakPtrFactory<DirectFeedFetcherDelegateImpl> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_BROWSER_BRAVE_NEWS_DIRECT_FEED_FETCHER_DELEGATE_IMPL_H_
