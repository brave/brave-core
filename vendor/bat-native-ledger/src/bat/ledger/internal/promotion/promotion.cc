/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/promotion/promotion.h"

#include <map>
#include <memory>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/constants.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/wallet_info_properties.h"
#include "bat/ledger/internal/promotion/promotion_transfer.h"
#include "bat/ledger/internal/promotion/promotion_util.h"
#include "bat/ledger/option_keys.h"

#include "wrapper.hpp"  // NOLINT

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::UnblindedToken;

namespace ledger {
namespace promotion {

namespace {

const int kFetchPromotionsThresholdInSeconds =
    10 * base::Time::kSecondsPerMinute;

void HandleExpiredPromotions(
    LedgerImpl* ledger_impl,
    type::PromotionMap* promotions) {
  DCHECK(promotions);
  if (!promotions) {
    return;
  }

  const uint64_t current_time = util::GetCurrentTimeStamp();

  for (auto& item : *promotions) {
    if (!item.second || item.second->status == type::PromotionStatus::OVER) {
      continue;
    }

    // we shouldn't expire ad grant
    if (item.second->type == type::PromotionType::ADS) {
      continue;
    }

    if (item.second->expires_at > 0 &&
        item.second->expires_at <= current_time)  {
      ledger_impl->database()->UpdatePromotionStatus(
          item.second->id,
          type::PromotionStatus::OVER,
          [](const type::Result _){});
    }
  }
}

}  // namespace

Promotion::Promotion(LedgerImpl* ledger)
    : attestation_(
          std::make_unique<ledger::attestation::AttestationImpl>(ledger)),
      transfer_(std::make_unique<PromotionTransfer>(ledger)),
      promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)),
      ledger_(ledger) {
  DCHECK(ledger_);
  credentials_ = credential::CredentialsFactory::Create(
      ledger_,
      type::CredsBatchType::PROMOTION);
  DCHECK(credentials_);
}

Promotion::~Promotion() = default;

void Promotion::Initialize() {
  if (!ledger_->state()->GetPromotionCorruptedMigrated()) {
    BLOG(1, "Migrating corrupted promotions");
    auto check_callback = std::bind(&Promotion::CheckForCorrupted,
        this,
        _1);

    ledger_->database()->GetAllPromotions(check_callback);
  }

  auto retry_callback = std::bind(&Promotion::Retry,
      this,
      _1);

  ledger_->database()->GetAllPromotions(retry_callback);
}

void Promotion::Fetch(ledger::FetchPromotionCallback callback) {
  // If we fetched promotions recently, fulfill this request from the
  // database instead of querying the server again
  if (!ledger::is_testing && _environment != type::Environment::STAGING) {
    const uint64_t last_promo_stamp =
        ledger_->state()->GetPromotionLastFetchStamp();
    const uint64_t now = util::GetCurrentTimeStamp();
    if (now - last_promo_stamp < kFetchPromotionsThresholdInSeconds) {
      auto all_callback = std::bind(
          &Promotion::OnGetAllPromotionsFromDatabase,
          this,
          _1,
          callback);
      ledger_->database()->GetAllPromotions(all_callback);
      return;
    }
  }

  auto url_callback = std::bind(&Promotion::OnFetch,
      this,
      _1,
      _2,
      _3,
      std::move(callback));

  auto client_info = ledger_->ledger_client()->GetClientInfo();
  const std::string client = ParseClientInfoToString(std::move(client_info));
  promotion_server_->get_available()->Request(client, url_callback);
}

void Promotion::OnFetch(
    const type::Result result,
    type::PromotionList list,
    const std::vector<std::string>& corrupted_promotions,
    ledger::FetchPromotionCallback callback) {
  if (result == type::Result::NOT_FOUND) {
    ProcessFetchedPromotions(
        type::Result::NOT_FOUND,
        std::move(list),
        callback);
    return;
  }

  if (result == type::Result::LEDGER_ERROR) {
    ProcessFetchedPromotions(
        type::Result::LEDGER_ERROR,
        std::move(list),
        callback);
    return;
  }

  // even though that some promotions are corrupted
  // we should display non corrupted ones either way
  BLOG_IF(
      1,
      result == type::Result::CORRUPTED_DATA,
      "Promotions are not correct: "
          << base::JoinString(corrupted_promotions, ", "));

  auto shared_list = std::make_shared<type::PromotionList>(std::move(list));

  auto all_callback = std::bind(&Promotion::OnGetAllPromotions,
      this,
      _1,
      shared_list,
      callback);

  ledger_->database()->GetAllPromotions(all_callback);
}

