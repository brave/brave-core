/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/common/mojom/ledger_types.mojom.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"

namespace ledger {

inline mojom::Environment _environment = mojom::Environment::PRODUCTION;
inline bool is_debug = false;
inline bool is_testing = false;
inline int state_migration_target_version_for_testing = -1;
inline int reconcile_interval = 0;  // minutes
inline int retry_interval = 0;      // seconds

using PublisherBannerCallback = std::function<void(mojom::PublisherBannerPtr)>;

using GetRewardsParametersCallback =
    base::OnceCallback<void(mojom::RewardsParametersPtr)>;

using CreateRewardsWalletCallback =
    base::OnceCallback<void(mojom::CreateRewardsWalletResult)>;

using OnRefreshPublisherCallback = std::function<void(mojom::PublisherStatus)>;

using ConnectExternalWalletResult =
    base::expected<void, mojom::ConnectExternalWalletError>;

using ConnectExternalWalletCallback =
    base::OnceCallback<void(ConnectExternalWalletResult)>;

using FetchBalanceResult =
    base::expected<mojom::BalancePtr, mojom::FetchBalanceError>;

using FetchBalanceCallback = base::OnceCallback<void(FetchBalanceResult)>;

using GetExternalWalletResult =
    base::expected<mojom::ExternalWalletPtr, mojom::GetExternalWalletError>;

using GetExternalWalletCallback =
    base::OnceCallback<void(GetExternalWalletResult)>;

using FetchPromotionCallback =
    base::OnceCallback<void(mojom::Result, std::vector<mojom::PromotionPtr>)>;

using ClaimPromotionCallback =
    base::OnceCallback<void(mojom::Result, const std::string&)>;

using RewardsInternalsInfoCallback =
    std::function<void(mojom::RewardsInternalsInfoPtr)>;

using AttestPromotionCallback =
    base::OnceCallback<void(mojom::Result, mojom::PromotionPtr)>;

using GetBalanceReportCallback =
    std::function<void(mojom::Result, mojom::BalanceReportInfoPtr)>;

using GetBalanceReportListCallback =
    std::function<void(std::vector<mojom::BalanceReportInfoPtr>)>;

using ContributionInfoListCallback =
    std::function<void(std::vector<mojom::ContributionInfoPtr>)>;

using GetMonthlyReportCallback =
    std::function<void(mojom::Result, mojom::MonthlyReportInfoPtr)>;

using GetAllMonthlyReportIdsCallback =
    std::function<void(const std::vector<std::string>&)>;

using GetEventLogsCallback =
    std::function<void(std::vector<mojom::EventLogPtr>)>;

using SKUOrderCallback = std::function<void(mojom::Result, const std::string&)>;

using GetContributionReportCallback =
    std::function<void(std::vector<mojom::ContributionReportInfoPtr>)>;

using GetTransactionReportCallback =
    std::function<void(std::vector<mojom::TransactionReportInfoPtr>)>;

using GetAllPromotionsCallback =
    std::function<void(base::flat_map<std::string, mojom::PromotionPtr>)>;

using LegacyResultCallback = std::function<void(mojom::Result)>;

using ResultCallback = base::OnceCallback<void(mojom::Result)>;

using PendingContributionsTotalCallback = std::function<void(double)>;

using PendingContributionInfoListCallback =
    std::function<void(std::vector<mojom::PendingContributionInfoPtr>)>;

using UnverifiedPublishersCallback =
    std::function<void(std::vector<std::string>&&)>;

using PublisherInfoListCallback =
    std::function<void(std::vector<mojom::PublisherInfoPtr>)>;

using PublisherInfoCallback =
    std::function<void(mojom::Result, mojom::PublisherInfoPtr)>;

using GetPublisherInfoCallback =
    std::function<void(mojom::Result, mojom::PublisherInfoPtr)>;

using GetRewardsWalletCallback = std::function<void(mojom::RewardsWalletPtr)>;

using PostSuggestionsClaimCallback =
    base::OnceCallback<void(mojom::Result result, std::string drain_id)>;

using RunDBTransactionCallback =
    base::OnceCallback<void(mojom::DBCommandResponsePtr)>;

using LegacyRunDBTransactionCallback =
    std::function<void(mojom::DBCommandResponsePtr)>;

using LoadURLCallback = base::OnceCallback<void(mojom::UrlResponsePtr)>;

using LegacyLoadURLCallback = std::function<void(mojom::UrlResponsePtr)>;

}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_H_
