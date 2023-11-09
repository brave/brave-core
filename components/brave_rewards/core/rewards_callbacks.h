/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_CALLBACKS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_CALLBACKS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_database.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"

namespace brave_rewards::internal {

using AttestPromotionCallback = mojom::RewardsEngine::AttestPromotionCallback;

using BeginExternalWalletLoginCallback =
    mojom::RewardsEngine::BeginExternalWalletLoginCallback;

using ClaimPromotionCallback = mojom::RewardsEngine::ClaimPromotionCallback;

using ConnectExternalWalletCallback =
    mojom::RewardsEngine::ConnectExternalWalletCallback;

using CreateRewardsWalletCallback =
    mojom::RewardsEngine::CreateRewardsWalletCallback;

using FetchBalanceCallback = mojom::RewardsEngine::FetchBalanceCallback;

using FetchPromotionsCallback = mojom::RewardsEngine::FetchPromotionsCallback;

using GetExternalWalletCallback =
    mojom::RewardsEngine::GetExternalWalletCallback;

using GetRewardsParametersCallback =
    mojom::RewardsEngine::GetRewardsParametersCallback;

using GetRewardsWalletCallback = mojom::RewardsEngine::GetRewardsWalletCallback;

using LoadURLCallback = base::OnceCallback<void(mojom::UrlResponsePtr)>;

using PostSuggestionsClaimCallback =
    base::OnceCallback<void(mojom::Result, std::string)>;

using ResultCallback = base::OnceCallback<void(mojom::Result)>;

using RunDBTransactionCallback =
    base::OnceCallback<void(mojom::DBCommandResponsePtr)>;

// Legacy callbacks:

using ContributionInfoListCallback =
    std::function<void(std::vector<mojom::ContributionInfoPtr>)>;

using GetActivityInfoListCallback =
    std::function<void(std::vector<mojom::PublisherInfoPtr>)>;

using GetAllMonthlyReportIdsCallback =
    std::function<void(const std::vector<std::string>&)>;

using GetAllPromotionsCallback =
    std::function<void(base::flat_map<std::string, mojom::PromotionPtr>)>;

using GetBalanceReportListCallback = std::function<void(
    std::vector<mojom::BalanceReportInfoPtr>)>;  // TODO(sszaloki): unused?

using GetContributionReportCallback =
    std::function<void(std::vector<mojom::ContributionReportInfoPtr>)>;

using GetEventLogsCallback =
    std::function<void(std::vector<mojom::EventLogPtr>)>;

using GetExcludedListCallback =
    std::function<void(std::vector<mojom::PublisherInfoPtr>)>;

using GetMonthlyReportCallback =
    std::function<void(mojom::Result, mojom::MonthlyReportInfoPtr)>;

using GetOneTimeTipsCallback =
    std::function<void(std::vector<mojom::PublisherInfoPtr>)>;

using GetPublisherBannerCallback =
    std::function<void(mojom::PublisherBannerPtr)>;

using GetPublisherInfoCallback =
    std::function<void(mojom::Result, mojom::PublisherInfoPtr)>;

using GetPublisherPanelInfoCallback =
    std::function<void(mojom::Result, mojom::PublisherInfoPtr)>;

using GetRecurringTipsCallback =
    std::function<void(std::vector<mojom::PublisherInfoPtr>)>;

using GetTransactionReportCallback =
    std::function<void(std::vector<mojom::TransactionReportInfoPtr>)>;

using LegacyLoadURLCallback = std::function<void(mojom::UrlResponsePtr)>;

using LegacyResultCallback = std::function<void(mojom::Result)>;

using PublisherInfoCallback =
    std::function<void(mojom::Result, mojom::PublisherInfoPtr)>;

using RefreshPublisherCallback = std::function<void(mojom::PublisherStatus)>;

using SKUOrderCallback = std::function<void(mojom::Result, const std::string&)>;

using UnverifiedPublishersCallback =
    std::function<void(std::vector<std::string>&&)>;

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_CALLBACKS_H_
