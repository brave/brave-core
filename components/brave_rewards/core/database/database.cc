/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/database/database.h"

#include <optional>
#include <utility>

#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

Database::Database(RewardsEngine& engine)
    : engine_(engine),
      activity_info_(engine),
      balance_report_(engine),
      contribution_info_(engine),
      contribution_queue_(engine),
      creds_batch_(engine),
      event_log_(engine),
      external_transactions_(engine),
      media_publisher_info_(engine),
      publisher_info_(engine),
      publisher_prefix_list_(engine),
      recurring_tip_(engine),
      server_publisher_info_(engine),
      sku_order_(engine),
      sku_transaction_(engine),
      unblinded_token_(engine) {}

Database::~Database() = default;

/**
 * ACTIVITY INFO
 */
void Database::SaveActivityInfo(mojom::PublisherInfoPtr info,
                                ResultCallback callback) {
  activity_info_.InsertOrUpdate(std::move(info), std::move(callback));
}

void Database::NormalizeActivityInfoList(
    std::vector<mojom::PublisherInfoPtr> list,
    ResultCallback callback) {
  activity_info_.NormalizeList(std::move(list), std::move(callback));
}

void Database::GetActivityInfoList(uint32_t start,
                                   uint32_t limit,
                                   mojom::ActivityInfoFilterPtr filter,
                                   GetActivityInfoListCallback callback) {
  activity_info_.GetRecordsList(start, limit, std::move(filter),
                                std::move(callback));
}

void Database::DeleteActivityInfo(const std::string& publisher_key,
                                  ResultCallback callback) {
  activity_info_.DeleteRecord(publisher_key, std::move(callback));
}

void Database::GetPublishersVisitedCount(
    base::OnceCallback<void(int)> callback) {
  activity_info_.GetPublishersVisitedCount(std::move(callback));
}

/**
 * BALANCE REPORT INFO
 */
void Database::SaveBalanceReportInfo(mojom::BalanceReportInfoPtr info,
                                     ResultCallback callback) {
  balance_report_.InsertOrUpdate(std::move(info), std::move(callback));
}

void Database::SaveBalanceReportInfoList(
    std::vector<mojom::BalanceReportInfoPtr> list,
    ResultCallback callback) {
  balance_report_.InsertOrUpdateList(std::move(list), std::move(callback));
}

void Database::SaveBalanceReportInfoItem(mojom::ActivityMonth month,
                                         int year,
                                         mojom::ReportType type,
                                         double amount,
                                         ResultCallback callback) {
  balance_report_.SetAmount(month, year, type, amount, std::move(callback));
}

void Database::GetBalanceReportInfo(
    mojom::ActivityMonth month,
    int year,
    mojom::RewardsEngine::GetBalanceReportCallback callback) {
  balance_report_.GetRecord(month, year, std::move(callback));
}

void Database::GetAllBalanceReports(GetBalanceReportListCallback callback) {
  balance_report_.GetAllRecords(std::move(callback));
}

void Database::DeleteAllBalanceReports(ResultCallback callback) {
  balance_report_.DeleteAllRecords(std::move(callback));
}

/**
 * CONTRIBUTION INFO
 */
void Database::SaveContributionInfo(mojom::ContributionInfoPtr info,
                                    ResultCallback callback) {
  contribution_info_.InsertOrUpdate(std::move(info), std::move(callback));
}

void Database::GetContributionInfo(const std::string& contribution_id,
                                   GetContributionInfoCallback callback) {
  contribution_info_.GetRecord(contribution_id, std::move(callback));
}

void Database::GetAllContributions(ContributionInfoListCallback callback) {
  contribution_info_.GetAllRecords(std::move(callback));
}

void Database::GetOneTimeTips(const mojom::ActivityMonth month,
                              const int year,
                              GetOneTimeTipsCallback callback) {
  contribution_info_.GetOneTimeTips(month, year, std::move(callback));
}

void Database::GetNotCompletedContributions(
    ContributionInfoListCallback callback) {
  contribution_info_.GetNotCompletedRecords(std::move(callback));
}

