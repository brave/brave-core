/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/promotion/promotion.h"

#include <map>
#include <memory>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/legacy/wallet_info_properties.h"
#include "brave/components/brave_rewards/core/promotion/promotion_transfer.h"
#include "brave/components/brave_rewards/core/promotion/promotion_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"

namespace brave_rewards::internal {
namespace promotion {

namespace {

const int kFetchPromotionsThresholdInSeconds =
    10 * base::Time::kSecondsPerMinute;

void HandleExpiredPromotions(
    RewardsEngineImpl& engine_impl,
    base::flat_map<std::string, mojom::PromotionPtr>* promotions) {
  DCHECK(promotions);
  if (!promotions) {
    return;
  }

  const uint64_t current_time = util::GetCurrentTimeStamp();

  for (auto& item : *promotions) {
    if (!item.second || item.second->status == mojom::PromotionStatus::OVER) {
      continue;
    }

    // we shouldn't expire ad grant
    if (item.second->type == mojom::PromotionType::ADS) {
      continue;
    }

    if (item.second->expires_at > 0 &&
        item.second->expires_at <= current_time) {
      engine_impl.database()->UpdatePromotionStatus(
          item.second->id, mojom::PromotionStatus::OVER, base::DoNothing());
    }
  }
}

}  // namespace

Promotion::Promotion(RewardsEngineImpl& engine)
    : engine_(engine),
      attestation_(engine),
      transfer_(engine),
      credentials_(engine),
      promotion_server_(engine) {}

Promotion::~Promotion() = default;

void Promotion::Initialize() {
  if (!engine_->state()->GetPromotionCorruptedMigrated()) {
    engine_->Log(FROM_HERE) << "Migrating corrupted promotions";

    engine_->database()->GetAllPromotions(base::BindOnce(
        &Promotion::CheckForCorrupted, weak_factory_.GetWeakPtr()));
  }

  engine_->database()->GetAllPromotions(
      base::BindOnce(&Promotion::Retry, weak_factory_.GetWeakPtr()));
}

void Promotion::Fetch(FetchPromotionsCallback callback) {
  // If we fetched promotions recently, fulfill this request from the
  // database instead of querying the server again
  auto env = engine_->Get<EnvironmentConfig>().current_environment();
  if (!engine_->options().is_testing && env != mojom::Environment::kStaging) {
    const uint64_t last_promo_stamp =
        engine_->state()->GetPromotionLastFetchStamp();
    const uint64_t now = util::GetCurrentTimeStamp();
    if (now - last_promo_stamp < kFetchPromotionsThresholdInSeconds) {
      engine_->database()->GetAllPromotions(
          base::BindOnce(&Promotion::OnGetAllPromotionsFromDatabase,
                         weak_factory_.GetWeakPtr(), std::move(callback)));
      return;
    }
  }

  auto client_info = engine_->GetClientInfo();
  const std::string client = ParseClientInfoToString(std::move(client_info));
  promotion_server_.get_available().Request(
      client, base::BindOnce(&Promotion::OnFetch, weak_factory_.GetWeakPtr(),
                             std::move(callback)));
}

void Promotion::OnFetch(FetchPromotionsCallback callback,
                        mojom::Result result,
                        std::vector<mojom::PromotionPtr> list,
                        std::vector<std::string> corrupted_promotions) {
  if (result == mojom::Result::NOT_FOUND) {
    ProcessFetchedPromotions(mojom::Result::NOT_FOUND, std::move(list),
                             std::move(callback));
    return;
  }

  if (result == mojom::Result::FAILED) {
    ProcessFetchedPromotions(mojom::Result::FAILED, std::move(list),
                             std::move(callback));
    return;
  }

  // even though that some promotions are corrupted
  // we should display non corrupted ones either way
  if (result == mojom::Result::CORRUPTED_DATA) {
    engine_->Log(FROM_HERE) << "Promotions are not correct: "
                            << base::JoinString(corrupted_promotions, ", ");
  }

  engine_->database()->GetAllPromotions(
      base::BindOnce(&Promotion::OnGetAllPromotions, weak_factory_.GetWeakPtr(),
                     std::move(callback), std::move(list)));
}

void Promotion::OnGetAllPromotions(
    FetchPromotionsCallback callback,
    std::vector<mojom::PromotionPtr> list,
    base::flat_map<std::string, mojom::PromotionPtr> promotions) {
  HandleExpiredPromotions(*engine_, &promotions);

  std::vector<mojom::PromotionPtr> promotions_ui;
  for (const auto& item : list) {
    auto it = promotions.find(item->id);
    if (it != promotions.end()) {
      const auto status = it->second->status;
      promotions.erase(item->id);
      // Skip any promotions that are in the database and have been processed
      // in some way.
      if (status != mojom::PromotionStatus::ACTIVE &&
          status != mojom::PromotionStatus::OVER) {
        continue;
      }
    }

    // if the server return expiration for ads we need to set it to 0
    if (item->type == mojom::PromotionType::ADS) {
      item->expires_at = 0;
    }

    if (item->legacy_claimed) {
      item->status = mojom::PromotionStatus::ATTESTED;
      engine_->database()->SavePromotion(
          item->Clone(),
          base::BindOnce(&Promotion::LegacyClaimedSaved,
                         weak_factory_.GetWeakPtr(), item->Clone()));
      continue;
    }

    promotions_ui.push_back(item->Clone());

    engine_->database()->SavePromotion(item->Clone(), base::DoNothing());
  }

  // mark as over promotions that are in db with status active,
  // but are not available on the server anymore
  for (const auto& promotion : promotions) {
    if (promotion.second->status != mojom::PromotionStatus::ACTIVE) {
      continue;
    }

    bool found = std::any_of(
        list.begin(), list.end(),
        [&promotion](auto& item) { return item->id == promotion.second->id; });

    if (!found) {
      engine_->database()->UpdatePromotionStatus(promotion.second->id,
                                                 mojom::PromotionStatus::OVER,
                                                 base::DoNothing());
    }
  }

  ProcessFetchedPromotions(mojom::Result::OK, std::move(promotions_ui),
                           std::move(callback));
}

void Promotion::OnGetAllPromotionsFromDatabase(
    FetchPromotionsCallback callback,
    base::flat_map<std::string, mojom::PromotionPtr> promotions) {
  HandleExpiredPromotions(*engine_, &promotions);

  std::vector<mojom::PromotionPtr> promotions_ui;
  for (const auto& item : promotions) {
    if (item.second->status == mojom::PromotionStatus::ACTIVE) {
      promotions_ui.push_back(item.second->Clone());
    }
  }
  std::move(callback).Run(mojom::Result::OK, std::move(promotions_ui));
}

void Promotion::LegacyClaimedSaved(mojom::PromotionPtr promotion,
                                   mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Save failed";
    return;
  }

