/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_H_
#define BRAVELEDGER_DATABASE_DATABASE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_activity_info.h"
#include "bat/ledger/internal/database/database_balance_report.h"
#include "bat/ledger/internal/database/database_contribution_info.h"
#include "bat/ledger/internal/database/database_contribution_queue.h"
#include "bat/ledger/internal/database/database_creds_batch.h"
#include "bat/ledger/internal/database/database_event_log.h"
#include "bat/ledger/internal/database/database_initialize.h"
#include "bat/ledger/internal/database/database_media_publisher_info.h"
#include "bat/ledger/internal/database/database_multi_tables.h"
#include "bat/ledger/internal/database/database_pending_contribution.h"
#include "bat/ledger/internal/database/database_processed_publisher.h"
#include "bat/ledger/internal/database/database_promotion.h"
#include "bat/ledger/internal/database/database_publisher_info.h"
#include "bat/ledger/internal/database/database_publisher_prefix_list.h"
#include "bat/ledger/internal/database/database_recurring_tip.h"
#include "bat/ledger/internal/database/database_server_publisher_info.h"
#include "bat/ledger/internal/database/database_sku_order.h"
#include "bat/ledger/internal/database/database_sku_transaction.h"
#include "bat/ledger/internal/database/database_unblinded_token.h"
#include "bat/ledger/internal/publisher/prefix_list_reader.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace database {

class Database {
 public:
  explicit Database(LedgerImpl* ledger);
  virtual ~Database();

  void Initialize(
      const bool execute_create_script,
      ledger::ResultCallback callback);

  void Close(ledger::ResultCallback callback);

  /**
   * ACTIVITY INFO
   */
  void SaveActivityInfo(
      type::PublisherInfoPtr info,
      ledger::ResultCallback callback);

  void NormalizeActivityInfoList(
      type::PublisherInfoList list,
      ledger::ResultCallback callback);

  void GetActivityInfoList(
      uint32_t start,
      uint32_t limit,
      type::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoListCallback callback);

  void DeleteActivityInfo(
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  /**
   * BALANCE REPORT
   */
  void SaveBalanceReportInfo(
      type::BalanceReportInfoPtr info,
      ledger::ResultCallback callback);

  void SaveBalanceReportInfoList(
      type::BalanceReportInfoList list,
      ledger::ResultCallback callback);

  void SaveBalanceReportInfoItem(
      type::ActivityMonth month,
      int year,
      type::ReportType type,
      double amount,
      ledger::ResultCallback callback);

  void GetBalanceReportInfo(
      type::ActivityMonth month,
      int year,
      ledger::GetBalanceReportCallback callback);

  void GetAllBalanceReports(ledger::GetBalanceReportListCallback callback);

  void DeleteAllBalanceReports(ledger::ResultCallback callback);

  /**
   * CONTRIBUTION INFO
   */
  void SaveContributionInfo(
      type::ContributionInfoPtr info,
      ledger::ResultCallback callback);

  void GetContributionInfo(
      const std::string& contribution_id,
      GetContributionInfoCallback callback);

  void GetOneTimeTips(
      const type::ActivityMonth month,
      const int year,
      ledger::PublisherInfoListCallback callback);

  void GetContributionReport(
      const type::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback);

  void GetNotCompletedContributions(
      ledger::ContributionInfoListCallback callback);

  void UpdateContributionInfoStep(
      const std::string& contribution_id,
      const type::ContributionStep step,
      ledger::ResultCallback callback);

  void UpdateContributionInfoStepAndCount(
      const std::string& contribution_id,
      const type::ContributionStep step,
      const int32_t retry_count,
      ledger::ResultCallback callback);

  void UpdateContributionInfoContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  void GetAllContributions(ledger::ContributionInfoListCallback callback);

  void FinishAllInProgressContributions(ledger::ResultCallback callback);

  /**
   * CONTRIBUTION QUEUE
   */
  void SaveContributionQueue(
      type::ContributionQueuePtr info,
      ledger::ResultCallback callback);

  void GetFirstContributionQueue(
      GetFirstContributionQueueCallback callback);

  void MarkContributionQueueAsComplete(
      const std::string& id,
      ledger::ResultCallback callback);

  /**
   * CREDS BATCH
   */
  void SaveCredsBatch(
      type::CredsBatchPtr info,
      ledger::ResultCallback callback);

  void GetCredsBatchByTrigger(
      const std::string& trigger_id,
      const type::CredsBatchType trigger_type,
      GetCredsBatchCallback callback);

  void SaveSignedCreds(
      type::CredsBatchPtr info,
      ledger::ResultCallback callback);

  void GetAllCredsBatches(GetCredsBatchListCallback callback);

  void UpdateCredsBatchStatus(
      const std::string& trigger_id,
      const type::CredsBatchType trigger_type,
      const type::CredsBatchStatus status,
      ledger::ResultCallback callback);

  void UpdateCredsBatchesStatus(
      const std::vector<std::string>& trigger_ids,
      const type::CredsBatchType trigger_type,
      const type::CredsBatchStatus status,
      ledger::ResultCallback callback);

  void GetCredsBatchesByTriggers(
      const std::vector<std::string>& trigger_ids,
      GetCredsBatchListCallback callback);

  /**
   * EVENT LOG
   */
  void SaveEventLog(const std::string& key, const std::string& value);

  void SaveEventLogs(
      const std::map<std::string, std::string>& records,
      ledger::ResultCallback callback);

  void GetLastEventLogs(ledger::GetEventLogsCallback callback);

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
      const type::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback);

