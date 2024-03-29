/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_H_

#include <stdint.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "brave/components/brave_rewards/core/database/database_activity_info.h"
#include "brave/components/brave_rewards/core/database/database_balance_report.h"
#include "brave/components/brave_rewards/core/database/database_contribution_info.h"
#include "brave/components/brave_rewards/core/database/database_contribution_queue.h"
#include "brave/components/brave_rewards/core/database/database_creds_batch.h"
#include "brave/components/brave_rewards/core/database/database_event_log.h"
#include "brave/components/brave_rewards/core/database/database_external_transactions.h"
#include "brave/components/brave_rewards/core/database/database_media_publisher_info.h"
#include "brave/components/brave_rewards/core/database/database_publisher_info.h"
#include "brave/components/brave_rewards/core/database/database_publisher_prefix_list.h"
#include "brave/components/brave_rewards/core/database/database_recurring_tip.h"
#include "brave/components/brave_rewards/core/database/database_server_publisher_info.h"
#include "brave/components/brave_rewards/core/database/database_sku_order.h"
#include "brave/components/brave_rewards/core/database/database_sku_transaction.h"
#include "brave/components/brave_rewards/core/database/database_unblinded_token.h"
#include "brave/components/brave_rewards/core/publisher/prefix_list_reader.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace database {

class Database {
 public:
  explicit Database(RewardsEngine& engine);
  virtual ~Database();

  /**
   * ACTIVITY INFO
   */
  void SaveActivityInfo(mojom::PublisherInfoPtr info, ResultCallback callback);

