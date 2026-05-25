// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_news/direct_feed_fetcher_delegate_impl.h"

#include "base/check.h"
#include "base/feature_list.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_news {

DirectFeedFetcherDelegateImpl::DirectFeedFetcherDelegateImpl(
    PrefService* pref_service)
    : pref_service_(pref_service) {
  CHECK(pref_service_);
}

DirectFeedFetcherDelegateImpl::~DirectFeedFetcherDelegateImpl() = default;

DirectFeedFetcher::Delegate::HTTPSUpgradeInfo
DirectFeedFetcherDelegateImpl::GetURLHTTPSUpgradeInfo(const GURL& url) {
  HTTPSUpgradeInfo info;
  info.should_upgrade =
      base::FeatureList::IsEnabled(features::kHttpsUpgrades) &&
      pref_service_->GetBoolean(prefs::kHttpsUpgradesEnabled);
  info.should_force = pref_service_->GetBoolean(prefs::kHttpsOnlyModeEnabled);
  return info;
}

base::WeakPtr<DirectFeedFetcher::Delegate>
DirectFeedFetcherDelegateImpl::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace brave_news
