/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_MOJOM_STRUCTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_MOJOM_STRUCTS_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger.mojom.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_database.mojom.h"

namespace ledger {
namespace type {

// DEPRECATED. Use the "ledger::mojom" namespace directly, and for clarity,
// generally avoid aliasing standard library containers.

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

using DrainStatus = mojom::DrainStatus;

using EventLog = mojom::EventLog;
using EventLogPtr = mojom::EventLogPtr;
using EventLogs = std::vector<EventLogPtr>;

using Environment = mojom::Environment;

using ExcludeFilter = mojom::ExcludeFilter;

using ExternalWallet = mojom::ExternalWallet;
using ExternalWalletPtr = mojom::ExternalWalletPtr;

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
using PromotionMap = base::flat_map<std::string, PromotionPtr>;

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

using VirtualGrant = mojom::VirtualGrant;
using VirtualGrantPtr = mojom::VirtualGrantPtr;
using VirtualGrants =
    std::multimap<std::string /* creds_id */, VirtualGrantPtr>;

using VisitData = mojom::VisitData;
using VisitDataPtr = mojom::VisitDataPtr;

using WalletStatus = mojom::WalletStatus;

/**
 * DATABASE
 */
using DBCommand = mojom::DBCommand;
using DBCommandPtr = mojom::DBCommandPtr;

using DBCommandBinding = mojom::DBCommandBinding;
using DBCommandBindingPtr = mojom::DBCommandBindingPtr;

using DBCommandResult = mojom::DBCommandResult;
using DBCommandResultPtr = mojom::DBCommandResultPtr;

using DBCommandResponse = mojom::DBCommandResponse;
using DBCommandResponsePtr = mojom::DBCommandResponsePtr;

using DBRecord = mojom::DBRecord;
using DBRecordPtr = mojom::DBRecordPtr;

using DBTransaction = mojom::DBTransaction;
using DBTransactionPtr = mojom::DBTransactionPtr;

using DBValue = mojom::DBValue;
using DBValuePtr = mojom::DBValuePtr;

}  // namespace type
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_MOJOM_STRUCTS_H_
