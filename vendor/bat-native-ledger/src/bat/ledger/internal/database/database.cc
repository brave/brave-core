/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

using std::placeholders::_1;

namespace ledger {
namespace database {

Database::Database(LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);

  initialize_ = std::make_unique<DatabaseInitialize>(ledger_);
  activity_info_ = std::make_unique<DatabaseActivityInfo>(ledger_);
  balance_report_ = std::make_unique<DatabaseBalanceReport>(ledger_);
  contribution_queue_ = std::make_unique<DatabaseContributionQueue>(ledger_);
  contribution_info_ = std::make_unique<DatabaseContributionInfo>(ledger_);
  creds_batch_ = std::make_unique<DatabaseCredsBatch>(ledger_);
  event_log_ = std::make_unique<DatabaseEventLog>(ledger_);
  media_publisher_info_ =
      std::make_unique<DatabaseMediaPublisherInfo>(ledger_);
  multi_tables_ = std::make_unique<DatabaseMultiTables>(ledger_);
  pending_contribution_ =
      std::make_unique<DatabasePendingContribution>(ledger_);
  processed_publisher_ = std::make_unique<DatabaseProcessedPublisher>(ledger_);
  promotion_ = std::make_unique<DatabasePromotion>(ledger_);
  publisher_info_ = std::make_unique<DatabasePublisherInfo>(ledger_);
  publisher_prefix_list_ =
      std::make_unique<DatabasePublisherPrefixList>(ledger_);
  recurring_tip_ = std::make_unique<DatabaseRecurringTip>(ledger_);
  server_publisher_info_ =
      std::make_unique<DatabaseServerPublisherInfo>(ledger_);
  sku_transaction_ = std::make_unique<DatabaseSKUTransaction>(ledger_);
  sku_order_ = std::make_unique<DatabaseSKUOrder>(ledger_);
  unblinded_token_ =
      std::make_unique<DatabaseUnblindedToken>(ledger_);
}

Database::~Database() = default;

void Database::Initialize(
    const bool execute_create_script,
    ledger::ResultCallback callback) {
  initialize_->Start(execute_create_script, callback);
}

void Database::Close(ledger::ResultCallback callback) {
  auto transaction = type::DBTransaction::New();
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::CLOSE;

  transaction->commands.push_back(std::move(command));
  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

/**
 * ACTIVITY INFO
 */
void Database::SaveActivityInfo(
    type::PublisherInfoPtr info,
    ledger::ResultCallback callback) {
  activity_info_->InsertOrUpdate(std::move(info), callback);
}

void Database::NormalizeActivityInfoList(
    type::PublisherInfoList list,
    ledger::ResultCallback callback) {
  activity_info_->NormalizeList(std::move(list), callback);
}

void Database::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    type::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  activity_info_->GetRecordsList(start, limit, std::move(filter), callback);
}

void Database::UpdateActivityInfoDuration(
    const std::string& publisher_key,
    uint64_t duration,
    ledger::ResultCallback callback) {
  activity_info_->UpdateDuration(publisher_key, duration, callback);
}

void Database::DeleteActivityInfo(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  activity_info_->DeleteRecord(publisher_key, callback);
}

/**
 * BALANCE REPORT INFO
 */
void Database::SaveBalanceReportInfo(
    type::BalanceReportInfoPtr info,
    ledger::ResultCallback callback) {
  balance_report_->InsertOrUpdate(std::move(info), callback);
}

void Database::SaveBalanceReportInfoList(
      type::BalanceReportInfoList list,
      ledger::ResultCallback callback) {
  balance_report_->InsertOrUpdateList(std::move(list), callback);
}

void Database::SaveBalanceReportInfoItem(
    type::ActivityMonth month,
    int year,
    type::ReportType type,
    double amount,
    ledger::ResultCallback callback) {
  balance_report_->SetAmount(month, year, type, amount, callback);
}

void Database::GetBalanceReportInfo(
    type::ActivityMonth month,
    int year,
    ledger::GetBalanceReportCallback callback) {
  balance_report_->GetRecord(month, year, callback);
}

void Database::GetAllBalanceReports(
    ledger::GetBalanceReportListCallback callback) {
  balance_report_->GetAllRecords(callback);
}

void Database::DeleteAllBalanceReports(
    ledger::ResultCallback callback) {
  balance_report_->DeleteAllRecords(callback);
}

/**
 * CONTRIBUTION INFO
 */
void Database::SaveContributionInfo(
    type::ContributionInfoPtr info,
    ledger::ResultCallback callback) {
  contribution_info_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetContributionInfo(
    const std::string& contribution_id,
    GetContributionInfoCallback callback) {
  contribution_info_->GetRecord(contribution_id, callback);
}

void Database::GetAllContributions(
    ledger::ContributionInfoListCallback callback) {
  contribution_info_->GetAllRecords(callback);
}

void Database::GetOneTimeTips(
    const type::ActivityMonth month,
    const int year,
    ledger::PublisherInfoListCallback callback) {
  contribution_info_->GetOneTimeTips(month, year, callback);
}

void Database::GetContributionReport(
    const type::ActivityMonth month,
    const int year,
    ledger::GetContributionReportCallback callback) {
  contribution_info_->GetContributionReport(month, year, callback);
}

void Database::GetNotCompletedContributions(
    ledger::ContributionInfoListCallback callback) {
  contribution_info_->GetNotCompletedRecords(callback);
}

void Database::UpdateContributionInfoStep(
    const std::string& contribution_id,
    const type::ContributionStep step,
    ledger::ResultCallback callback) {
  contribution_info_->UpdateStep(
      contribution_id,
      step,
      callback);
}

void Database::UpdateContributionInfoStepAndCount(
    const std::string& contribution_id,
    const type::ContributionStep step,
    const int32_t retry_count,
    ledger::ResultCallback callback) {
  contribution_info_->UpdateStepAndCount(
      contribution_id,
      step,
      retry_count,
      callback);
}

void Database::UpdateContributionInfoContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  contribution_info_->UpdateContributedAmount(
      contribution_id,
      publisher_key,
      callback);
}

void Database::FinishAllInProgressContributions(
    ledger::ResultCallback callback) {
  contribution_info_->FinishAllInProgressRecords(callback);
}

/**
 * CONTRIBUTION QUEUE
 */
void Database::SaveContributionQueue(
    type::ContributionQueuePtr info,
    ledger::ResultCallback callback) {
  return contribution_queue_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetFirstContributionQueue(
    GetFirstContributionQueueCallback callback) {
  return contribution_queue_->GetFirstRecord(callback);
}

void Database::MarkContributionQueueAsComplete(
    const std::string& id,
    ledger::ResultCallback callback) {
  return contribution_queue_->MarkRecordAsComplete(id, callback);
}

/**
 * CREDS BATCH
 */
void Database::SaveCredsBatch(
    type::CredsBatchPtr info,
    ledger::ResultCallback callback) {
  creds_batch_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetCredsBatchByTrigger(
    const std::string& trigger_id,
    const type::CredsBatchType trigger_type,
    GetCredsBatchCallback callback) {
  creds_batch_->GetRecordByTrigger(trigger_id, trigger_type, callback);
}

void Database::SaveSignedCreds(
    type::CredsBatchPtr info,
    ledger::ResultCallback callback) {
  creds_batch_->SaveSignedCreds(std::move(info), callback);
}

void Database::GetAllCredsBatches(GetCredsBatchListCallback callback) {
  creds_batch_->GetAllRecords(callback);
}

void Database::UpdateCredsBatchStatus(
    const std::string& trigger_id,
    const type::CredsBatchType trigger_type,
    const type::CredsBatchStatus status,
    ledger::ResultCallback callback) {
  creds_batch_->UpdateStatus(trigger_id, trigger_type, status, callback);
}

void Database::UpdateCredsBatchesStatus(
    const std::vector<std::string>& trigger_ids,
    const type::CredsBatchType trigger_type,
    const type::CredsBatchStatus status,
    ledger::ResultCallback callback) {
  creds_batch_->UpdateRecordsStatus(
      trigger_ids,
      trigger_type,
      status,
      callback);
}

void Database::GetCredsBatchesByTriggers(
    const std::vector<std::string>& trigger_ids,
    GetCredsBatchListCallback callback) {
  creds_batch_->GetRecordsByTriggers(trigger_ids, callback);
}

/**
 * EVENT LOG
 */
void Database::SaveEventLog(const std::string& key, const std::string& value) {
  event_log_->Insert(key, value);
}

void Database::SaveEventLogs(
    const std::map<std::string, std::string>& records,
    ledger::ResultCallback callback) {
  event_log_->InsertRecords(records, callback);
}

void Database::GetLastEventLogs(ledger::GetEventLogsCallback callback) {
  event_log_->GetLastRecords(callback);
}

/**
 * MEDIA PUBLISHER INFO
 */
void Database::SaveMediaPublisherInfo(
    const std::string& media_key,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  media_publisher_info_->InsertOrUpdate(media_key, publisher_key, callback);
}

void Database::GetMediaPublisherInfo(
    const std::string& media_key,
    ledger::PublisherInfoCallback callback) {
  media_publisher_info_->GetRecord(media_key, callback);
}

/**
 * MULTI TABLES
 * for queries that are not limited to one table
 */
void Database::GetTransactionReport(
    const type::ActivityMonth month,
    const int year,
    ledger::GetTransactionReportCallback callback) {
  multi_tables_->GetTransactionReport(month, year, callback);
}

/**
 * PENDING CONTRIBUTION
 */
void Database::SavePendingContribution(
    type::PendingContributionList list,
    ledger::ResultCallback callback) {
  pending_contribution_->InsertOrUpdateList(std::move(list), callback);
}

void Database::GetPendingContributionsTotal(
    ledger::PendingContributionsTotalCallback callback) {
  pending_contribution_->GetReservedAmount(callback);
}

void Database::GetPendingContributions(
    ledger::PendingContributionInfoListCallback callback) {
  pending_contribution_->GetAllRecords(callback);
}

void Database::RemovePendingContribution(
    const uint64_t id,
    ledger::ResultCallback callback) {
  pending_contribution_->DeleteRecord(id, callback);
}

void Database::RemoveAllPendingContributions(ledger::ResultCallback callback) {
  pending_contribution_->DeleteAllRecords(callback);
}

/**
 * PROCESSED PUBLISHER
 */
void Database::SaveProcessedPublisherList(
    const std::vector<std::string>& list,
    ledger::ResultCallback callback) {
  processed_publisher_->InsertOrUpdateList(list, callback);
}

void Database::WasPublisherProcessed(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  processed_publisher_->WasProcessed(publisher_key, callback);
}

/**
 * PROMOTION
 */
void Database::SavePromotion(
    type::PromotionPtr info,
    ledger::ResultCallback callback) {
  promotion_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetPromotion(
    const std::string& id,
    GetPromotionCallback callback) {
  promotion_->GetRecord(id, callback);
}

void Database::GetAllPromotions(ledger::GetAllPromotionsCallback callback) {
  promotion_->GetAllRecords(callback);
}

void Database::SavePromotionClaimId(
    const std::string& promotion_id,
    const std::string& claim_id,
    ledger::ResultCallback callback) {
  promotion_->SaveClaimId(promotion_id, claim_id, callback);
}

void Database::UpdatePromotionStatus(
    const std::string& promotion_id,
    const type::PromotionStatus status,
    ledger::ResultCallback callback) {
  promotion_->UpdateStatus(promotion_id, status, callback);
}

void Database::UpdatePromotionsStatus(
    const std::vector<std::string>& promotion_ids,
    const type::PromotionStatus status,
    ledger::ResultCallback callback) {
  promotion_->UpdateRecordsStatus(promotion_ids, status, callback);
}

void Database::PromotionCredentialCompleted(
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  promotion_->CredentialCompleted(promotion_id, callback);
}

void Database::GetPromotionList(
    const std::vector<std::string>& ids,
    client::GetPromotionListCallback callback) {
  promotion_->GetRecords(ids, callback);
}

void Database::GetPromotionListByType(
    const std::vector<type::PromotionType>& types,
    client::GetPromotionListCallback callback) {
  promotion_->GetRecordsByType(types, callback);
}

void Database::UpdatePromotionsBlankPublicKey(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  promotion_->UpdateRecordsBlankPublicKey(ids, callback);
}

/**
 * PUBLISHER INFO
 */
void Database::SavePublisherInfo(
    type::PublisherInfoPtr publisher_info,
    ledger::ResultCallback callback) {
  publisher_info_->InsertOrUpdate(std::move(publisher_info), callback);
}

void Database::GetPublisherInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  publisher_info_->GetRecord(publisher_key, callback);
}

void Database::GetPanelPublisherInfo(
    type::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoCallback callback) {
  publisher_info_->GetPanelRecord(std::move(filter), callback);
}

void Database::RestorePublishers(ledger::ResultCallback callback) {
  publisher_info_->RestorePublishers(callback);
}

void Database::GetExcludedList(ledger::PublisherInfoListCallback callback) {
  publisher_info_->GetExcludedList(callback);
}

/**
 * RECURRING TIPS
 */
void Database::SaveRecurringTip(
    type::RecurringTipPtr info,
    ledger::ResultCallback callback) {
  if (info) {
    SaveEventLog(log::kRecurringTipAdded, info->publisher_key);
  }
  recurring_tip_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetRecurringTips(ledger::PublisherInfoListCallback callback) {
  recurring_tip_->GetAllRecords(callback);
}

void Database::RemoveRecurringTip(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  SaveEventLog(log::kRecurringTipRemoved, publisher_key);
  recurring_tip_->DeleteRecord(publisher_key, callback);
}

/**
 * SERVER PUBLISHER INFO
 */
void Database::SearchPublisherPrefixList(
    const std::string& publisher_prefix,
    SearchPublisherPrefixListCallback callback) {
  publisher_prefix_list_->Search(publisher_prefix, callback);
}

void Database::ResetPublisherPrefixList(
    std::unique_ptr<publisher::PrefixListReader> reader,
    ledger::ResultCallback callback) {
  publisher_prefix_list_->Reset(std::move(reader), callback);
}

void Database::InsertServerPublisherInfo(
    const type::ServerPublisherInfo& server_info,
    ledger::ResultCallback callback) {
  server_publisher_info_->InsertOrUpdate(server_info, callback);
}

void Database::GetServerPublisherInfo(
    const std::string& publisher_key,
    client::GetServerPublisherInfoCallback callback) {
  server_publisher_info_->GetRecord(publisher_key, callback);
}

void Database::DeleteExpiredServerPublisherInfo(
    const int64_t max_age_seconds,
    ledger::ResultCallback callback) {
  server_publisher_info_->DeleteExpiredRecords(max_age_seconds, callback);
}

/**
 * SKU ORDER
 */
void Database::SaveSKUOrder(
    type::SKUOrderPtr order,
    ledger::ResultCallback callback) {
  sku_order_->InsertOrUpdate(std::move(order), callback);
}

void Database::UpdateSKUOrderStatus(
    const std::string& order_id,
    const type::SKUOrderStatus status,
    ledger::ResultCallback callback) {
  sku_order_->UpdateStatus(order_id, status, callback);
}

void Database::GetSKUOrder(
    const std::string& order_id,
    GetSKUOrderCallback callback) {
  sku_order_->GetRecord(order_id, callback);
}

void Database::GetSKUOrderByContributionId(
    const std::string& contribution_id,
    GetSKUOrderCallback callback) {
  sku_order_->GetRecordByContributionId(contribution_id, callback);
}

void Database::SaveContributionIdForSKUOrder(
    const std::string& order_id,
    const std::string& contribution_id,
    ledger::ResultCallback callback) {
  sku_order_->SaveContributionIdForSKUOrder(
      order_id,
      contribution_id,
      callback);
}

/**
 * SKU TRANSACTION
 */
void Database::SaveSKUTransaction(
    type::SKUTransactionPtr transaction,
    ledger::ResultCallback callback) {
  sku_transaction_->InsertOrUpdate(std::move(transaction), callback);
}

void Database::SaveSKUExternalTransaction(
    const std::string& transaction_id,
    const std::string& external_transaction_id,
    ledger::ResultCallback callback) {
  sku_transaction_->SaveExternalTransaction(
      transaction_id,
      external_transaction_id,
      callback);
}

void Database::GetSKUTransactionByOrderId(
    const std::string& order_id,
    GetSKUTransactionCallback callback) {
  sku_transaction_->GetRecordByOrderId(order_id, callback);
}

/**
 * UNBLINDED TOKEN
 */
void Database::SaveUnblindedTokenList(
    type::UnblindedTokenList list,
    ledger::ResultCallback callback) {
  unblinded_token_->InsertOrUpdateList(std::move(list), callback);
}

void Database::MarkUnblindedTokensAsSpent(
    const std::vector<std::string>& ids,
    type::RewardsType redeem_type,
    const std::string& redeem_id,
    ledger::ResultCallback callback) {
  unblinded_token_->MarkRecordListAsSpent(
      ids,
      redeem_type,
      redeem_id,
      callback);
}

void Database::MarkUnblindedTokensAsReserved(
    const std::vector<std::string>& ids,
    const std::string& redeem_id,
    ledger::ResultCallback callback) {
  unblinded_token_->MarkRecordListAsReserved(
      ids,
      redeem_id,
      callback);
}

void Database::MarkUnblindedTokensAsSpendable(
    const std::string& redeem_id,
    ledger::ResultCallback callback) {
  unblinded_token_->MarkRecordListAsSpendable(
      redeem_id,
      callback);
}

void Database::GetSpendableUnblindedTokensByTriggerIds(
    const std::vector<std::string>& trigger_ids,
    GetUnblindedTokenListCallback callback) {
  unblinded_token_->GetSpendableRecordsByTriggerIds(trigger_ids, callback);
}

void Database::GetReservedUnblindedTokens(
    const std::string& redeem_id,
    GetUnblindedTokenListCallback callback) {
  unblinded_token_->GetReservedRecordList(redeem_id, callback);
}

void Database::GetSpendableUnblindedTokensByBatchTypes(
    const std::vector<type::CredsBatchType>& batch_types,
    GetUnblindedTokenListCallback callback) {
  unblinded_token_->GetSpendableRecordListByBatchTypes(batch_types, callback);
}

}  // namespace database
}  // namespace ledger
