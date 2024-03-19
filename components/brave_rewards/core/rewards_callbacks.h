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

using BeginExternalWalletLoginCallback =
    mojom::RewardsEngine::BeginExternalWalletLoginCallback;

using ConnectExternalWalletCallback =
    mojom::RewardsEngine::ConnectExternalWalletCallback;

using CreateRewardsWalletCallback =
    mojom::RewardsEngine::CreateRewardsWalletCallback;

using FetchBalanceCallback = mojom::RewardsEngine::FetchBalanceCallback;

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

using ContributionInfoListCallback =
    base::OnceCallback<void(std::vector<mojom::ContributionInfoPtr>)>;

using GetActivityInfoListCallback =
    base::OnceCallback<void(std::vector<mojom::PublisherInfoPtr>)>;

using GetAllMonthlyReportIdsCallback =
    mojom::RewardsEngine::GetAllMonthlyReportIdsCallback;

using GetBalanceReportListCallback = base::OnceCallback<void(
    std::vector<mojom::BalanceReportInfoPtr>)>;  // TODO(sszaloki): unused?

using GetContributionReportCallback =
    base::OnceCallback<void(std::vector<mojom::ContributionReportInfoPtr>)>;

using GetEventLogsCallback =
    base::OnceCallback<void(std::vector<mojom::EventLogPtr>)>;

using GetExcludedListCallback =
    base::OnceCallback<void(std::vector<mojom::PublisherInfoPtr>)>;

using GetMonthlyReportCallback =
    base::OnceCallback<void(mojom::Result, mojom::MonthlyReportInfoPtr)>;

using GetOneTimeTipsCallback =
    base::OnceCallback<void(std::vector<mojom::PublisherInfoPtr>)>;

using GetPublisherBannerCallback =
    base::OnceCallback<void(mojom::PublisherBannerPtr)>;

using GetPublisherInfoCallback =
    base::OnceCallback<void(mojom::Result, mojom::PublisherInfoPtr)>;

using GetPublisherPanelInfoCallback =
    base::OnceCallback<void(mojom::Result, mojom::PublisherInfoPtr)>;

using GetRecurringTipsCallback =
    base::OnceCallback<void(std::vector<mojom::PublisherInfoPtr>)>;

using PublisherInfoCallback =
    base::OnceCallback<void(mojom::Result, mojom::PublisherInfoPtr)>;

using GetServerPublisherInfoCallback =
    base::OnceCallback<void(mojom::ServerPublisherInfoPtr)>;

using RefreshPublisherCallback =
    base::OnceCallback<void(mojom::PublisherStatus)>;

using SKUOrderCallback =
    base::OnceCallback<void(mojom::Result, const std::string&)>;

using UnverifiedPublishersCallback =
    base::OnceCallback<void(std::vector<std::string>&&)>;

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_CALLBACKS_H_