  GetCredentials(base::DoNothing(), std::move(promotion));
}

void Promotion::Claim(const std::string& promotion_id,
                      const std::string& payload,
                      ClaimPromotionCallback callback) {
  engine_->database()->GetPromotion(
      promotion_id,
      base::BindOnce(&Promotion::OnClaimPromotion, weak_factory_.GetWeakPtr(),
                     std::move(callback), payload));
}

void Promotion::OnClaimPromotion(ClaimPromotionCallback callback,
                                 const std::string& payload,
                                 mojom::PromotionPtr promotion) {
  if (!promotion) {
    engine_->LogError(FROM_HERE) << "Promotion is null";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  if (promotion->status != mojom::PromotionStatus::ACTIVE) {
    engine_->Log(FROM_HERE) << "Promotion already in progress";
    std::move(callback).Run(mojom::Result::IN_PROGRESS, "");
    return;
  }

  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << "Rewards wallet does not exist";
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  attestation_.Start(payload, std::move(callback));
}

void Promotion::Attest(const std::string& promotion_id,
                       const std::string& solution,
                       AttestPromotionCallback callback) {
  engine_->database()->GetPromotion(
      promotion_id,
      base::BindOnce(&Promotion::OnAttestPromotion, weak_factory_.GetWeakPtr(),
                     std::move(callback), solution));
}

void Promotion::OnAttestPromotion(AttestPromotionCallback callback,
                                  const std::string& solution,
                                  mojom::PromotionPtr promotion) {
  if (!promotion) {
    engine_->Log(FROM_HERE) << "Promotion is null";
    std::move(callback).Run(mojom::Result::FAILED, nullptr);
    return;
  }

  if (promotion->status != mojom::PromotionStatus::ACTIVE) {
    engine_->Log(FROM_HERE) << "Promotion already in progress";
    std::move(callback).Run(mojom::Result::IN_PROGRESS, nullptr);
    return;
  }

  attestation_.Confirm(
      solution, base::BindOnce(&Promotion::OnAttestedPromotion,
                               weak_factory_.GetWeakPtr(), std::move(callback),
                               promotion->id));
}

void Promotion::OnAttestedPromotion(AttestPromotionCallback callback,
                                    const std::string& promotion_id,
                                    mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Attestation failed " << result;
    std::move(callback).Run(result, nullptr);
    return;
  }

  engine_->database()->GetPromotion(
      promotion_id,
      base::BindOnce(&Promotion::OnCompletedAttestation,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void Promotion::OnCompletedAttestation(AttestPromotionCallback callback,
                                       mojom::PromotionPtr promotion) {
  if (!promotion) {
    engine_->LogError(FROM_HERE) << "Promotion does not exist";
    std::move(callback).Run(mojom::Result::FAILED, nullptr);
    return;
  }

  if (promotion->status == mojom::PromotionStatus::FINISHED) {
    engine_->LogError(FROM_HERE) << "Promotions already claimed";
    std::move(callback).Run(mojom::Result::GRANT_ALREADY_CLAIMED, nullptr);
    return;
  }

  promotion->status = mojom::PromotionStatus::ATTESTED;

  auto save_callback =
      base::BindOnce(&Promotion::AttestedSaved, weak_factory_.GetWeakPtr(),
                     std::move(callback), promotion->Clone());

  engine_->database()->SavePromotion(std::move(promotion),
                                     std::move(save_callback));
}

void Promotion::AttestedSaved(AttestPromotionCallback callback,
                              mojom::PromotionPtr promotion,
                              mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Save failed ";
    std::move(callback).Run(result, nullptr);
    return;
  }

  auto claim_callback =
      base::BindOnce(&Promotion::Complete, weak_factory_.GetWeakPtr(),
                     std::move(callback), promotion->id);

  GetCredentials(std::move(claim_callback), std::move(promotion));
}

void Promotion::Complete(AttestPromotionCallback callback,
                         const std::string& promotion_id,
                         mojom::Result result) {
  engine_->database()->GetPromotion(
      promotion_id,
      base::BindOnce(&Promotion::OnComplete, weak_factory_.GetWeakPtr(),
                     std::move(callback), result));
}

void Promotion::OnComplete(AttestPromotionCallback callback,
                           mojom::Result result,
                           mojom::PromotionPtr promotion) {
  engine_->Log(FROM_HERE) << "Promotion completed with result " << result;
  if (promotion && result == mojom::Result::OK) {
    engine_->database()->SaveBalanceReportInfoItem(
        util::GetCurrentMonth(), util::GetCurrentYear(),
        ConvertPromotionTypeToReportType(promotion->type),
        promotion->approximate_value, base::DoNothing());
  }

  std::move(callback).Run(result, std::move(promotion));
}

void Promotion::ProcessFetchedPromotions(
    const mojom::Result result,
    std::vector<mojom::PromotionPtr> promotions,
    FetchPromotionsCallback callback) {
  const uint64_t now = util::GetCurrentTimeStamp();
  engine_->state()->SetPromotionLastFetchStamp(now);
  last_check_timer_.Stop();
  const bool retry =
      result != mojom::Result::OK && result != mojom::Result::NOT_FOUND;
  Refresh(retry);
  std::move(callback).Run(result, std::move(promotions));
}

void Promotion::GetCredentials(ResultCallback callback,
                               mojom::PromotionPtr promotion) {
  if (!promotion) {
    engine_->LogError(FROM_HERE) << "Promotion is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  credential::CredentialsTrigger trigger;
  trigger.id = promotion->id;
  trigger.size = promotion->suggestions;
  trigger.type = mojom::CredsBatchType::PROMOTION;

  credentials_.Start(
      trigger, base::BindOnce(&Promotion::CredentialsProcessed,
                              weak_factory_.GetWeakPtr(), std::move(callback),
                              promotion->id));
}

void Promotion::CredentialsProcessed(ResultCallback callback,
                                     const std::string& promotion_id,
                                     mojom::Result result) {
  if (result == mojom::Result::RETRY) {
    retry_timer_.Start(FROM_HERE, base::Seconds(5),
                       base::BindOnce(&Promotion::OnRetryTimerElapsed,
                                      weak_factory_.GetWeakPtr()));
    std::move(callback).Run(mojom::Result::OK);
    return;
  }

  if (result == mojom::Result::NOT_FOUND) {
    engine_->database()->UpdatePromotionStatus(
        promotion_id, mojom::PromotionStatus::OVER, std::move(callback));
    return;
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE)
        << "Credentials process not succeeded " << result;
    std::move(callback).Run(result);
    return;
  }

  engine_->database()->UpdatePromotionStatus(
      promotion_id, mojom::PromotionStatus::FINISHED, std::move(callback));
}

void Promotion::Retry(
    base::flat_map<std::string, mojom::PromotionPtr> promotions) {
  HandleExpiredPromotions(*engine_, &promotions);

  for (auto& promotion : promotions) {
    if (!promotion.second) {
      continue;
    }

    switch (promotion.second->status) {
      case mojom::PromotionStatus::ATTESTED: {
        GetCredentials(base::DoNothing(), std::move(promotion.second));
        break;
      }
      case mojom::PromotionStatus::ACTIVE:
      case mojom::PromotionStatus::FINISHED:
      case mojom::PromotionStatus::CORRUPTED:
      case mojom::PromotionStatus::OVER: {
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

    engine_->Log(FROM_HERE)
        << "Failed to refresh promotion, will try again in " << start_timer_in;
  } else {
    const auto default_time = constant::kPromotionRefreshInterval;
    const uint64_t now = util::GetCurrentTimeStamp();
    const uint64_t last_promo_stamp =
        engine_->state()->GetPromotionLastFetchStamp();

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
                                         weak_factory_.GetWeakPtr()));
}

void Promotion::CheckForCorrupted(
    base::flat_map<std::string, mojom::PromotionPtr> promotions) {
  if (promotions.empty()) {
    engine_->Log(FROM_HERE) << "Promotion is empty";
    return;
  }

  std::vector<std::string> corrupted_promotions;

  for (const auto& item : promotions) {
    if (!item.second ||
        item.second->status != mojom::PromotionStatus::ATTESTED) {
      continue;
    }

    if (item.second->public_keys.empty() || item.second->public_keys == "[]") {
      corrupted_promotions.push_back(item.second->id);
    }
  }

  if (corrupted_promotions.empty()) {
    engine_->Log(FROM_HERE) << "No corrupted promotions";
    CorruptedPromotionFixed(mojom::Result::OK);
    return;
  }

  engine_->database()->UpdatePromotionsBlankPublicKey(
      corrupted_promotions, base::BindOnce(&Promotion::CorruptedPromotionFixed,
                                           weak_factory_.GetWeakPtr()));
}

void Promotion::CorruptedPromotionFixed(mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Could not update public keys";
    return;
  }

  engine_->database()->GetAllCredsBatches(base::BindOnce(
      &Promotion::CheckForCorruptedCreds, weak_factory_.GetWeakPtr()));
}

void Promotion::CheckForCorruptedCreds(std::vector<mojom::CredsBatchPtr> list) {
  if (list.empty()) {
    engine_->Log(FROM_HERE) << "Creds list is empty";
    engine_->state()->SetPromotionCorruptedMigrated(true);
    return;
  }

  std::vector<std::string> corrupted_promotions;

  for (auto& item : list) {
    if (!item || (item->status != mojom::CredsBatchStatus::SIGNED &&
                  item->status != mojom::CredsBatchStatus::FINISHED)) {
      continue;
    }

    auto unblinded_encoded_tokens = credential::UnBlindCreds(*item);

    if (!unblinded_encoded_tokens.has_value()) {
      engine_->Log(FROM_HERE) << "Promotion corrupted " << item->trigger_id;
      corrupted_promotions.push_back(item->trigger_id);
    }
  }

  if (corrupted_promotions.empty()) {
    engine_->Log(FROM_HERE) << "No corrupted creds";
    engine_->state()->SetPromotionCorruptedMigrated(true);
    return;
  }

  engine_->database()->GetPromotionList(
      corrupted_promotions,
      base::BindOnce(&Promotion::CorruptedPromotions,
                     weak_factory_.GetWeakPtr(), corrupted_promotions));
}

void Promotion::CorruptedPromotions(
    std::vector<std::string> ids,
    std::vector<mojom::PromotionPtr> promotions) {
  base::Value::List corrupted_claims;

  for (auto& item : promotions) {
    if (!item) {
      continue;
    }

    corrupted_claims.Append(base::Value(item->claim_id));
  }

  if (corrupted_claims.empty()) {
    engine_->Log(FROM_HERE) << "No corrupted creds";
    engine_->state()->SetPromotionCorruptedMigrated(true);
    return;
  }

  promotion_server_.post_clobbered_claims().Request(
      std::move(corrupted_claims),
      base::BindOnce(&Promotion::OnCheckForCorrupted,
                     weak_factory_.GetWeakPtr(), std::move(ids)));
}

void Promotion::OnCheckForCorrupted(std::vector<std::string> promotion_id_list,
                                    mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE)
        << "Failed to parse corrupted promotions response";
    return;
  }

  engine_->state()->SetPromotionCorruptedMigrated(true);

  auto update_callback =
      base::BindOnce(&Promotion::ErrorStatusSaved, weak_factory_.GetWeakPtr(),
                     promotion_id_list);

  engine_->database()->UpdatePromotionsStatus(promotion_id_list,
                                              mojom::PromotionStatus::CORRUPTED,
                                              std::move(update_callback));
}

void Promotion::ErrorStatusSaved(std::vector<std::string> promotion_id_list,
                                 mojom::Result result) {
  // even if promotions fail, let's try to update at least creds
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Promotion status save failed";
  }

  engine_->database()->UpdateCredsBatchesStatus(
      promotion_id_list, mojom::CredsBatchType::PROMOTION,
      mojom::CredsBatchStatus::CORRUPTED,
      base::BindOnce(&Promotion::ErrorCredsStatusSaved,
                     weak_factory_.GetWeakPtr()));
}

void Promotion::ErrorCredsStatusSaved(mojom::Result result) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Creds status save failed";
  }

  // let's retry promotions that are valid now
  engine_->database()->GetAllPromotions(
      base::BindOnce(&Promotion::Retry, weak_factory_.GetWeakPtr()));
}

void Promotion::TransferTokens(PostSuggestionsClaimCallback callback) {
  transfer_.Start(std::move(callback));
}

void Promotion::OnRetryTimerElapsed() {
  engine_->database()->GetAllPromotions(
      base::BindOnce(&Promotion::Retry, weak_factory_.GetWeakPtr()));
}

void Promotion::OnLastCheckTimerElapsed() {
  Fetch(base::DoNothing());
}

}  // namespace promotion
}  // namespace brave_rewards::internal