  void NormalizeActivityInfoList(std::vector<mojom::PublisherInfoPtr> list,
                                 ResultCallback callback);

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           mojom::ActivityInfoFilterPtr filter,
                           GetActivityInfoListCallback callback);

  void DeleteActivityInfo(const std::string& publisher_key,
                          ResultCallback callback);

  void GetPublishersVisitedCount(base::OnceCallback<void(int)> callback);

  /**
   * BALANCE REPORT
   */
  void SaveBalanceReportInfo(mojom::BalanceReportInfoPtr info,
                             ResultCallback callback);

  void SaveBalanceReportInfoList(std::vector<mojom::BalanceReportInfoPtr> list,
                                 ResultCallback callback);

  void SaveBalanceReportInfoItem(mojom::ActivityMonth month,
                                 int year,
                                 mojom::ReportType type,
                                 double amount,
                                 ResultCallback callback);

  void GetBalanceReportInfo(
      mojom::ActivityMonth month,
      int year,
      mojom::RewardsEngine::GetBalanceReportCallback callback);

  void GetAllBalanceReports(GetBalanceReportListCallback callback);

  void DeleteAllBalanceReports(ResultCallback callback);

  /**
   * CONTRIBUTION INFO
   */
  void SaveContributionInfo(mojom::ContributionInfoPtr info,
                            ResultCallback callback);

  virtual void GetContributionInfo(const std::string& contribution_id,
                                   GetContributionInfoCallback callback);

  void GetOneTimeTips(const mojom::ActivityMonth month,
                      const int year,
                      GetOneTimeTipsCallback callback);

  void GetNotCompletedContributions(ContributionInfoListCallback callback);

  void UpdateContributionInfoStep(const std::string& contribution_id,
                                  mojom::ContributionStep step,
                                  ResultCallback callback);

  void UpdateContributionInfoStepAndCount(const std::string& contribution_id,
                                          mojom::ContributionStep step,
                                          int32_t retry_count,
                                          ResultCallback callback);

  void UpdateContributionInfoContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ResultCallback callback);

  void GetAllContributions(ContributionInfoListCallback callback);

  void FinishAllInProgressContributions(ResultCallback callback);

  /**
   * CONTRIBUTION QUEUE
   */
  void SaveContributionQueue(mojom::ContributionQueuePtr info,
                             ResultCallback callback);

  void GetFirstContributionQueue(GetFirstContributionQueueCallback callback);

  void MarkContributionQueueAsComplete(const std::string& id,
                                       ResultCallback callback);

  /**
   * CREDS BATCH
   */
  void SaveCredsBatch(mojom::CredsBatchPtr info, ResultCallback callback);

  void GetCredsBatchByTrigger(const std::string& trigger_id,
                              const mojom::CredsBatchType trigger_type,
                              GetCredsBatchCallback callback);

  void SaveSignedCreds(mojom::CredsBatchPtr info, ResultCallback callback);

  void GetAllCredsBatches(GetCredsBatchListCallback callback);

  void UpdateCredsBatchStatus(const std::string& trigger_id,
                              mojom::CredsBatchType trigger_type,
                              mojom::CredsBatchStatus status,
                              ResultCallback callback);

  void UpdateCredsBatchesStatus(const std::vector<std::string>& trigger_ids,
                                mojom::CredsBatchType trigger_type,
                                mojom::CredsBatchStatus status,
                                ResultCallback callback);

  void GetCredsBatchesByTriggers(const std::vector<std::string>& trigger_ids,
                                 GetCredsBatchListCallback callback);

  /**
   * EVENT LOG
   */
  void SaveEventLog(const std::string& key, const std::string& value);

  void SaveEventLogs(const std::map<std::string, std::string>& records,
                     ResultCallback callback);

  void GetLastEventLogs(GetEventLogsCallback callback);

  /**
   * EXTERNAL TRANSACTIONS
   */
  void SaveExternalTransaction(mojom::ExternalTransactionPtr, ResultCallback);

  void GetExternalTransaction(const std::string& contribution_id,
                              const std::string& destination,
                              GetExternalTransactionCallback);

  /**
   * MEDIA PUBLISHER INFO
   */
  void SaveMediaPublisherInfo(const std::string& media_key,
                              const std::string& publisher_key,
                              ResultCallback callback);

  void GetMediaPublisherInfo(const std::string& media_key,
                             PublisherInfoCallback callback);

  /**
   * PUBLISHER INFO
   */
  void SavePublisherInfo(mojom::PublisherInfoPtr publisher_info,
                         ResultCallback callback);

  void GetPublisherInfo(const std::string& publisher_key,
                        GetPublisherInfoCallback callback);

  void GetPanelPublisherInfo(mojom::ActivityInfoFilterPtr filter,
                             GetPublisherPanelInfoCallback callback);

  void RestorePublishers(ResultCallback callback);

  void GetExcludedList(GetExcludedListCallback callback);

  /**
   * RECURRING TIPS
   */

  // DEPRECATED
  void SaveRecurringTip(mojom::RecurringTipPtr info, ResultCallback callback);

  void SetMonthlyContribution(const std::string& publisher_id,
                              double amount,
                              base::OnceCallback<void(bool)> callback);

  void AdvanceMonthlyContributionDates(
      const std::vector<std::string>& publisher_ids,
      base::OnceCallback<void(bool)> callback);

  void GetNextMonthlyContributionTime(
      base::OnceCallback<void(std::optional<base::Time>)> callback);

  void GetRecurringTips(GetRecurringTipsCallback callback);

  void RemoveRecurringTip(const std::string& publisher_key,
                          ResultCallback callback);

  /**
   * SERVER PUBLISHER INFO
   */
  void SearchPublisherPrefixList(const std::string& publisher_key,
                                 SearchPublisherPrefixListCallback callback);

  void ResetPublisherPrefixList(publisher::PrefixListReader reader,
                                ResultCallback callback);

  void InsertServerPublisherInfo(const mojom::ServerPublisherInfo& server_info,
                                 ResultCallback callback);

  void DeleteExpiredServerPublisherInfo(int64_t max_age_seconds,
                                        ResultCallback callback);

  void GetServerPublisherInfo(const std::string& publisher_key,
                              GetServerPublisherInfoCallback callback);

  /**
   * SKU ORDER
   */
  void SaveSKUOrder(mojom::SKUOrderPtr order, ResultCallback callback);

  void UpdateSKUOrderStatus(const std::string& order_id,
                            mojom::SKUOrderStatus status,
                            ResultCallback callback);

  void GetSKUOrder(const std::string& order_id, GetSKUOrderCallback callback);

  void GetSKUOrderByContributionId(const std::string& contribution_id,
                                   GetSKUOrderCallback callback);

  void SaveContributionIdForSKUOrder(const std::string& order_id,
                                     const std::string& contribution_id,
                                     ResultCallback callback);

  /**
   * SKU TRANSACTION
   */
  void SaveSKUTransaction(mojom::SKUTransactionPtr transaction,
                          ResultCallback callback);

  void SaveSKUExternalTransaction(const std::string& transaction_id,
                                  const std::string& external_transaction_id,
                                  ResultCallback callback);

  void GetSKUTransactionByOrderId(const std::string& order_id,
                                  GetSKUTransactionCallback callback);

  /**
   * UNBLINDED TOKEN
   */
  void SaveUnblindedTokenList(std::vector<mojom::UnblindedTokenPtr> list,
                              ResultCallback callback);

  void MarkUnblindedTokensAsSpent(const std::vector<std::string>& ids,
                                  mojom::RewardsType redeem_type,
                                  const std::string& redeem_id,
                                  ResultCallback callback);

  void MarkUnblindedTokensAsReserved(const std::vector<std::string>& ids,
                                     const std::string& redeem_id,
                                     ResultCallback callback);

  void MarkUnblindedTokensAsSpendable(const std::string& redeem_id,
                                      ResultCallback callback);

  void GetSpendableUnblindedTokens(GetUnblindedTokenListCallback callback);

  void GetReservedUnblindedTokens(const std::string& redeem_id,
                                  GetUnblindedTokenListCallback callback);

  virtual void GetSpendableUnblindedTokensByBatchTypes(
      const std::vector<mojom::CredsBatchType>& batch_types,
      GetUnblindedTokenListCallback callback);

 private:
  const raw_ref<RewardsEngine> engine_;
  DatabaseActivityInfo activity_info_;
  DatabaseBalanceReport balance_report_;
  DatabaseContributionInfo contribution_info_;
  DatabaseContributionQueue contribution_queue_;
  DatabaseCredsBatch creds_batch_;
  DatabaseEventLog event_log_;
  DatabaseExternalTransactions external_transactions_;
  DatabaseMediaPublisherInfo media_publisher_info_;
  DatabasePublisherInfo publisher_info_;
  DatabasePublisherPrefixList publisher_prefix_list_;
  DatabaseRecurringTip recurring_tip_;
  DatabaseServerPublisherInfo server_publisher_info_;
  DatabaseSKUOrder sku_order_;
  DatabaseSKUTransaction sku_transaction_;
  DatabaseUnblindedToken unblinded_token_;
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_H_