void Database::UpdateContributionInfoStep(const std::string& contribution_id,
                                          mojom::ContributionStep step,
                                          ResultCallback callback) {
  contribution_info_.UpdateStep(contribution_id, step, std::move(callback));
}

void Database::UpdateContributionInfoStepAndCount(
    const std::string& contribution_id,
    mojom::ContributionStep step,
    int32_t retry_count,
    ResultCallback callback) {
  contribution_info_.UpdateStepAndCount(contribution_id, step, retry_count,
                                        std::move(callback));
}

void Database::UpdateContributionInfoContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ResultCallback callback) {
  contribution_info_.UpdateContributedAmount(contribution_id, publisher_key,
                                             std::move(callback));
}

void Database::FinishAllInProgressContributions(ResultCallback callback) {
  contribution_info_.FinishAllInProgressRecords(std::move(callback));
}

/**
 * CONTRIBUTION QUEUE
 */
void Database::SaveContributionQueue(mojom::ContributionQueuePtr info,
                                     ResultCallback callback) {
  return contribution_queue_.InsertOrUpdate(std::move(info),
                                            std::move(callback));
}

void Database::GetFirstContributionQueue(
    GetFirstContributionQueueCallback callback) {
  return contribution_queue_.GetFirstRecord(std::move(callback));
}

void Database::MarkContributionQueueAsComplete(const std::string& id,
                                               ResultCallback callback) {
  return contribution_queue_.MarkRecordAsComplete(id, std::move(callback));
}

/**
 * CREDS BATCH
 */
void Database::SaveCredsBatch(mojom::CredsBatchPtr info,
                              ResultCallback callback) {
  creds_batch_.InsertOrUpdate(std::move(info), std::move(callback));
}

void Database::GetCredsBatchByTrigger(const std::string& trigger_id,
                                      const mojom::CredsBatchType trigger_type,
                                      GetCredsBatchCallback callback) {
  creds_batch_.GetRecordByTrigger(trigger_id, trigger_type,
                                  std::move(callback));
}

void Database::SaveSignedCreds(mojom::CredsBatchPtr info,
                               ResultCallback callback) {
  creds_batch_.SaveSignedCreds(std::move(info), std::move(callback));
}

void Database::GetAllCredsBatches(GetCredsBatchListCallback callback) {
  creds_batch_.GetAllRecords(std::move(callback));
}

void Database::UpdateCredsBatchStatus(const std::string& trigger_id,
                                      mojom::CredsBatchType trigger_type,
                                      mojom::CredsBatchStatus status,
                                      ResultCallback callback) {
  creds_batch_.UpdateStatus(trigger_id, trigger_type, status,
                            std::move(callback));
}

void Database::UpdateCredsBatchesStatus(
    const std::vector<std::string>& trigger_ids,
    mojom::CredsBatchType trigger_type,
    mojom::CredsBatchStatus status,
    ResultCallback callback) {
  creds_batch_.UpdateRecordsStatus(trigger_ids, trigger_type, status,
                                   std::move(callback));
}

void Database::GetCredsBatchesByTriggers(
    const std::vector<std::string>& trigger_ids,
    GetCredsBatchListCallback callback) {
  creds_batch_.GetRecordsByTriggers(trigger_ids, std::move(callback));
}

/**
 * EVENT LOG
 */
void Database::SaveEventLog(const std::string& key, const std::string& value) {
  event_log_.Insert(key, value);
}

void Database::SaveEventLogs(const std::map<std::string, std::string>& records,
                             ResultCallback callback) {
  event_log_.InsertRecords(records, std::move(callback));
}

void Database::GetLastEventLogs(GetEventLogsCallback callback) {
  event_log_.GetLastRecords(std::move(callback));
}

/**
 * EXTERNAL TRANSACTIONS
 */
void Database::SaveExternalTransaction(
    mojom::ExternalTransactionPtr transaction,
    ResultCallback callback) {
  external_transactions_.Insert(std::move(transaction), std::move(callback));
}