void Promotion::OnGetAllPromotions(
    type::PromotionMap promotions,
    std::shared_ptr<type::PromotionList> list,
    ledger::FetchPromotionCallback callback) {
  HandleExpiredPromotions(ledger_, &promotions);

  if (!list) {
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  type::PromotionList promotions_ui;
  for (const auto& item : *list) {
    auto it = promotions.find(item->id);
    if (it != promotions.end()) {
      const auto status = it->second->status;
      promotions.erase(item->id);
      // Skip any promotions that are in the database and have been processed
      // in some way.
      if (status != type::PromotionStatus::ACTIVE &&
          status != type::PromotionStatus::OVER) {
        continue;
      }
    }

    // if the server return expiration for ads we need to set it to 0
    if (item->type == type::PromotionType::ADS) {
      item->expires_at = 0;
    }

    if (item->legacy_claimed) {
      item->status = type::PromotionStatus::ATTESTED;
      auto legacy_callback = std::bind(&Promotion::LegacyClaimedSaved,
          this,
          _1,
          std::make_shared<type::PromotionPtr>(item->Clone()));
      ledger_->database()->SavePromotion(item->Clone(), legacy_callback);
      continue;
    }

    promotions_ui.push_back(item->Clone());

    ledger_->database()->SavePromotion(
        item->Clone(),
        [](const type::Result _){});
  }

  // mark as over promotions that are in db with status active,
  // but are not available on the server anymore
  for (const auto& promotion : promotions) {
    if (promotion.second->status != type::PromotionStatus::ACTIVE) {
      continue;
    }

    bool found =
        std::any_of(list->begin(), list->end(), [&promotion](auto& item) {
          return item->id == promotion.second->id;
        });

    if (!found) {
      ledger_->database()->UpdatePromotionStatus(
          promotion.second->id,
          type::PromotionStatus::OVER,
          [](const type::Result){});
    }
  }

  ProcessFetchedPromotions(
      type::Result::LEDGER_OK,
      std::move(promotions_ui),
      callback);
}

void Promotion::OnGetAllPromotionsFromDatabase(
    type::PromotionMap promotions,
    ledger::FetchPromotionCallback callback) {
  HandleExpiredPromotions(ledger_, &promotions);

  type::PromotionList promotions_ui;
  for (const auto& item : promotions) {
    if (item.second->status == type::PromotionStatus::ACTIVE) {
      promotions_ui.push_back(item.second->Clone());
    }
  }
  callback(type::Result::LEDGER_OK, std::move(promotions_ui));
}

void Promotion::LegacyClaimedSaved(
    const type::Result result,
    std::shared_ptr<type::PromotionPtr> shared_promotion) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Save failed");
    return;
  }

  GetCredentials(std::move(*shared_promotion), [](const type::Result _){});
}

void Promotion::Claim(
    const std::string& promotion_id,
    const std::string& payload,
    ledger::ClaimPromotionCallback callback) {
  auto promotion_callback = std::bind(&Promotion::OnClaimPromotion,
      this,
      _1,
      payload,
      callback);

  ledger_->database()->GetPromotion(promotion_id, promotion_callback);
}

