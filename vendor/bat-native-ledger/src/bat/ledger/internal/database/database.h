/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_H_
#define BRAVELEDGER_DATABASE_DATABASE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_database {

class DatabaseInitialize;
class DatabaseActivityInfo;
class DatabaseCredsBatch;
class DatabaseContributionInfo;
class DatabaseContributionQueue;
class DatabaseMediaPublisherInfo;
class DatabaseMultiTables;
class DatabasePendingContribution;
class DatabasePromotion;
class DatabasePublisherInfo;
class DatabaseRecurringTip;
class DatabaseServerPublisherInfo;
class DatabaseSKUOrder;
class DatabaseSKUTransaction;
class DatabaseUnblindedToken;

class Database {
 public:
  explicit Database(bat_ledger::LedgerImpl* ledger);
  ~Database();

  void Initialize(
      const bool execute_create_script,
      ledger::ResultCallback callback);

  /**
   * ACTIVITY INFO
   */
  void SaveActivityInfo(
      ledger::PublisherInfoPtr info,
      ledger::ResultCallback callback);

  void SaveActivityInfoList(
      ledger::PublisherInfoList list,
      ledger::ResultCallback callback);

  void GetActivityInfoList(
      uint32_t start,
      uint32_t limit,
      ledger::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoListCallback callback);

  void DeleteActivityInfo(
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  /**
   * CONTRIBUTION INFO
   */
  void SaveContributionInfo(
      ledger::ContributionInfoPtr info,
      ledger::ResultCallback callback);

  void GetContributionInfo(
      const std::string& contribution_id,
      ledger::GetContributionInfoCallback callback);

  void GetOneTimeTips(
      const ledger::ActivityMonth month,
      const int year,
      ledger::PublisherInfoListCallback callback);

  void GetContributionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback);

  void GetIncompleteContributions(
      const ledger::ContributionProcessor processor,
      ledger::ContributionInfoListCallback callback);

  void UpdateContributionInfoStepAndCount(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      const int32_t retry_count,
      ledger::ResultCallback callback);

  void UpdateContributionInfoContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  void GetAllContributions(ledger::ContributionInfoListCallback callback);

  /**
   * CONTRIBUTION QUEUE
   */
  void SaveContributionQueue(
      ledger::ContributionQueuePtr info,
      ledger::ResultCallback callback);

  void GetFirstContributionQueue(
      ledger::GetFirstContributionQueueCallback callback);

  void DeleteContributionQueue(
      const uint64_t id,
      ledger::ResultCallback callback);

  /**
   * CREDS BATCH
   */
  void SaveCredsBatch(
      ledger::CredsBatchPtr info,
      ledger::ResultCallback callback);

  void GetCredsBatchByTrigger(
      const std::string& trigger_id,
      const ledger::CredsBatchType trigger_type,
      ledger::GetCredsBatchCallback callback);

  void SaveSignedCreds(
      ledger::CredsBatchPtr info,
      ledger::ResultCallback callback);

  void GetAllCredsBatches(ledger::GetAllCredsBatchCallback callback);

  void UpdateCredsBatchStatus(
      const std::string& trigger_id,
      const ledger::CredsBatchType trigger_type,
      const ledger::CredsBatchStatus status,
      ledger::ResultCallback callback);

  /**
   * MEDIA PUBLISHER INFO
   */
  void SaveMediaPublisherInfo(
      const std::string& media_key,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  void GetMediaPublisherInfo(
      const std::string& media_key,
      ledger::PublisherInfoCallback callback);

  /**
   * MULTI TABLE
   * for queries that are not limited to one table
   */
  void GetTransactionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback);

  /**
   * PENDING CONTRIBUTION
   */
  void SavePendingContribution(
      ledger::PendingContributionList list,
      ledger::ResultCallback callback);

  void GetPendingContributionsTotal(
      ledger::PendingContributionsTotalCallback callback);

  void GetPendingContributions(
      ledger::PendingContributionInfoListCallback callback);

  void RemovePendingContribution(
      const uint64_t id,
      ledger::ResultCallback callback);

  void RemoveAllPendingContributions(ledger::ResultCallback callback);

  /**
   * PROMOTION
   */
  void SavePromotion(
      ledger::PromotionPtr info,
      ledger::ResultCallback callback);

  void GetPromotion(
      const std::string& id,
      ledger::GetPromotionCallback callback);

  void GetAllPromotions(ledger::GetAllPromotionsCallback callback);

