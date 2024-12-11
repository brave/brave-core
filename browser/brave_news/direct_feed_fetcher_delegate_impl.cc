// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/direct_feed_fetcher_delegate_impl.h"

#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "content/public/browser/browser_thread.h"

namespace brave_news {

DirectFeedFetcherDelegateImpl::DirectFeedFetcherDelegateImpl(
    HostContentSettingsMap* host_content_settings_map)
    : host_content_settings_map_(host_content_settings_map),
      https_upgrade_exceptions_service_(
          g_brave_browser_process->https_upgrade_exceptions_service()) {}

DirectFeedFetcherDelegateImpl::~DirectFeedFetcherDelegateImpl() = default;

bool DirectFeedFetcherDelegateImpl::ShouldUpgradeToHttps(const GURL& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!host_content_settings_map_ || !https_upgrade_exceptions_service_) {
    return true;
  }
  return brave_shields::ShouldUpgradeToHttps(host_content_settings_map_, url,
                                             https_upgrade_exceptions_service_);
}

base::WeakPtr<DirectFeedFetcher::Delegate>
DirectFeedFetcherDelegateImpl::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace brave_news