  /**
   * PENDING CONTRIBUTION
   */
  void SavePendingContribution(
      type::PendingContributionList list,
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
   * PROCESSED PUBLISHER
   */
  void SaveProcessedPublisherList(
      const std::vector<std::string>& list,
      ledger::ResultCallback callback);

  void WasPublisherProcessed(
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  /**
   * PROMOTION
   */
  virtual void SavePromotion(
      type::PromotionPtr info,
      ledger::ResultCallback callback);

  void GetPromotion(
      const std::string& id,
      GetPromotionCallback callback);

  virtual void GetAllPromotions(ledger::GetAllPromotionsCallback callback);

  void SavePromotionClaimId(
      const std::string& promotion_id,
      const std::string& claim_id,
      ledger::ResultCallback callback);

  void UpdatePromotionStatus(
      const std::string& promotion_id,
      const type::PromotionStatus status,
      ledger::ResultCallback callback);

  void UpdatePromotionsStatus(
      const std::vector<std::string>& promotion_ids,
      const type::PromotionStatus status,
      ledger::ResultCallback callback);

  void PromotionCredentialCompleted(
      const std::string& promotion_id,
      ledger::ResultCallback callback);

  void GetPromotionList(
      const std::vector<std::string>& ids,
      client::GetPromotionListCallback callback);

  void GetPromotionListByType(
      const std::vector<type::PromotionType>& types,
      client::GetPromotionListCallback callback);

  void UpdatePromotionsBlankPublicKey(
      const std::vector<std::string>& ids,
      ledger::ResultCallback callback);

  /**
   * PUBLISHER INFO
   */
  void SavePublisherInfo(
      type::PublisherInfoPtr publisher_info,
      ledger::ResultCallback callback);

  void GetPublisherInfo(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback);

  void GetPanelPublisherInfo(
      type::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoCallback callback);

  void RestorePublishers(ledger::ResultCallback callback);

  void GetExcludedList(ledger::PublisherInfoListCallback callback);

  /**
   * RECURRING TIPS
   */
  void SaveRecurringTip(
      type::RecurringTipPtr info,
      ledger::ResultCallback callback);

  void GetRecurringTips(ledger::PublisherInfoListCallback callback);

  void RemoveRecurringTip(
      const std::string& publisher_key,
      ledger::ResultCallback callback);

  /**
   * SERVER PUBLISHER INFO
   */
  void SearchPublisherPrefixList(
      const std::string& publisher_key,
      SearchPublisherPrefixListCallback callback);

  void ResetPublisherPrefixList(
      std::unique_ptr<publisher::PrefixListReader> reader,
      ledger::ResultCallback callback);

  void InsertServerPublisherInfo(
      const type::ServerPublisherInfo& server_info,
      ledger::ResultCallback callback);

  void DeleteExpiredServerPublisherInfo(
      const int64_t max_age_seconds,
      ledger::ResultCallback callback);

  void GetServerPublisherInfo(
      const std::string& publisher_key,
      client::GetServerPublisherInfoCallback callback);

  /**
   * SKU ORDER
   */
  void SaveSKUOrder(type::SKUOrderPtr order, ledger::ResultCallback callback);

  void UpdateSKUOrderStatus(
      const std::string& order_id,
      const type::SKUOrderStatus status,
      ledger::ResultCallback callback);

  void GetSKUOrder(
      const std::string& order_id,
      GetSKUOrderCallback callback);

  void GetSKUOrderByContributionId(
      const std::string& contribution_id,
      GetSKUOrderCallback callback);

  void SaveContributionIdForSKUOrder(
      const std::string& order_id,
      const std::string& contribution_id,
      ledger::ResultCallback callback);

  /**
   * SKU TRANSACTION
   */
  void SaveSKUTransaction(
      type::SKUTransactionPtr transaction,
      ledger::ResultCallback callback);

  void SaveSKUExternalTransaction(
      const std::string& transaction_id,
      const std::string& external_transaction_id,
      ledger::ResultCallback callback);

  void GetSKUTransactionByOrderId(
      const std::string& order_id,
      GetSKUTransactionCallback callback);

  /**
   * UNBLINDED TOKEN
   */
  void SaveUnblindedTokenList(
      type::UnblindedTokenList list,
      ledger::ResultCallback callback);

  void MarkUnblindedTokensAsSpent(
      const std::vector<std::string>& ids,
      type::RewardsType redeem_type,
      const std::string& redeem_id,
      ledger::ResultCallback callback);

  void MarkUnblindedTokensAsReserved(
      const std::vector<std::string>& ids,
      const std::string& redeem_id,
      ledger::ResultCallback callback);

  void MarkUnblindedTokensAsSpendable(
      const std::string& redeem_id,
      ledger::ResultCallback callback);

  void GetSpendableUnblindedTokensByTriggerIds(
      const std::vector<std::string>& trigger_ids,
      GetUnblindedTokenListCallback callback);

  void GetReservedUnblindedTokens(
      const std::string& redeem_id,
      GetUnblindedTokenListCallback callback);

  void GetSpendableUnblindedTokensByBatchTypes(
      const std::vector<type::CredsBatchType>& batch_types,
      GetUnblindedTokenListCallback callback);

 private:
  std::unique_ptr<DatabaseInitialize> initialize_;
  std::unique_ptr<DatabaseActivityInfo> activity_info_;
  std::unique_ptr<DatabaseBalanceReport> balance_report_;
  std::unique_ptr<DatabaseContributionInfo> contribution_info_;
  std::unique_ptr<DatabaseContributionQueue> contribution_queue_;
  std::unique_ptr<DatabaseCredsBatch> creds_batch_;
  std::unique_ptr<DatabaseEventLog> event_log_;
  std::unique_ptr<DatabasePendingContribution> pending_contribution_;
  std::unique_ptr<DatabaseProcessedPublisher> processed_publisher_;
  std::unique_ptr<DatabasePromotion> promotion_;
  std::unique_ptr<DatabaseMediaPublisherInfo> media_publisher_info_;
  std::unique_ptr<DatabaseMultiTables> multi_tables_;
  std::unique_ptr<DatabasePublisherInfo> publisher_info_;
  std::unique_ptr<DatabasePublisherPrefixList> publisher_prefix_list_;
  std::unique_ptr<DatabaseRecurringTip> recurring_tip_;
  std::unique_ptr<DatabaseServerPublisherInfo> server_publisher_info_;
  std::unique_ptr<DatabaseSKUOrder> sku_order_;
  std::unique_ptr<DatabaseSKUTransaction> sku_transaction_;
  std::unique_ptr<DatabaseUnblindedToken> unblinded_token_;
  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_H_
