/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/database/database.h"
#include "bat/ledger/internal/database/database_activity_info.h"
#include "bat/ledger/internal/database/database_creds_batch.h"
#include "bat/ledger/internal/database/database_contribution_info.h"
#include "bat/ledger/internal/database/database_contribution_queue.h"
#include "bat/ledger/internal/database/database_initialize.h"
#include "bat/ledger/internal/database/database_media_publisher_info.h"
#include "bat/ledger/internal/database/database_multi_tables.h"
#include "bat/ledger/internal/database/database_pending_contribution.h"
#include "bat/ledger/internal/database/database_promotion.h"
#include "bat/ledger/internal/database/database_publisher_info.h"
#include "bat/ledger/internal/database/database_recurring_tip.h"
#include "bat/ledger/internal/database/database_server_publisher_info.h"
#include "bat/ledger/internal/database/database_sku_order.h"
#include "bat/ledger/internal/database/database_sku_transaction.h"
#include "bat/ledger/internal/database/database_unblinded_token.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_database {

Database::Database(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);

  initialize_ = std::make_unique<DatabaseInitialize>(ledger_);
  activity_info_ = std::make_unique<DatabaseActivityInfo>(ledger_);
  contribution_queue_ = std::make_unique<DatabaseContributionQueue>(ledger_);
  contribution_info_ = std::make_unique<DatabaseContributionInfo>(ledger_);
  creds_batch_ = std::make_unique<DatabaseCredsBatch>(ledger_);
  media_publisher_info_ =
      std::make_unique<DatabaseMediaPublisherInfo>(ledger_);
  multi_tables_ = std::make_unique<DatabaseMultiTables>(ledger_);
  pending_contribution_ =
      std::make_unique<DatabasePendingContribution>(ledger_);
  promotion_ = std::make_unique<DatabasePromotion>(ledger_);
  publisher_info_ = std::make_unique<DatabasePublisherInfo>(ledger_);
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

/**
 * ACTIVITY INFO
 */
void Database::SaveActivityInfo(
    ledger::PublisherInfoPtr info,
    ledger::ResultCallback callback) {
  ledger::PublisherInfoList list;
  list.push_back(std::move(info));
  activity_info_->InsertOrUpdateList(std::move(list), callback);
}

void Database::SaveActivityInfoList(
    ledger::PublisherInfoList list,
    ledger::ResultCallback callback) {
  activity_info_->InsertOrUpdateList(std::move(list), callback);
}

void Database::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilterPtr filter,
    ledger::PublisherInfoListCallback callback) {
  activity_info_->GetRecordsList(start, limit, std::move(filter), callback);
}

void Database::DeleteActivityInfo(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  activity_info_->DeleteRecord(publisher_key, callback);
}

/**
 * CONTRIBUTION INFO
 */
void Database::SaveContributionInfo(
    ledger::ContributionInfoPtr info,
    ledger::ResultCallback callback) {
  contribution_info_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetContributionInfo(
    const std::string& contribution_id,
    ledger::GetContributionInfoCallback callback) {
  contribution_info_->GetRecord(contribution_id, callback);
}

void Database::GetAllContributions(
    ledger::ContributionInfoListCallback callback) {
  contribution_info_->GetAllRecords(callback);
}

void Database::GetOneTimeTips(
    const ledger::ActivityMonth month,
    const int year,
    ledger::PublisherInfoListCallback callback) {
  contribution_info_->GetOneTimeTips(month, year, callback);
}

void Database::GetContributionReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetContributionReportCallback callback) {
  contribution_info_->GetContributionReport(month, year, callback);
}

void Database::GetIncompleteContributions(
    const ledger::ContributionProcessor processor,
    ledger::ContributionInfoListCallback callback) {
  contribution_info_->GetIncompletedRecords(processor, callback);
}

