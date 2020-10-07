/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_MOJOM_STRUCTS_
#define BAT_LEDGER_MOJOM_STRUCTS_

#include <map>
#include <string>
#include <vector>

#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_database.mojom.h"

namespace ledger {
namespace type {
/**
 * LEDGER
 */
using ActivityInfoFilter = mojom::ActivityInfoFilter;
using ActivityInfoFilterPtr = mojom::ActivityInfoFilterPtr;

using ActivityInfoFilterOrderPair = mojom::ActivityInfoFilterOrderPair;

using ActivityMonth = mojom::ActivityMonth;

using AutoContributeProperties = mojom::AutoContributeProperties;
using AutoContributePropertiesPtr = mojom::AutoContributePropertiesPtr;

using Balance = mojom::Balance;
using BalancePtr = mojom::BalancePtr;

using BalanceReportInfo = mojom::BalanceReportInfo;
using BalanceReportInfoPtr = mojom::BalanceReportInfoPtr;
using BalanceReportInfoList =
    std::vector<mojom::BalanceReportInfoPtr>;

using BraveWallet = mojom::BraveWallet;
using BraveWalletPtr = mojom::BraveWalletPtr;

using ClientInfo = mojom::ClientInfo;
using ClientInfoPtr = mojom::ClientInfoPtr;

using ContributionInfo = mojom::ContributionInfo;
using ContributionInfoPtr = mojom::ContributionInfoPtr;
using ContributionInfoList = std::vector<ContributionInfoPtr>;

using ContributionProcessor = mojom::ContributionProcessor;

using ContributionPublisher = mojom::ContributionPublisher;
using ContributionPublisherPtr = mojom::ContributionPublisherPtr;
using ContributionPublisherList = std::vector<ContributionPublisherPtr>;

using ContributionReportInfo = mojom::ContributionReportInfo;
using ContributionReportInfoPtr = mojom::ContributionReportInfoPtr;
using ContributionReportInfoList = std::vector<ContributionReportInfoPtr>;

using ContributionQueue = mojom::ContributionQueue;
using ContributionQueuePtr = mojom::ContributionQueuePtr;

using ContributionQueuePublisher = mojom::ContributionQueuePublisher;
using ContributionQueuePublisherPtr =
    mojom::ContributionQueuePublisherPtr;
using ContributionQueuePublisherList =
    std::vector<ContributionQueuePublisherPtr>;

using ContributionStep = mojom::ContributionStep;

using CredsBatch = mojom::CredsBatch;
using CredsBatchPtr = mojom::CredsBatchPtr;
using CredsBatchList = std::vector<CredsBatchPtr>;

using CredsBatchType = mojom::CredsBatchType;

using CredsBatchStatus = mojom::CredsBatchStatus;

using EventLog = mojom::EventLog;
using EventLogPtr = mojom::EventLogPtr;
using EventLogs = std::vector<EventLogPtr>;

using Environment = mojom::Environment;

using ExcludeFilter = mojom::ExcludeFilter;

using UpholdWallet = mojom::UpholdWallet;
using UpholdWalletPtr = mojom::UpholdWalletPtr;

using InlineTipsPlatforms = mojom::InlineTipsPlatforms;

using MediaEventInfo = mojom::MediaEventInfo;
using MediaEventInfoPtr = mojom::MediaEventInfoPtr;

using MonthlyReportInfo = mojom::MonthlyReportInfo;
using MonthlyReportInfoPtr = mojom::MonthlyReportInfoPtr;

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

using PromotionStatus = mojom::PromotionStatus;

using PublisherBanner = mojom::PublisherBanner;
using PublisherBannerPtr = mojom::PublisherBannerPtr;

using PublisherInfo = mojom::PublisherInfo;
using PublisherInfoPtr = mojom::PublisherInfoPtr;
using PublisherInfoList = std::vector<PublisherInfoPtr>;

using PublisherStatus = mojom::PublisherStatus;

using PublisherExclude = mojom::PublisherExclude;

using RecurringTip = mojom::RecurringTip;
using RecurringTipPtr = mojom::RecurringTipPtr;

using ReportType = mojom::ReportType;

using Result = mojom::Result;

using RewardsInternalsInfo = mojom::RewardsInternalsInfo;
using RewardsInternalsInfoPtr = mojom::RewardsInternalsInfoPtr;

using RewardsParameters = mojom::RewardsParameters;
using RewardsParametersPtr = mojom::RewardsParametersPtr;

using RewardsType = mojom::RewardsType;

using ServerPublisherInfo = mojom::ServerPublisherInfo;
using ServerPublisherInfoPtr = mojom::ServerPublisherInfoPtr;

using SKUOrder = mojom::SKUOrder;
using SKUOrderPtr = mojom::SKUOrderPtr;

using SKUOrderStatus = mojom::SKUOrderStatus;

using SKUOrderItem = mojom::SKUOrderItem;
using SKUOrderItemPtr = mojom::SKUOrderItemPtr;
using SKUOrderItemList = std::vector<SKUOrderItemPtr>;

using SKUOrderItemType = mojom::SKUOrderItemType;

using SKUTransaction = mojom::SKUTransaction;
using SKUTransactionPtr = mojom::SKUTransactionPtr;

using SKUTransactionStatus = mojom::SKUTransactionStatus;

using SKUTransactionType = mojom::SKUTransactionType;

using TransactionReportInfo = mojom::TransactionReportInfo;
using TransactionReportInfoPtr = mojom::TransactionReportInfoPtr;
using TransactionReportInfoList = std::vector<TransactionReportInfoPtr>;

using UnblindedToken = mojom::UnblindedToken;
using UnblindedTokenPtr = mojom::UnblindedTokenPtr;
using UnblindedTokenList = std::vector<UnblindedTokenPtr>;

using UrlMethod = mojom::UrlMethod;

using UrlRequest = mojom::UrlRequest;
using UrlRequestPtr = mojom::UrlRequestPtr;

using UrlResponse = mojom::UrlResponse;
using UrlResponsePtr = mojom::UrlResponsePtr;

using VisitData = mojom::VisitData;
using VisitDataPtr = mojom::VisitDataPtr;

using WalletStatus = mojom::WalletStatus;

/**
 * DATABASE
 */
using DBCommand = ledger_database::mojom::DBCommand;
using DBCommandPtr = ledger_database::mojom::DBCommandPtr;

using DBCommandBinding = ledger_database::mojom::DBCommandBinding;
using DBCommandBindingPtr = ledger_database::mojom::DBCommandBindingPtr;

using DBCommandResult = ledger_database::mojom::DBCommandResult;
using DBCommandResultPtr = ledger_database::mojom::DBCommandResultPtr;

using DBCommandResponse = ledger_database::mojom::DBCommandResponse;
using DBCommandResponsePtr = ledger_database::mojom::DBCommandResponsePtr;

using DBRecord = ledger_database::mojom::DBRecord;
using DBRecordPtr = ledger_database::mojom::DBRecordPtr;

using DBTransaction = ledger_database::mojom::DBTransaction;
using DBTransactionPtr = ledger_database::mojom::DBTransactionPtr;

using DBValue = ledger_database::mojom::DBValue;
using DBValuePtr = ledger_database::mojom::DBValuePtr;

}  // namespace type
}  // namespace ledger

#endif  // BAT_LEDGER_MOJOM_STRUCTS_