void Promotion::OnClaimPromotion(
    type::PromotionPtr promotion,
    const std::string& payload,
    ledger::ClaimPromotionCallback callback) {
  if (!promotion) {
    BLOG(0, "Promotion is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  if (promotion->status != type::PromotionStatus::ACTIVE) {
    BLOG(1, "Promotion already in progress");
    callback(type::Result::IN_PROGRESS, "");
    return;
  }

  const auto wallet = ledger_->wallet()->GetWallet();
  if (wallet) {
    attestation_->Start(payload, callback);
    return;
  }

  ledger_->wallet()->CreateWalletIfNecessary(
      [this, payload, callback](const type::Result result) {
        if (result != type::Result::WALLET_CREATED) {
          BLOG(0, "Wallet couldn't be created");
          callback(type::Result::LEDGER_ERROR, "");
          return;
        }

        attestation_->Start(payload, callback);
      });
}

void Promotion::Attest(
    const std::string& promotion_id,
    const std::string& solution,
    ledger::AttestPromotionCallback callback) {
  auto promotion_callback = std::bind(&Promotion::OnAttestPromotion,
      this,
      _1,
      solution,
      callback);

  ledger_->database()->GetPromotion(promotion_id, promotion_callback);
}

void Promotion::OnAttestPromotion(
    type::PromotionPtr promotion,
    const std::string& solution,
    ledger::AttestPromotionCallback callback) {
  if (!promotion) {
    BLOG(1, "Promotion is null");
    callback(type::Result::LEDGER_ERROR, nullptr);
    return;
  }

  if (promotion->status != type::PromotionStatus::ACTIVE) {
    BLOG(1, "Promotion already in progress");
    callback(type::Result::IN_PROGRESS, nullptr);
    return;
  }

  auto confirm_callback = std::bind(&Promotion::OnAttestedPromotion,
      this,
      _1,
      promotion->id,
      callback);
  attestation_->Confirm(solution, confirm_callback);
}

void Promotion::OnAttestedPromotion(
    const type::Result result,
    const std::string& promotion_id,
    ledger::AttestPromotionCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Attestation failed " << result);
    callback(result, nullptr);
    return;
  }

  auto promotion_callback = std::bind(&Promotion::OnCompletedAttestation,
      this,
      _1,
      callback);

  ledger_->database()->GetPromotion(promotion_id, promotion_callback);
}

void Promotion::OnCompletedAttestation(
    type::PromotionPtr promotion,
    ledger::AttestPromotionCallback callback) {
  if (!promotion) {
    BLOG(0, "Promotion does not exist");
    callback(type::Result::LEDGER_ERROR, nullptr);
    return;
  }

  if (promotion->status == type::PromotionStatus::FINISHED) {
    BLOG(0, "Promotions already claimed");
    callback(type::Result::GRANT_ALREADY_CLAIMED, nullptr);
    return;
  }

  promotion->status = type::PromotionStatus::ATTESTED;

  auto save_callback = std::bind(&Promotion::AttestedSaved,
      this,
      _1,
      std::make_shared<type::PromotionPtr>(promotion->Clone()),
      callback);

  ledger_->database()->SavePromotion(promotion->Clone(), save_callback);
}

void Promotion::AttestedSaved(
    const type::Result result,
    std::shared_ptr<type::PromotionPtr> shared_promotion,
    ledger::AttestPromotionCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Save failed ");
    callback(result, nullptr);
    return;
  }
  if (!shared_promotion) {
    BLOG(1, "Promotion is null");
    callback(type::Result::LEDGER_ERROR, nullptr);
    return;
  }

  auto claim_callback = std::bind(&Promotion::Complete,
      this,
      _1,
      (*shared_promotion)->id,
      callback);

  GetCredentials((*shared_promotion)->Clone(), claim_callback);
}

void Promotion::Complete(
    const type::Result result,
    const std::string& promotion_id,
    ledger::AttestPromotionCallback callback) {
  auto promotion_callback = std::bind(&Promotion::OnComplete,
      this,
      _1,
      result,
      callback);
  ledger_->database()->GetPromotion(promotion_id, promotion_callback);
}

void Promotion::OnComplete(
    type::PromotionPtr promotion,
    const type::Result result,
    ledger::AttestPromotionCallback callback) {
    BLOG(1, "Promotion completed with result " << result);
  if (promotion && result == type::Result::LEDGER_OK) {
    ledger_->database()->SaveBalanceReportInfoItem(
        util::GetCurrentMonth(),
        util::GetCurrentYear(),
        ConvertPromotionTypeToReportType(promotion->type),
        promotion->approximate_value,
        [](const type::Result){});
  }

  callback(result, std::move(promotion));
}

void Promotion::ProcessFetchedPromotions(
    const type::Result result,
    type::PromotionList promotions,
    ledger::FetchPromotionCallback callback) {
  const uint64_t now = util::GetCurrentTimeStamp();
  ledger_->state()->SetPromotionLastFetchStamp(now);
  last_check_timer_.Stop();
  const bool retry = result != type::Result::LEDGER_OK &&
      result != type::Result::NOT_FOUND;
  Refresh(retry);
  callback(result, std::move(promotions));
}

