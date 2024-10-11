// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/filter_list_service.h"

#include <utility>

#include <string>
#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_shields/content/browser/ad_block_custom_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/filter_list.mojom-forward.h"
#include "brave/components/brave_shields/core/common/filter_list.mojom-shared.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/time_format.h"

namespace brave_shields {

FilterListService::FilterListService(AdBlockService* ad_block_service)
    : ad_block_service_(ad_block_service) {}

FilterListService::~FilterListService() = default;

mojo::PendingRemote<mojom::FilterListAndroidHandler>
FilterListService::MakeRemote() {
  mojo::PendingRemote<mojom::FilterListAndroidHandler> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void FilterListService::Bind(
    mojo::PendingReceiver<mojom::FilterListAndroidHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void FilterListService::IsFilterListEnabled(
    const std::string& filterListUuid,
    IsFilterListEnabledCallback callback) {
  std::move(callback).Run(
      ad_block_service_->component_service_manager()->IsFilterListEnabled(
          filterListUuid));
}

void FilterListService::EnableFilter(const std::string& filterListUuid,
                                     bool shouldEnableFilter) {
  ad_block_service_->component_service_manager()->EnableFilterList(
      filterListUuid, shouldEnableFilter);
}

void FilterListService::GetFilterLists(GetFilterListsCallback callback) {
  std::move(callback).Run(
      ad_block_service_->component_service_manager()->GetRegionalLists());
}

void FilterListService::GetSubscriptions(GetSubscriptionsCallback callback) {
  std::vector<mojom::SubscriptionInfoPtr> items;
  std::vector<SubscriptionInfo> subscriptions_ =
      ad_block_service_->subscription_service_manager()->GetSubscriptions();
  base::Time now = base::Time::Now();
  for (const auto& subscription : subscriptions_) {
    auto info = mojom::SubscriptionInfo::New();
    info->enabled = subscription.enabled;
    if (subscription.title) {
      info->title = subscription.title.value();
    }

    base::TimeDelta relative_time_delta =
        now - subscription.last_successful_update_attempt;
    if (relative_time_delta < base::TimeDelta()) {
      relative_time_delta = base::TimeDelta();
    }

    const auto time_str = ui::TimeFormat::Simple(
        ui::TimeFormat::Format::FORMAT_ELAPSED,
        ui::TimeFormat::Length::LENGTH_LONG, relative_time_delta);

    if (subscription.homepage) {
      info->homepage = subscription.homepage.value();
    }
    info->subscription_url = subscription.subscription_url;
    info->last_update_attempt = subscription.last_update_attempt;
    info->last_successful_update_attempt =
        subscription.last_successful_update_attempt;
    info->last_updated_pretty_text = base::UTF16ToUTF8(time_str);
    info->expires = subscription.expires;
    items.push_back(std::move(info));
  }
  std::move(callback).Run(std::move(items));
}

void FilterListService::CreateSubscription(const GURL& subscription_url) {
  ad_block_service_->subscription_service_manager()->CreateSubscription(
      subscription_url);
}

void FilterListService::EnableSubscription(const GURL& sub_url, bool enabled) {
  ad_block_service_->subscription_service_manager()->EnableSubscription(
      sub_url, enabled);
}

void FilterListService::RefreshSubscription(const GURL& sub_url, bool from_ui) {
  ad_block_service_->subscription_service_manager()->RefreshSubscription(
      sub_url, from_ui);
}

void FilterListService::DeleteSubscription(const GURL& sub_url) {
  ad_block_service_->subscription_service_manager()->DeleteSubscription(
      sub_url);
}

void FilterListService::GetCustomFilters(GetCustomFiltersCallback callback) {
  std::move(callback).Run(
      ad_block_service_->custom_filters_provider()->GetCustomFilters());
}

void FilterListService::UpdateCustomFilters(
    const std::string& custom_filters,
    UpdateCustomFiltersCallback callback) {
  std::move(callback).Run(
      ad_block_service_->custom_filters_provider()->UpdateCustomFilters(
          custom_filters));
}

void FilterListService::UpdateFilterLists(UpdateFilterListsCallback callback) {
  ad_block_service_->component_service_manager()->UpdateFilterLists(
      std::move(callback));
}

}  // namespace brave_shields
