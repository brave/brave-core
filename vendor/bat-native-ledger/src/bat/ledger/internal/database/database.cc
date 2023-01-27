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
  external_transactions_ =
      std::make_unique<DatabaseExternalTransactions>(ledger_);
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

void Database::Initialize(bool execute_create_script,
                          ledger::LegacyResultCallback callback) {
  initialize_->Start(execute_create_script, callback);
}

void Database::Close(ledger::LegacyResultCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::CLOSE;

  transaction->commands.push_back(std::move(command));
  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

/**
 * ACTIVITY INFO
 */
void Database::SaveActivityInfo(mojom::PublisherInfoPtr info,
                                ledger::LegacyResultCallback callback) {
  activity_info_->InsertOrUpdate(std::move(info), callback);
}

void Database::NormalizeActivityInfoList(
    std::vector<mojom::PublisherInfoPtr> list,
    ledger::LegacyResultCallback callback) {
  activity_info_->NormalizeList(std::move(list), callback);
}

void Database::GetActivityInfoList(uint32_t start,
                                   uint32_t limit,
                                   mojom::ActivityInfoFilterPtr filter,
                                   ledger::PublisherInfoListCallback callback) {
  activity_info_->GetRecordsList(start, limit, std::move(filter), callback);
}

void Database::DeleteActivityInfo(const std::string& publisher_key,
                                  ledger::LegacyResultCallback callback) {
  activity_info_->DeleteRecord(publisher_key, callback);
}

void Database::GetPublishersVisitedCount(
    base::OnceCallback<void(int)> callback) {
  activity_info_->GetPublishersVisitedCount(std::move(callback));
}

/**
 * BALANCE REPORT INFO
 */
void Database::SaveBalanceReportInfo(mojom::BalanceReportInfoPtr info,
                                     ledger::LegacyResultCallback callback) {
  balance_report_->InsertOrUpdate(std::move(info), callback);
}

void Database::SaveBalanceReportInfoList(
    std::vector<mojom::BalanceReportInfoPtr> list,
    ledger::LegacyResultCallback callback) {
  balance_report_->InsertOrUpdateList(std::move(list), callback);
}

void Database::SaveBalanceReportInfoItem(
    mojom::ActivityMonth month,
    int year,
    mojom::ReportType type,
    double amount,
    ledger::LegacyResultCallback callback) {
  balance_report_->SetAmount(month, year, type, amount, callback);
}

void Database::GetBalanceReportInfo(mojom::ActivityMonth month,
                                    int year,
                                    ledger::GetBalanceReportCallback callback) {
  balance_report_->GetRecord(month, year, callback);
}

void Database::GetAllBalanceReports(
    ledger::GetBalanceReportListCallback callback) {
  balance_report_->GetAllRecords(callback);
}

void Database::DeleteAllBalanceReports(ledger::LegacyResultCallback callback) {
  balance_report_->DeleteAllRecords(callback);
}

/**
 * CONTRIBUTION INFO
 */
void Database::SaveContributionInfo(mojom::ContributionInfoPtr info,
                                    ledger::LegacyResultCallback callback) {
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

void Database::GetOneTimeTips(const mojom::ActivityMonth month,
                              const int year,
                              ledger::PublisherInfoListCallback callback) {
  contribution_info_->GetOneTimeTips(month, year, callback);
}

void Database::GetContributionReport(
    const mojom::ActivityMonth month,
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
    mojom::ContributionStep step,
    ledger::LegacyResultCallback callback) {
  contribution_info_->UpdateStep(
      contribution_id,
      step,
      callback);
}

void Database::UpdateContributionInfoStepAndCount(
    const std::string& contribution_id,
    mojom::ContributionStep step,
    int32_t retry_count,
    ledger::LegacyResultCallback callback) {
  contribution_info_->UpdateStepAndCount(
      contribution_id,
      step,
      retry_count,
      callback);
}

void Database::UpdateContributionInfoContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ledger::LegacyResultCallback callback) {
  contribution_info_->UpdateContributedAmount(
      contribution_id,
      publisher_key,
      callback);
}

void Database::FinishAllInProgressContributions(
    ledger::LegacyResultCallback callback) {
  contribution_info_->FinishAllInProgressRecords(callback);
}

/**
 * CONTRIBUTION QUEUE
 */
void Database::SaveContributionQueue(mojom::ContributionQueuePtr info,
                                     ledger::LegacyResultCallback callback) {
  return contribution_queue_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetFirstContributionQueue(
    GetFirstContributionQueueCallback callback) {
  return contribution_queue_->GetFirstRecord(callback);
}

void Database::MarkContributionQueueAsComplete(
    const std::string& id,
    ledger::LegacyResultCallback callback) {
  return contribution_queue_->MarkRecordAsComplete(id, callback);
}

/**
 * CREDS BATCH
 */
void Database::SaveCredsBatch(mojom::CredsBatchPtr info,
                              ledger::LegacyResultCallback callback) {
  creds_batch_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetCredsBatchByTrigger(const std::string& trigger_id,
                                      const mojom::CredsBatchType trigger_type,
                                      GetCredsBatchCallback callback) {
  creds_batch_->GetRecordByTrigger(trigger_id, trigger_type, callback);
}

void Database::SaveSignedCreds(mojom::CredsBatchPtr info,
                               ledger::LegacyResultCallback callback) {
  creds_batch_->SaveSignedCreds(std::move(info), callback);
}

void Database::GetAllCredsBatches(GetCredsBatchListCallback callback) {
  creds_batch_->GetAllRecords(callback);
}

void Database::UpdateCredsBatchStatus(const std::string& trigger_id,
                                      mojom::CredsBatchType trigger_type,
                                      mojom::CredsBatchStatus status,
                                      ledger::LegacyResultCallback callback) {
  creds_batch_->UpdateStatus(trigger_id, trigger_type, status, callback);
}

void Database::UpdateCredsBatchesStatus(
    const std::vector<std::string>& trigger_ids,
    mojom::CredsBatchType trigger_type,
    mojom::CredsBatchStatus status,
    ledger::LegacyResultCallback callback) {
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

void Database::SaveEventLogs(const std::map<std::string, std::string>& records,
                             ledger::LegacyResultCallback callback) {
  event_log_->InsertRecords(records, callback);
}

void Database::GetLastEventLogs(ledger::GetEventLogsCallback callback) {
  event_log_->GetLastRecords(callback);
}

/**
 * EXTERNAL TRANSACTIONS
 */
void Database::SaveExternalTransaction(
    mojom::ExternalTransactionPtr transaction,
    ledger::ResultCallback callback) {
  external_transactions_->Insert(std::move(transaction), std::move(callback));
}

void Database::GetExternalTransaction(const std::string& contribution_id,
                                      const std::string& destination,
                                      GetExternalTransactionCallback callback) {
  external_transactions_->GetTransaction(contribution_id, destination,
                                         std::move(callback));
}

/**
 * MEDIA PUBLISHER INFO
 */
void Database::SaveMediaPublisherInfo(const std::string& media_key,
                                      const std::string& publisher_key,
                                      ledger::LegacyResultCallback callback) {
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
    const mojom::ActivityMonth month,
    const int year,
    ledger::GetTransactionReportCallback callback) {
  multi_tables_->GetTransactionReport(month, year, callback);
}

/**
 * PENDING CONTRIBUTION
 */
void Database::SavePendingContribution(
    std::vector<mojom::PendingContributionPtr> list,
    ledger::LegacyResultCallback callback) {
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

void Database::GetUnverifiedPublishersForPendingContributions(
    ledger::UnverifiedPublishersCallback callback) {
  pending_contribution_->GetUnverifiedPublishers(std::move(callback));
}

void Database::RemovePendingContribution(
    uint64_t id,
    ledger::LegacyResultCallback callback) {
  pending_contribution_->DeleteRecord(id, callback);
}

void Database::RemoveAllPendingContributions(
    ledger::LegacyResultCallback callback) {
  pending_contribution_->DeleteAllRecords(callback);
}

/**
 * PROCESSED PUBLISHER
 */
void Database::SaveProcessedPublisherList(
    const std::vector<std::string>& list,
    ledger::LegacyResultCallback callback) {
  processed_publisher_->InsertOrUpdateList(list, callback);
}

void Database::WasPublisherProcessed(const std::string& publisher_key,
                                     ledger::LegacyResultCallback callback) {
  processed_publisher_->WasProcessed(publisher_key, callback);
}

/**
 * PROMOTION
 */
void Database::SavePromotion(mojom::PromotionPtr info,
                             ledger::LegacyResultCallback callback) {
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

void Database::SavePromotionClaimId(const std::string& promotion_id,
                                    const std::string& claim_id,
                                    ledger::LegacyResultCallback callback) {
  promotion_->SaveClaimId(promotion_id, claim_id, callback);
}

void Database::UpdatePromotionStatus(const std::string& promotion_id,
                                     mojom::PromotionStatus status,
                                     ledger::LegacyResultCallback callback) {
  promotion_->UpdateStatus(promotion_id, status, callback);
}

void Database::UpdatePromotionsStatus(
    const std::vector<std::string>& promotion_ids,
    mojom::PromotionStatus status,
    ledger::LegacyResultCallback callback) {
  promotion_->UpdateRecordsStatus(promotion_ids, status, callback);
}

void Database::PromotionCredentialCompleted(
    const std::string& promotion_id,
    ledger::LegacyResultCallback callback) {
  promotion_->CredentialCompleted(promotion_id, callback);
}

void Database::GetPromotionList(
    const std::vector<std::string>& ids,
    client::GetPromotionListCallback callback) {
  promotion_->GetRecords(ids, callback);
}

void Database::UpdatePromotionsBlankPublicKey(
    const std::vector<std::string>& ids,
    ledger::LegacyResultCallback callback) {
  promotion_->UpdateRecordsBlankPublicKey(ids, callback);
}

/**
 * PUBLISHER INFO
 */
void Database::SavePublisherInfo(mojom::PublisherInfoPtr publisher_info,
                                 ledger::LegacyResultCallback callback) {
  publisher_info_->InsertOrUpdate(std::move(publisher_info), callback);
}

void Database::GetPublisherInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  publisher_info_->GetRecord(publisher_key, callback);
}

void Database::GetPanelPublisherInfo(mojom::ActivityInfoFilterPtr filter,
                                     ledger::PublisherInfoCallback callback) {
  publisher_info_->GetPanelRecord(std::move(filter), callback);
}

void Database::RestorePublishers(ledger::ResultCallback callback) {
  publisher_info_->RestorePublishers(std::move(callback));
}

void Database::GetExcludedList(ledger::PublisherInfoListCallback callback) {
  publisher_info_->GetExcludedList(callback);
}

/**
 * RECURRING TIPS
 */
void Database::SaveRecurringTip(mojom::RecurringTipPtr info,
                                ledger::LegacyResultCallback callback) {
  if (info) {
    SaveEventLog(log::kRecurringTipAdded, info->publisher_key);
  }
  recurring_tip_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetRecurringTips(ledger::PublisherInfoListCallback callback) {
  recurring_tip_->GetAllRecords(callback);
}

void Database::RemoveRecurringTip(const std::string& publisher_key,
                                  ledger::LegacyResultCallback callback) {
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
    ledger::LegacyResultCallback callback) {
  publisher_prefix_list_->Reset(std::move(reader), callback);
}

void Database::InsertServerPublisherInfo(
    const mojom::ServerPublisherInfo& server_info,
    ledger::LegacyResultCallback callback) {
  server_publisher_info_->InsertOrUpdate(server_info, callback);
}

void Database::GetServerPublisherInfo(
    const std::string& publisher_key,
    client::GetServerPublisherInfoCallback callback) {
  server_publisher_info_->GetRecord(publisher_key, callback);
}

void Database::DeleteExpiredServerPublisherInfo(
    int64_t max_age_seconds,
    ledger::LegacyResultCallback callback) {
  server_publisher_info_->DeleteExpiredRecords(max_age_seconds, callback);
}

/**
 * SKU ORDER
 */
void Database::SaveSKUOrder(mojom::SKUOrderPtr order,
                            ledger::LegacyResultCallback callback) {
  sku_order_->InsertOrUpdate(std::move(order), callback);
}

void Database::UpdateSKUOrderStatus(const std::string& order_id,
                                    mojom::SKUOrderStatus status,
                                    ledger::LegacyResultCallback callback) {
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
    ledger::LegacyResultCallback callback) {
  sku_order_->SaveContributionIdForSKUOrder(
      order_id,
      contribution_id,
      callback);
}

/**
 * SKU TRANSACTION
 */
void Database::SaveSKUTransaction(mojom::SKUTransactionPtr transaction,
                                  ledger::LegacyResultCallback callback) {
  sku_transaction_->InsertOrUpdate(std::move(transaction), callback);
}

void Database::SaveSKUExternalTransaction(
    const std::string& transaction_id,
    const std::string& external_transaction_id,
    ledger::LegacyResultCallback callback) {
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
    std::vector<mojom::UnblindedTokenPtr> list,
    ledger::LegacyResultCallback callback) {
  unblinded_token_->InsertOrUpdateList(std::move(list), callback);
}

void Database::MarkUnblindedTokensAsSpent(
    const std::vector<std::string>& ids,
    mojom::RewardsType redeem_type,
    const std::string& redeem_id,
    ledger::LegacyResultCallback callback) {
  unblinded_token_->MarkRecordListAsSpent(
      ids,
      redeem_type,
      redeem_id,
      callback);
}

void Database::MarkUnblindedTokensAsReserved(
    const std::vector<std::string>& ids,
    const std::string& redeem_id,
    ledger::LegacyResultCallback callback) {
  unblinded_token_->MarkRecordListAsReserved(
      ids,
      redeem_id,
      callback);
}

void Database::MarkUnblindedTokensAsSpendable(
    const std::string& redeem_id,
    ledger::LegacyResultCallback callback) {
  unblinded_token_->MarkRecordListAsSpendable(
      redeem_id,
      callback);
}

void Database::GetSpendableUnblindedTokens(
    GetUnblindedTokenListCallback callback) {
  unblinded_token_->GetSpendableRecords(std::move(callback));
}

void Database::GetReservedUnblindedTokens(
    const std::string& redeem_id,
    GetUnblindedTokenListCallback callback) {
  unblinded_token_->GetReservedRecordList(redeem_id, callback);
}

void Database::GetSpendableUnblindedTokensByBatchTypes(
    const std::vector<mojom::CredsBatchType>& batch_types,
    GetUnblindedTokenListCallback callback) {
  unblinded_token_->GetSpendableRecordListByBatchTypes(batch_types, callback);
}

}  // namespace database
}  // namespace ledger