void Database::GetExternalTransaction(const std::string& contribution_id,
                                      const std::string& destination,
                                      GetExternalTransactionCallback callback) {
  external_transactions_.GetTransaction(contribution_id, destination,
                                        std::move(callback));
}

/**
 * MEDIA PUBLISHER INFO
 */
void Database::SaveMediaPublisherInfo(const std::string& media_key,
                                      const std::string& publisher_key,
                                      ResultCallback callback) {
  media_publisher_info_.InsertOrUpdate(media_key, publisher_key,
                                       std::move(callback));
}

void Database::GetMediaPublisherInfo(const std::string& media_key,
                                     PublisherInfoCallback callback) {
  media_publisher_info_.GetRecord(media_key, std::move(callback));
}

/**
 * PUBLISHER INFO
 */
void Database::SavePublisherInfo(mojom::PublisherInfoPtr publisher_info,
                                 ResultCallback callback) {
  publisher_info_.InsertOrUpdate(std::move(publisher_info),
                                 std::move(callback));
}

void Database::GetPublisherInfo(const std::string& publisher_key,
                                GetPublisherInfoCallback callback) {
  publisher_info_.GetRecord(publisher_key, std::move(callback));
}

void Database::GetPanelPublisherInfo(mojom::ActivityInfoFilterPtr filter,
                                     GetPublisherPanelInfoCallback callback) {
  publisher_info_.GetPanelRecord(std::move(filter), std::move(callback));
}

void Database::RestorePublishers(ResultCallback callback) {
  publisher_info_.RestorePublishers(std::move(callback));
}

void Database::GetExcludedList(GetExcludedListCallback callback) {
  publisher_info_.GetExcludedList(std::move(callback));
}

/**
 * RECURRING TIPS
 */
void Database::SaveRecurringTip(mojom::RecurringTipPtr info,
                                ResultCallback callback) {
  if (info) {
    SaveEventLog(log::kRecurringTipAdded, info->publisher_key);
  }
  recurring_tip_.InsertOrUpdate(std::move(info), std::move(callback));
}

void Database::SetMonthlyContribution(const std::string& publisher_id,
                                      double amount,
                                      base::OnceCallback<void(bool)> callback) {
  SaveEventLog(log::kRecurringTipAdded, publisher_id);
  recurring_tip_.InsertOrUpdate(publisher_id, amount, std::move(callback));
}

void Database::AdvanceMonthlyContributionDates(
    const std::vector<std::string>& publisher_ids,
    base::OnceCallback<void(bool)> callback) {
  recurring_tip_.AdvanceMonthlyContributionDates(publisher_ids,
                                                 std::move(callback));
}

void Database::GetNextMonthlyContributionTime(
    base::OnceCallback<void(std::optional<base::Time>)> callback) {
  recurring_tip_.GetNextMonthlyContributionTime(std::move(callback));
}

void Database::GetRecurringTips(GetRecurringTipsCallback callback) {
  recurring_tip_.GetAllRecords(std::move(callback));
}

void Database::RemoveRecurringTip(const std::string& publisher_key,
                                  ResultCallback callback) {
  SaveEventLog(log::kRecurringTipRemoved, publisher_key);
  recurring_tip_.DeleteRecord(publisher_key, std::move(callback));
}

/**
 * SERVER PUBLISHER INFO
 */
void Database::SearchPublisherPrefixList(
    const std::string& publisher_prefix,
    SearchPublisherPrefixListCallback callback) {
  publisher_prefix_list_.Search(publisher_prefix, std::move(callback));
}

void Database::ResetPublisherPrefixList(publisher::PrefixListReader reader,
                                        ResultCallback callback) {
  publisher_prefix_list_.Reset(std::move(reader), std::move(callback));
}

void Database::InsertServerPublisherInfo(
    const mojom::ServerPublisherInfo& server_info,
    ResultCallback callback) {
  server_publisher_info_.InsertOrUpdate(server_info, std::move(callback));
}