  void DeletePromotionList(
      const std::vector<std::string>& ids,
      ledger::ResultCallback callback);

  void SavePromotionClaimId(
      const std::string& promotion_id,
      const std::string& claim_id,
      ledger::ResultCallback callback);

  void UpdatePromotionStatus(
      const std::string& promotion_id,
      const ledger::PromotionStatus status,
      ledger::ResultCallback callback);

  void PromotionCredentialCompleted(
      const std::string& promotion_id,
      ledger::ResultCallback callback);

  void GetPromotionList(
      const std::vector<std::string>& ids,
      ledger::GetPromotionListCallback callback);

  void GetPromotionListByType(
      const std::vector<ledger::PromotionType>& types,
      ledger::GetPromotionListCallback callback);

  /**
   * PUBLISHER INFO
   */
  void SavePublisherInfo(
      ledger::PublisherInfoPtr publisher_info,
      ledger::ResultCallback callback);

  void GetPublisherInfo(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback);

  void GetPanelPublisherInfo(
      ledger::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoCallback callback);

  void RestorePublishers(ledger::ResultCallback callback);

  void GetExcludedList(ledger::PublisherInfoListCallback callback);

  /**
   * RECURRING TIPS
   */
  void SaveRecurringTip(
      ledger::RecurringTipPtr info,
      ledger::ResultCallback callback);

  void GetRecurringTips(ledger::PublisherInfoListCallback callback);

  void RemoveRecurringTip(
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  /**
   * SERVER PUBLISHER INFO
   */
  void ClearServerPublisherList(ledger::ResultCallback callback);

  void InsertServerPublisherList(
      const std::vector<ledger::ServerPublisherPartial>& list,
      ledger::ResultCallback callback);

  void InsertPublisherBannerList(
      const std::vector<ledger::PublisherBanner>& list,
      ledger::ResultCallback callback);

  void GetServerPublisherInfo(
      const std::string& publisher_key,
      ledger::GetServerPublisherInfoCallback callback);

  /**
   * SKU ORDER
   */
  void SaveSKUOrder(ledger::SKUOrderPtr order, ledger::ResultCallback callback);

  void UpdateSKUOrderStatus(
      const std::string& order_id,
      const ledger::SKUOrderStatus status,
      ledger::ResultCallback callback);

  void GetSKUOrder(
      const std::string& order_id,
      ledger::GetSKUOrderCallback callback);

  /**
   * SKU TRANSACTION
   */
  void SaveSKUTransaction(
      ledger::SKUTransactionPtr transaction,
      ledger::ResultCallback callback);

  void SaveSKUExternalTransaction(
      const std::string& transaction_id,
      const std::string& external_transaction_id,
      ledger::ResultCallback callback);

  /**
   * UNBLINDED TOKEN
   */
  void SaveUnblindedTokenList(
      ledger::UnblindedTokenList list,
      ledger::ResultCallback callback);

  void GetAllUnblindedTokens(
      ledger::GetUnblindedTokenListCallback callback);

  void DeleteUnblindedTokens(
      const std::vector<std::string>& ids,
      ledger::ResultCallback callback);

  void GetUnblindedTokensByTriggerIds(
      const std::vector<std::string>& trigger_ids,
      ledger::GetUnblindedTokenListCallback callback);

  void CheckUnblindedTokensExpiration(ledger::ResultCallback callback);

 private:
  std::unique_ptr<DatabaseInitialize> initialize_;
  std::unique_ptr<DatabaseActivityInfo> activity_info_;
  std::unique_ptr<DatabaseContributionInfo> contribution_info_;
  std::unique_ptr<DatabaseContributionQueue> contribution_queue_;
  std::unique_ptr<DatabaseCredsBatch> creds_batch_;
  std::unique_ptr<DatabasePendingContribution> pending_contribution_;
  std::unique_ptr<DatabasePromotion> promotion_;
  std::unique_ptr<DatabaseMediaPublisherInfo> media_publisher_info_;
  std::unique_ptr<DatabaseMultiTables> multi_tables_;
  std::unique_ptr<DatabasePublisherInfo> publisher_info_;
  std::unique_ptr<DatabaseRecurringTip> recurring_tip_;
  std::unique_ptr<DatabaseServerPublisherInfo> server_publisher_info_;
  std::unique_ptr<DatabaseSKUOrder> sku_order_;
  std::unique_ptr<DatabaseSKUTransaction> sku_transaction_;
  std::unique_ptr<DatabaseUnblindedToken> unblinded_token_;
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_H_