void Promotion::GetCredentials(
    type::PromotionPtr promotion,
    ledger::ResultCallback callback) {
  if (!promotion) {
    BLOG(0, "Promotion is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  credential::CredentialsTrigger trigger;
  trigger.id = promotion->id;
  trigger.size = promotion->suggestions;
  trigger.type = type::CredsBatchType::PROMOTION;

  auto creds_callback = std::bind(&Promotion::CredentialsProcessed,
      this,
      _1,
      promotion->id,
      callback);

  credentials_->Start(trigger, creds_callback);
}

void Promotion::CredentialsProcessed(
    const type::Result result,
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  if (result == type::Result::RETRY) {
    retry_timer_.Start(FROM_HERE, base::Seconds(5),
                       base::BindOnce(&Promotion::OnRetryTimerElapsed,
                                      base::Unretained(this)));
    callback(type::Result::LEDGER_OK);
    return;
  }

  if (result == type::Result::NOT_FOUND) {
    ledger_->database()->UpdatePromotionStatus(
      promotion_id,
      type::PromotionStatus::OVER,
      callback);
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Credentials process not succeeded " << result);
    callback(result);
    return;
  }

  ledger_->database()->UpdatePromotionStatus(
      promotion_id,
      type::PromotionStatus::FINISHED,
      callback);
}

void Promotion::Retry(type::PromotionMap promotions) {
  HandleExpiredPromotions(ledger_, &promotions);

  for (auto& promotion : promotions) {
    if (!promotion.second) {
      continue;
    }

    switch (promotion.second->status) {
      case type::PromotionStatus::ATTESTED: {
        GetCredentials(
            std::move(promotion.second),
            [](const type::Result _){});
        break;
      }
      case type::PromotionStatus::ACTIVE:
      case type::PromotionStatus::FINISHED:
      case type::PromotionStatus::CORRUPTED:
      case type::PromotionStatus::OVER: {
        break;
      }
    }
  }
}

void Promotion::Refresh(const bool retry_after_error) {
  if (last_check_timer_.IsRunning()) {
    return;
  }

  base::TimeDelta start_timer_in;

  if (retry_after_error) {
    start_timer_in = util::GetRandomizedDelay(base::Seconds(300));

    BLOG(1, "Failed to refresh promotion, will try again in "
        << start_timer_in);
  } else {
    const auto default_time = constant::kPromotionRefreshInterval;
    const uint64_t now = util::GetCurrentTimeStamp();
    const uint64_t last_promo_stamp =
        ledger_->state()->GetPromotionLastFetchStamp();

    uint64_t time_since_last_promo_check = 0ull;

    if (last_promo_stamp != 0ull && last_promo_stamp < now) {
      time_since_last_promo_check = now - last_promo_stamp;
    }

    if (now == last_promo_stamp) {
      start_timer_in = base::Seconds(default_time);
    } else if (time_since_last_promo_check > 0 &&
        default_time > time_since_last_promo_check) {
      start_timer_in =
          base::Seconds(default_time - time_since_last_promo_check);
    }
  }

  last_check_timer_.Start(FROM_HERE, start_timer_in,
      base::BindOnce(&Promotion::OnLastCheckTimerElapsed,
          base::Unretained(this)));
}

void Promotion::CheckForCorrupted(const type::PromotionMap& promotions) {
  if (promotions.empty()) {
    BLOG(1, "Promotion is empty");
    return;
  }

  std::vector<std::string> corrupted_promotions;

  for (const auto& item : promotions) {
    if (!item.second ||
        item.second->status != type::PromotionStatus::ATTESTED) {
      continue;
    }

    if (item.second->public_keys.empty() ||
        item.second->public_keys == "[]") {
      corrupted_promotions.push_back(item.second->id);
    }
  }

  if (corrupted_promotions.empty()) {
    BLOG(1, "No corrupted promotions");
    CorruptedPromotionFixed(type::Result::LEDGER_OK);
    return;
  }

  auto get_callback = std::bind(&Promotion::CorruptedPromotionFixed,
      this,
      _1);

  ledger_->database()->UpdatePromotionsBlankPublicKey(
      corrupted_promotions,
      get_callback);
}

void Promotion::CorruptedPromotionFixed(const type::Result result) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Could not update public keys");
    return;
  }

  auto check_callback = std::bind(&Promotion::CheckForCorruptedCreds,
        this,
        _1);

  ledger_->database()->GetAllCredsBatches(check_callback);
}