void Database::UpdateContributionInfoStepAndCount(
    const std::string& contribution_id,
    const ledger::ContributionStep step,
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

/**
 * CONTRIBUTION QUEUE
 */
void Database::SaveContributionQueue(
    ledger::ContributionQueuePtr info,
    ledger::ResultCallback callback) {
  return contribution_queue_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetFirstContributionQueue(
    ledger::GetFirstContributionQueueCallback callback) {
  return contribution_queue_->GetFirstRecord(callback);
}

void Database::DeleteContributionQueue(
    const uint64_t id,
    ledger::ResultCallback callback) {
  return contribution_queue_->DeleteRecord(id, callback);
}

/**
 * CREDS BATCH
 */
void Database::SaveCredsBatch(
    ledger::CredsBatchPtr info,
    ledger::ResultCallback callback) {
  creds_batch_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetCredsBatchByTrigger(
    const std::string& trigger_id,
    const ledger::CredsBatchType trigger_type,
    ledger::GetCredsBatchCallback callback) {
  creds_batch_->GetRecordByTrigger(trigger_id, trigger_type, callback);
}

void Database::SaveSignedCreds(
    ledger::CredsBatchPtr info,
    ledger::ResultCallback callback) {
  creds_batch_->SaveSignedCreds(std::move(info), callback);
}

void Database::GetAllCredsBatches(ledger::GetAllCredsBatchCallback callback) {
  creds_batch_->GetAllRecords(callback);
}

void Database::UpdateCredsBatchStatus(
    const std::string& trigger_id,
    const ledger::CredsBatchType trigger_type,
    const ledger::CredsBatchStatus status,
    ledger::ResultCallback callback) {
  creds_batch_->UpdateStatus(trigger_id, trigger_type, status, callback);
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
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetTransactionReportCallback callback) {
  multi_tables_->GetTransactionReport(month, year, callback);
}

/**
 * PENDING CONTRIBUTION
 */
void Database::SavePendingContribution(
    ledger::PendingContributionList list,
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
 * PROMOTION
 */
void Database::SavePromotion(
    ledger::PromotionPtr info,
    ledger::ResultCallback callback) {
  promotion_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetPromotion(
    const std::string& id,
    ledger::GetPromotionCallback callback) {
  promotion_->GetRecord(id, callback);
}

void Database::GetAllPromotions(ledger::GetAllPromotionsCallback callback) {
  promotion_->GetAllRecords(callback);
}

void Database::DeletePromotionList(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  promotion_->DeleteRecordList(ids, callback);
}

void Database::SavePromotionClaimId(
    const std::string& promotion_id,
    const std::string& claim_id,
    ledger::ResultCallback callback) {
  promotion_->SaveClaimId(promotion_id, claim_id, callback);
}

void Database::UpdatePromotionStatus(
    const std::string& promotion_id,
    const ledger::PromotionStatus status,
    ledger::ResultCallback callback) {
  promotion_->UpdateStatus(promotion_id, status, callback);
}

void Database::PromotionCredentialCompleted(
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  promotion_->CredentialCompleted(promotion_id, callback);
}

void Database::GetPromotionList(
    const std::vector<std::string>& ids,
    ledger::GetPromotionListCallback callback) {
  promotion_->GetRecords(ids, callback);
}

void Database::GetPromotionListByType(
    const std::vector<ledger::PromotionType>& types,
    ledger::GetPromotionListCallback callback) {
  promotion_->GetRecordsByType(types, callback);
}

/**
 * PUBLISHER INFO
 */
void Database::SavePublisherInfo(
    ledger::PublisherInfoPtr publisher_info,
    ledger::ResultCallback callback) {
  publisher_info_->InsertOrUpdate(std::move(publisher_info), callback);
}

void Database::GetPublisherInfo(
    const std::string& publisher_key,
    ledger::PublisherInfoCallback callback) {
  publisher_info_->GetRecord(publisher_key, callback);
}

void Database::GetPanelPublisherInfo(
    ledger::ActivityInfoFilterPtr filter,
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
    ledger::RecurringTipPtr info,
    ledger::ResultCallback callback) {
  recurring_tip_->InsertOrUpdate(std::move(info), callback);
}

void Database::GetRecurringTips(ledger::PublisherInfoListCallback callback) {
  recurring_tip_->GetAllRecords(callback);
}

void Database::RemoveRecurringTip(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  recurring_tip_->DeleteRecord(publisher_key, callback);
}

/**
 * SERVER PUBLISHER INFO
 */
void Database::ClearServerPublisherList(ledger::ResultCallback callback) {
  server_publisher_info_->DeleteAll(callback);
}

void Database::InsertServerPublisherList(
    const std::vector<ledger::ServerPublisherPartial>& list,
    ledger::ResultCallback callback) {
  server_publisher_info_->InsertOrUpdatePartialList(list, callback);
}

void Database::InsertPublisherBannerList(
    const std::vector<ledger::PublisherBanner>& list,
    ledger::ResultCallback callback) {
  server_publisher_info_->InsertOrUpdateBannerList(list, callback);
}

void Database::GetServerPublisherInfo(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) {
  server_publisher_info_->GetRecord(publisher_key, callback);
}

/**
 * SKU ORDER
 */
void Database::SaveSKUOrder(
    ledger::SKUOrderPtr order,
    ledger::ResultCallback callback) {
  sku_order_->InsertOrUpdate(std::move(order), callback);
}

void Database::UpdateSKUOrderStatus(
    const std::string& order_id,
    const ledger::SKUOrderStatus status,
    ledger::ResultCallback callback) {
  sku_order_->UpdateStatus(order_id, status, callback);
}

void Database::GetSKUOrder(
    const std::string& order_id,
    ledger::GetSKUOrderCallback callback) {
  sku_order_->GetRecord(order_id, callback);
}

/**
 * SKU TRANSACTION
 */
void Database::SaveSKUTransaction(
    ledger::SKUTransactionPtr transaction,
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

/**
 * UNBLINDED TOKEN
 */
void Database::SaveUnblindedTokenList(
    ledger::UnblindedTokenList list,
    ledger::ResultCallback callback) {
  unblinded_token_->InsertOrUpdateList(std::move(list), callback);
}

void Database::GetAllUnblindedTokens(
    ledger::GetUnblindedTokenListCallback callback) {
  unblinded_token_->GetAllRecords(callback);
}

void Database::DeleteUnblindedTokens(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  unblinded_token_->DeleteRecordList(ids, callback);
}

void Database::GetUnblindedTokensByTriggerIds(
    const std::vector<std::string>& trigger_ids,
    ledger::GetUnblindedTokenListCallback callback) {
  unblinded_token_->GetRecordsByTriggerIds(trigger_ids, callback);
}

void Database::CheckUnblindedTokensExpiration(
    ledger::ResultCallback callback) {
  unblinded_token_->CheckRecordsExpiration(callback);
}

}  // namespace braveledger_database
