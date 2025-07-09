// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/direct_feed_fetcher_delegate_impl.h"

#include "base/check.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_news/browser/direct_feed_fetcher.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "content/public/browser/browser_thread.h"

namespace brave_news {

DirectFeedFetcherDelegateImpl::DirectFeedFetcherDelegateImpl(
    HostContentSettingsMap* host_content_settings_map)
    : host_content_settings_map_(host_content_settings_map),
      https_upgrade_exceptions_service_(
          g_brave_browser_process->https_upgrade_exceptions_service()) {
  CHECK(host_content_settings_map_);
  CHECK(https_upgrade_exceptions_service_);
}

DirectFeedFetcherDelegateImpl::~DirectFeedFetcherDelegateImpl() = default;

DirectFeedFetcher::Delegate::HTTPSUpgradeInfo
DirectFeedFetcherDelegateImpl::GetURLHTTPSUpgradeInfo(const GURL& url) {
  HTTPSUpgradeInfo info;
  info.should_upgrade = brave_shields::ShouldUpgradeToHttps(
      host_content_settings_map_, url, https_upgrade_exceptions_service_);
  info.should_force =
      brave_shields::ShouldForceHttps(host_content_settings_map_, url);
  return info;
}

base::WeakPtr<DirectFeedFetcher::Delegate>
DirectFeedFetcherDelegateImpl::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace brave_news
