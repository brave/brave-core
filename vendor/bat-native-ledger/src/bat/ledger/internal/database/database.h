/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_H_

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

  void Initialize(bool execute_create_script,
                  ledger::LegacyResultCallback callback);

  void Close(ledger::LegacyResultCallback callback);

  /**
   * ACTIVITY INFO
   */
  void SaveActivityInfo(type::PublisherInfoPtr info,
                        ledger::LegacyResultCallback callback);

  void NormalizeActivityInfoList(type::PublisherInfoList list,
                                 ledger::LegacyResultCallback callback);

  void GetActivityInfoList(
      uint32_t start,
      uint32_t limit,
      type::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoListCallback callback);

  void DeleteActivityInfo(const std::string& publisher_key,
                          ledger::LegacyResultCallback callback);

  /**
   * BALANCE REPORT
   */
  void SaveBalanceReportInfo(type::BalanceReportInfoPtr info,
                             ledger::LegacyResultCallback callback);

  void SaveBalanceReportInfoList(type::BalanceReportInfoList list,
                                 ledger::LegacyResultCallback callback);

  void SaveBalanceReportInfoItem(type::ActivityMonth month,
                                 int year,
                                 type::ReportType type,
                                 double amount,
                                 ledger::LegacyResultCallback callback);

  void GetBalanceReportInfo(
      type::ActivityMonth month,
      int year,
      ledger::GetBalanceReportCallback callback);

  void GetAllBalanceReports(ledger::GetBalanceReportListCallback callback);

  void DeleteAllBalanceReports(ledger::LegacyResultCallback callback);

  /**
   * CONTRIBUTION INFO
   */
  void SaveContributionInfo(type::ContributionInfoPtr info,
                            ledger::LegacyResultCallback callback);

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

  void UpdateContributionInfoStep(const std::string& contribution_id,
                                  type::ContributionStep step,
                                  ledger::LegacyResultCallback callback);

  void UpdateContributionInfoStepAndCount(
      const std::string& contribution_id,
      type::ContributionStep step,
      int32_t retry_count,
      ledger::LegacyResultCallback callback);

  void UpdateContributionInfoContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ledger::LegacyResultCallback callback);

  void GetAllContributions(ledger::ContributionInfoListCallback callback);

  void FinishAllInProgressContributions(ledger::LegacyResultCallback callback);

  /**
   * CONTRIBUTION QUEUE
   */
  void SaveContributionQueue(type::ContributionQueuePtr info,
                             ledger::LegacyResultCallback callback);

  void GetFirstContributionQueue(
      GetFirstContributionQueueCallback callback);

  void MarkContributionQueueAsComplete(const std::string& id,
                                       ledger::LegacyResultCallback callback);

  /**
   * CREDS BATCH
   */
  void SaveCredsBatch(type::CredsBatchPtr info,
                      ledger::LegacyResultCallback callback);

  void GetCredsBatchByTrigger(
      const std::string& trigger_id,
      const type::CredsBatchType trigger_type,
      GetCredsBatchCallback callback);

  void SaveSignedCreds(type::CredsBatchPtr info,
                       ledger::LegacyResultCallback callback);

  void GetAllCredsBatches(GetCredsBatchListCallback callback);

  void UpdateCredsBatchStatus(const std::string& trigger_id,
                              type::CredsBatchType trigger_type,
                              type::CredsBatchStatus status,
                              ledger::LegacyResultCallback callback);

  void UpdateCredsBatchesStatus(const std::vector<std::string>& trigger_ids,
                                type::CredsBatchType trigger_type,
                                type::CredsBatchStatus status,
                                ledger::LegacyResultCallback callback);

  void GetCredsBatchesByTriggers(
      const std::vector<std::string>& trigger_ids,
      GetCredsBatchListCallback callback);

  /**
   * EVENT LOG
   */
  void SaveEventLog(const std::string& key, const std::string& value);

  void SaveEventLogs(const std::map<std::string, std::string>& records,
                     ledger::LegacyResultCallback callback);

  void GetLastEventLogs(ledger::GetEventLogsCallback callback);

  /**
   * MEDIA PUBLISHER INFO
   */
  void SaveMediaPublisherInfo(const std::string& media_key,
                              const std::string& publisher_key,
                              ledger::LegacyResultCallback callback);

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
  void SavePendingContribution(type::PendingContributionList list,
                               ledger::LegacyResultCallback callback);

  void GetPendingContributionsTotal(
      ledger::PendingContributionsTotalCallback callback);

  void GetPendingContributions(
      ledger::PendingContributionInfoListCallback callback);

  void GetUnverifiedPublishersForPendingContributions(
      ledger::UnverifiedPublishersCallback callback);

  void RemovePendingContribution(uint64_t id,
                                 ledger::LegacyResultCallback callback);

  void RemoveAllPendingContributions(ledger::LegacyResultCallback callback);

  /**
   * PROCESSED PUBLISHER
   */
  void SaveProcessedPublisherList(const std::vector<std::string>& list,
                                  ledger::LegacyResultCallback callback);

  void WasPublisherProcessed(const std::string& publisher_key,
                             ledger::LegacyResultCallback callback);

  /**
   * PROMOTION
   */
  virtual void SavePromotion(type::PromotionPtr info,
                             ledger::LegacyResultCallback callback);

  void GetPromotion(
      const std::string& id,
      GetPromotionCallback callback);

  virtual void GetAllPromotions(ledger::GetAllPromotionsCallback callback);

  void SavePromotionClaimId(const std::string& promotion_id,
                            const std::string& claim_id,
                            ledger::LegacyResultCallback callback);

  void UpdatePromotionStatus(const std::string& promotion_id,
                             type::PromotionStatus status,
                             ledger::LegacyResultCallback callback);

  void UpdatePromotionsStatus(const std::vector<std::string>& promotion_ids,
                              type::PromotionStatus status,
                              ledger::LegacyResultCallback callback);

  void PromotionCredentialCompleted(const std::string& promotion_id,
                                    ledger::LegacyResultCallback callback);

  void GetPromotionList(
      const std::vector<std::string>& ids,
      client::GetPromotionListCallback callback);

  void UpdatePromotionsBlankPublicKey(const std::vector<std::string>& ids,
                                      ledger::LegacyResultCallback callback);

  /**
   * PUBLISHER INFO
   */
  void SavePublisherInfo(type::PublisherInfoPtr publisher_info,
                         ledger::LegacyResultCallback callback);

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
  void SaveRecurringTip(type::RecurringTipPtr info,
                        ledger::LegacyResultCallback callback);

  void GetRecurringTips(ledger::PublisherInfoListCallback callback);

  void RemoveRecurringTip(const std::string& publisher_key,
                          ledger::LegacyResultCallback callback);

  /**
   * SERVER PUBLISHER INFO
   */
  void SearchPublisherPrefixList(
      const std::string& publisher_key,
      SearchPublisherPrefixListCallback callback);

  void ResetPublisherPrefixList(
      std::unique_ptr<publisher::PrefixListReader> reader,
      ledger::LegacyResultCallback callback);

  void InsertServerPublisherInfo(const type::ServerPublisherInfo& server_info,
                                 ledger::LegacyResultCallback callback);

  void DeleteExpiredServerPublisherInfo(int64_t max_age_seconds,
                                        ledger::LegacyResultCallback callback);

  void GetServerPublisherInfo(
      const std::string& publisher_key,
      client::GetServerPublisherInfoCallback callback);

  /**
   * SKU ORDER
   */
  void SaveSKUOrder(type::SKUOrderPtr order,
                    ledger::LegacyResultCallback callback);

  void UpdateSKUOrderStatus(const std::string& order_id,
                            type::SKUOrderStatus status,
                            ledger::LegacyResultCallback callback);

  void GetSKUOrder(
      const std::string& order_id,
      GetSKUOrderCallback callback);

  void GetSKUOrderByContributionId(
      const std::string& contribution_id,
      GetSKUOrderCallback callback);

  void SaveContributionIdForSKUOrder(const std::string& order_id,
                                     const std::string& contribution_id,
                                     ledger::LegacyResultCallback callback);

  /**
   * SKU TRANSACTION
   */
  void SaveSKUTransaction(type::SKUTransactionPtr transaction,
                          ledger::LegacyResultCallback callback);

  void SaveSKUExternalTransaction(const std::string& transaction_id,
                                  const std::string& external_transaction_id,
                                  ledger::LegacyResultCallback callback);

  void GetSKUTransactionByOrderId(
      const std::string& order_id,
      GetSKUTransactionCallback callback);

  /**
   * UNBLINDED TOKEN
   */
  void SaveUnblindedTokenList(type::UnblindedTokenList list,
                              ledger::LegacyResultCallback callback);

  void MarkUnblindedTokensAsSpent(const std::vector<std::string>& ids,
                                  type::RewardsType redeem_type,
                                  const std::string& redeem_id,
                                  ledger::LegacyResultCallback callback);

  void MarkUnblindedTokensAsReserved(const std::vector<std::string>& ids,
                                     const std::string& redeem_id,
                                     ledger::LegacyResultCallback callback);

  void MarkUnblindedTokensAsSpendable(const std::string& redeem_id,
                                      ledger::LegacyResultCallback callback);

  void GetSpendableUnblindedTokens(GetUnblindedTokenListCallback callback);

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

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_H_