void Promotion::CheckForCorruptedCreds(type::CredsBatchList list) {
  if (list.empty()) {
    BLOG(1, "Creds list is empty");
    ledger_->state()->SetPromotionCorruptedMigrated(true);
    return;
  }

  std::vector<std::string> corrupted_promotions;

  for (auto& item : list) {
    if (!item ||
        (item->status != type::CredsBatchStatus::SIGNED &&
         item->status != type::CredsBatchStatus::FINISHED)) {
      continue;
    }

    std::vector<std::string> unblinded_encoded_tokens;
    std::string error;
    bool result = credential::UnBlindCreds(
        *item,
        &unblinded_encoded_tokens,
        &error);

    if (!result) {
      BLOG(1, "Promotion corrupted " << item->trigger_id);
      corrupted_promotions.push_back(item->trigger_id);
    }
  }

  if (corrupted_promotions.empty()) {
    BLOG(1, "No corrupted creds");
    ledger_->state()->SetPromotionCorruptedMigrated(true);
    return;
  }

  auto get_callback = std::bind(&Promotion::CorruptedPromotions,
      this,
      _1,
      corrupted_promotions);

  ledger_->database()->GetPromotionList(corrupted_promotions, get_callback);
}

void Promotion::CorruptedPromotions(
    type::PromotionList promotions,
    const std::vector<std::string>& ids) {
  base::Value corrupted_claims(base::Value::Type::LIST);

  for (auto& item : promotions) {
    if (!item) {
      continue;
    }

    corrupted_claims.Append(base::Value(item->claim_id));
  }

  if (corrupted_claims.GetList().empty()) {
    BLOG(1, "No corrupted creds");
    ledger_->state()->SetPromotionCorruptedMigrated(true);
    return;
  }

  auto url_callback = std::bind(&Promotion::OnCheckForCorrupted,
      this,
      _1,
      ids);

  promotion_server_->post_clobbered_claims()->Request(
      std::move(corrupted_claims),
      url_callback);
}

void Promotion::OnCheckForCorrupted(
    const type::Result result,
    const std::vector<std::string>& promotion_id_list) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Failed to parse corrupted promotions response");
    return;
  }

  ledger_->state()->SetPromotionCorruptedMigrated(true);

  auto update_callback = std::bind(&Promotion::ErrorStatusSaved,
      this,
      _1,
      promotion_id_list);

  ledger_->database()->UpdatePromotionsStatus(
      promotion_id_list,
      type::PromotionStatus::CORRUPTED,
      update_callback);
}

void Promotion::ErrorStatusSaved(
    const type::Result result,
    const std::vector<std::string>& promotion_id_list) {
  // even if promotions fail, let's try to update at least creds
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Promotion status save failed");
  }

  auto update_callback = std::bind(&Promotion::ErrorCredsStatusSaved,
      this,
      _1);

  ledger_->database()->UpdateCredsBatchesStatus(
      promotion_id_list,
      type::CredsBatchType::PROMOTION,
      type::CredsBatchStatus::CORRUPTED,
      update_callback);
}

void Promotion::ErrorCredsStatusSaved(const type::Result result) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Creds status save failed");
  }

  // let's retry promotions that are valid now
  auto retry_callback = std::bind(&Promotion::Retry,
    this,
    _1);

  ledger_->database()->GetAllPromotions(retry_callback);
}

void Promotion::TransferTokens(ledger::PostSuggestionsClaimCallback callback) {
  transfer_->Start(callback);
}

void Promotion::OnRetryTimerElapsed() {
  ledger_->database()->GetAllPromotions(std::bind(&Promotion::Retry, this, _1));
}

void Promotion::OnLastCheckTimerElapsed() {
  Fetch([](type::Result, type::PromotionList) {});
}

void Promotion::GetTransferableAmount(
    ledger::GetTransferableAmountCallback callback) {
  transfer_->GetAmount(callback);
}

void Promotion::GetDrainStatus(const std::string& drain_id,
                               ledger::GetDrainCallback callback) {
  promotion_server_->get_drain()->Request(drain_id, callback);
}

}  // namespace promotion
}  // namespace ledger