void Database::GetServerPublisherInfo(const std::string& publisher_key,
                                      GetServerPublisherInfoCallback callback) {
  server_publisher_info_.GetRecord(publisher_key, std::move(callback));
}

void Database::DeleteExpiredServerPublisherInfo(int64_t max_age_seconds,
                                                ResultCallback callback) {
  server_publisher_info_.DeleteExpiredRecords(max_age_seconds,
                                              std::move(callback));
}

/**
 * SKU ORDER
 */
void Database::SaveSKUOrder(mojom::SKUOrderPtr order, ResultCallback callback) {
  sku_order_.InsertOrUpdate(std::move(order), std::move(callback));
}

void Database::UpdateSKUOrderStatus(const std::string& order_id,
                                    mojom::SKUOrderStatus status,
                                    ResultCallback callback) {
  sku_order_.UpdateStatus(order_id, status, std::move(callback));
}

void Database::GetSKUOrder(const std::string& order_id,
                           GetSKUOrderCallback callback) {
  sku_order_.GetRecord(order_id, std::move(callback));
}

void Database::GetSKUOrderByContributionId(const std::string& contribution_id,
                                           GetSKUOrderCallback callback) {
  sku_order_.GetRecordByContributionId(contribution_id, std::move(callback));
}

void Database::SaveContributionIdForSKUOrder(const std::string& order_id,
                                             const std::string& contribution_id,
                                             ResultCallback callback) {
  sku_order_.SaveContributionIdForSKUOrder(order_id, contribution_id,
                                           std::move(callback));
}

/**
 * SKU TRANSACTION
 */
void Database::SaveSKUTransaction(mojom::SKUTransactionPtr transaction,
                                  ResultCallback callback) {
  sku_transaction_.InsertOrUpdate(std::move(transaction), std::move(callback));
}

void Database::SaveSKUExternalTransaction(
    const std::string& transaction_id,
    const std::string& external_transaction_id,
    ResultCallback callback) {
  sku_transaction_.SaveExternalTransaction(
      transaction_id, external_transaction_id, std::move(callback));
}

void Database::GetSKUTransactionByOrderId(const std::string& order_id,
                                          GetSKUTransactionCallback callback) {
  sku_transaction_.GetRecordByOrderId(order_id, std::move(callback));
}

/**
 * UNBLINDED TOKEN
 */
void Database::SaveUnblindedTokenList(
    std::vector<mojom::UnblindedTokenPtr> list,
    ResultCallback callback) {
  unblinded_token_.InsertOrUpdateList(std::move(list), std::move(callback));
}

void Database::MarkUnblindedTokensAsSpent(const std::vector<std::string>& ids,
                                          mojom::RewardsType redeem_type,
                                          const std::string& redeem_id,
                                          ResultCallback callback) {
  unblinded_token_.MarkRecordListAsSpent(ids, redeem_type, redeem_id,
                                         std::move(callback));
}

void Database::MarkUnblindedTokensAsReserved(
    const std::vector<std::string>& ids,
    const std::string& redeem_id,
    ResultCallback callback) {
  unblinded_token_.MarkRecordListAsReserved(ids, redeem_id,
                                            std::move(callback));
}

void Database::MarkUnblindedTokensAsSpendable(const std::string& redeem_id,
                                              ResultCallback callback) {
  unblinded_token_.MarkRecordListAsSpendable(redeem_id, std::move(callback));
}

void Database::GetSpendableUnblindedTokens(
    GetUnblindedTokenListCallback callback) {
  unblinded_token_.GetSpendableRecords(std::move(callback));
}

void Database::GetReservedUnblindedTokens(
    const std::string& redeem_id,
    GetUnblindedTokenListCallback callback) {
  unblinded_token_.GetReservedRecordList(redeem_id, std::move(callback));
}

void Database::GetSpendableUnblindedTokensByBatchTypes(
    const std::vector<mojom::CredsBatchType>& batch_types,
    GetUnblindedTokenListCallback callback) {
  unblinded_token_.GetSpendableRecordListByBatchTypes(batch_types,
                                                      std::move(callback));
}

}  // namespace brave_rewards::internal::database
