/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_MOJOM_STRUCTS_
#define BAT_LEDGER_MOJOM_STRUCTS_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using ActivityInfoFilter = mojom::ActivityInfoFilter;
using ActivityInfoFilterPtr = mojom::ActivityInfoFilterPtr;

using ActivityInfoFilterOrderPair = mojom::ActivityInfoFilterOrderPair;
using ActivityInfoFilterOrderPairPtr = mojom::ActivityInfoFilterOrderPairPtr;

using ActivityMonth = mojom::ActivityMonth;

using AutoContributeProps = mojom::AutoContributeProps;
using AutoContributePropsPtr = mojom::AutoContributePropsPtr;

using Balance = mojom::Balance;
using BalancePtr = mojom::BalancePtr;

using BalanceReportInfo = mojom::BalanceReportInfo;
using BalanceReportInfoPtr = mojom::BalanceReportInfoPtr;

using ClientInfo = mojom::ClientInfo;
using ClientInfoPtr = mojom::ClientInfoPtr;

using ContributionInfo = mojom::ContributionInfo;
using ContributionInfoPtr = mojom::ContributionInfoPtr;

using ContributionQueue = ledger::mojom::ContributionQueue;
using ContributionQueuePtr = ledger::mojom::ContributionQueuePtr;
using ContributionQueueList = std::vector<ledger::mojom::ContributionQueuePtr>;

using ContributionQueuePublisher = ledger::mojom::ContributionQueuePublisher;
using ContributionQueuePublisherPtr =
    ledger::mojom::ContributionQueuePublisherPtr;
using ContributionQueuePublisherList =
    std::vector<ContributionQueuePublisherPtr>;

using ContributionRetry = mojom::ContributionRetry;

using Environment = ledger::mojom::Environment;

using ExcludeFilter = mojom::ExcludeFilter;

using ExternalWallet = mojom::ExternalWallet;
using ExternalWalletPtr = mojom::ExternalWalletPtr;

using MediaEventInfo = mojom::MediaEventInfo;
using MediaEventInfoPtr = mojom::MediaEventInfoPtr;

using OperatingSystem = mojom::OperatingSystem;

using Platform = mojom::Platform;

using PendingContribution = mojom::PendingContribution;
using PendingContributionPtr = mojom::PendingContributionPtr;
using PendingContributionList = std::vector<PendingContributionPtr>;

using PendingContributionInfo = mojom::PendingContributionInfo;
using PendingContributionInfoPtr = mojom::PendingContributionInfoPtr;
using PendingContributionInfoList = std::vector<PendingContributionInfoPtr>;

using Promotion = mojom::Promotion;
using PromotionPtr = mojom::PromotionPtr;
using PromotionList = std::vector<PromotionPtr>;
using PromotionMap = std::map<std::string, PromotionPtr>;

using PromotionType = mojom::PromotionType;

using PromotionCreds = mojom::PromotionCreds;
using PromotionCredsPtr = mojom::PromotionCredsPtr;

using PromotionStatus = mojom::PromotionStatus;

using PublisherBanner = mojom::PublisherBanner;
using PublisherBannerPtr = mojom::PublisherBannerPtr;

using PublisherInfo = mojom::PublisherInfo;
using PublisherInfoPtr = mojom::PublisherInfoPtr;
using PublisherInfoList = std::vector<PublisherInfoPtr>;
using PublisherStatus = mojom::PublisherStatus;

using PublisherExclude = mojom::PublisherExclude;

using ReconcileInfo = mojom::ReconcileInfo;
using ReconcileInfoPtr = mojom::ReconcileInfoPtr;

using ReportType = mojom::ReportType;

using Result = mojom::Result;

using RewardsInternalsInfo = mojom::RewardsInternalsInfo;
using RewardsInternalsInfoPtr = mojom::RewardsInternalsInfoPtr;

using RewardsType = mojom::RewardsType;

using ServerPublisherInfo = mojom::ServerPublisherInfo;
using ServerPublisherInfoPtr = mojom::ServerPublisherInfoPtr;
using ServerPublisherInfoList = std::vector<ServerPublisherInfoPtr>;

using TransferFee = mojom::TransferFee;
using TransferFeePtr = mojom::TransferFeePtr;
using TransferFeeList = std::map<std::string, TransferFeePtr>;

using UnblindedToken = mojom::UnblindedToken;
using UnblindedTokenPtr = mojom::UnblindedTokenPtr;
using UnblindedTokenList = std::vector<UnblindedTokenPtr>;

using UrlMethod = mojom::UrlMethod;

using VisitData = mojom::VisitData;
using VisitDataPtr = mojom::VisitDataPtr;

using WalletProperties = mojom::WalletProperties;
using WalletPropertiesPtr = mojom::WalletPropertiesPtr;

using WalletStatus = mojom::WalletStatus;

}  // namespace ledger

#endif  // BAT_LEDGER_MOJOM_STRUCTS_
