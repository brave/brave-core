/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/confirmations/confirmations.h"

#include <stdint.h>

#include <functional>

#include "base/time/time.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"
#include "bat/ads/internal/confirmations/confirmations_state.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/redeem_unblinded_token.h"

namespace ads {

namespace {
const int64_t kRetryAfterSeconds = 5 * base::Time::kSecondsPerMinute;
}  // namespace

Confirmations::Confirmations()
    : confirmations_state_(std::make_unique<ConfirmationsState>()),
      redeem_unblinded_token_(std::make_unique<RedeemUnblindedToken>()) {
  redeem_unblinded_token_->set_delegate(this);
}

Confirmations::~Confirmations() {
  redeem_unblinded_token_->set_delegate(nullptr);
}

void Confirmations::set_ad_rewards(
    AdRewards* ad_rewards) {
  ConfirmationsState::Get()->set_ad_rewards(ad_rewards);
}

void Confirmations::AddObserver(
    ConfirmationsObserver* observer) {
  observers_.AddObserver(observer);
}

void Confirmations::RemoveObserver(
    ConfirmationsObserver* observer) {
  observers_.RemoveObserver(observer);
}

void Confirmations::SetCatalogIssuers(
    const CatalogIssuersInfo& catalog_issuers) {
  BLOG(1, "SetCatalogIssuers:");
  BLOG(1, "  Public key: " << catalog_issuers.public_key);
  BLOG(1, "  Issuers:");

  for (const auto& issuer : catalog_issuers.issuers) {
    BLOG(1, "    Name: " << issuer.name);
    BLOG(1, "    Public key: " << issuer.public_key);
  }

  const CatalogIssuersInfo current_catalog_issuers =
      ConfirmationsState::Get()->get_catalog_issuers();

  const bool public_key_was_rotated =
      !current_catalog_issuers.public_key.empty() &&
          current_catalog_issuers.public_key != catalog_issuers.public_key;

  ConfirmationsState::Get()->set_catalog_issuers(catalog_issuers);

  if (public_key_was_rotated) {
    ConfirmationsState::Get()->get_unblinded_tokens()->RemoveAllTokens();
  }

  ConfirmationsState::Get()->Save();
}

void Confirmations::ConfirmAd(
    const std::string& creative_instance_id,
    const ConfirmationType& confirmation_type) {
  BLOG(1, "Confirming " << std::string(confirmation_type)
      << " ad for creative instance id " << creative_instance_id);

  if (ConfirmationsState::Get()->get_unblinded_tokens()->IsEmpty()) {
    BLOG(1, "There are no unblinded tokens");

    BLOG(3, "Failed to confirm ad with creative instance id "
        << creative_instance_id);

    return;
  }

  const privacy::UnblindedTokenInfo unblinded_token
      = ConfirmationsState::Get()->get_unblinded_tokens()->GetToken();

  ConfirmationsState::Get()->get_unblinded_tokens()->RemoveToken(
      unblinded_token);
  ConfirmationsState::Get()->Save();

  redeem_unblinded_token_->Redeem(creative_instance_id,
      confirmation_type, unblinded_token);
}

void Confirmations::RetryAfterDelay() {
  if (retry_timer_.IsRunning()) {
    return;
  }

  const base::Time time = retry_timer_.StartWithPrivacy(
      base::TimeDelta::FromSeconds(kRetryAfterSeconds),
          base::BindOnce(&Confirmations::Retry, base::Unretained(this)));

  BLOG(1, "Retry failed confirmations " << FriendlyDateAndTime(time));
}

///////////////////////////////////////////////////////////////////////////////

void Confirmations::CreateNewConfirmationAndAppendToRetryQueue(
    const ConfirmationInfo& confirmation) {
  if (ConfirmationsState::Get()->get_unblinded_tokens()->IsEmpty()) {
    AppendToRetryQueue(confirmation);
    return;
  }

  const privacy::UnblindedTokenInfo unblinded_token =
      ConfirmationsState::Get()->get_unblinded_tokens()->GetToken();

  ConfirmationsState::Get()->get_unblinded_tokens()->RemoveToken(
      unblinded_token);
  ConfirmationsState::Get()->Save();

  const ConfirmationInfo new_confirmation = CreateConfirmationInfo(
      confirmation.creative_instance_id, confirmation.type, unblinded_token);

  AppendToRetryQueue(new_confirmation);
}

void Confirmations::AppendToRetryQueue(
    const ConfirmationInfo& confirmation) {
  ConfirmationsState::Get()->append_failed_confirmation(confirmation);
  ConfirmationsState::Get()->Save();

  BLOG(1, "Added confirmation id " << confirmation.id << ", creative instance "
      "id " << confirmation.creative_instance_id << " and "
          << std::string(confirmation.type) << " to the confirmations queue");
}

void Confirmations::RemoveFromRetryQueue(
    const ConfirmationInfo& confirmation) {
  if (!ConfirmationsState::Get()->remove_failed_confirmation(confirmation)) {
    BLOG(0, "Failed to remove confirmation id " << confirmation.id
        << ", creative instance id " << confirmation.creative_instance_id
            << " and " << std::string(confirmation.type)
                << " from the confirmations queue");

    return;
  }

  BLOG(1, "Removed confirmation id " << confirmation.id << ", creative "
      "instance id " << confirmation.creative_instance_id << " and "
          << std::string(confirmation.type) << " from the confirmations queue");

  ConfirmationsState::Get()->Save();
}

void Confirmations::Retry() {
  ConfirmationList failed_confirmations =
      ConfirmationsState::Get()->get_failed_confirmations();
  if (failed_confirmations.empty()) {
    BLOG(1, "No failed confirmations to retry");
    return;
  }

  ConfirmationInfo confirmation = failed_confirmations.front();
  RemoveFromRetryQueue(confirmation);

  redeem_unblinded_token_->Redeem(confirmation);

  RetryAfterDelay();
}

void Confirmations::OnDidRedeemUnblindedToken(
    const ConfirmationInfo& confirmation,
    const privacy::UnblindedTokenInfo& unblinded_payment_token) {
  if (ConfirmationsState::Get()->get_unblinded_payment_tokens()->
      TokenExists(unblinded_payment_token)) {
    BLOG(1, "Unblinded payment token is a duplicate");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
    return;
  }

  BLOG(1, "Successfully redeemed unblinded token with confirmation id "
      << confirmation.id << ", creative instance id "
          << confirmation.creative_instance_id << " and "
              << std::string(confirmation.type));

  ConfirmationsState::Get()->get_unblinded_payment_tokens()->AddTokens(
      {unblinded_payment_token});
  ConfirmationsState::Get()->Save();

  const CatalogIssuersInfo catalog_issuers;

  const base::Optional<double> estimated_redemption_value =
      catalog_issuers.GetEstimatedRedemptionValue(
          unblinded_payment_token.public_key.encode_base64());
  if (!estimated_redemption_value) {
    BLOG(1, "Invalid estimated redemption value");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
    return;
  }

  BLOG(1, "Added 1 unblinded payment token with an estimated redemption value "
      "of " << *estimated_redemption_value << " BAT, you now have "
          << ConfirmationsState::Get()->get_unblinded_payment_tokens()->Count()
              << " unblinded payment tokens");

  NotifyConfirmAd(*estimated_redemption_value, confirmation);
}

void Confirmations::OnFailedToRedeemUnblindedToken(
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  BLOG(1, "Failed to redeem unblinded token with confirmation id "
      << confirmation.id << ", creative instance id "
          << confirmation.creative_instance_id << " and "
              << std::string(confirmation.type));

  if (should_retry) {
    if (!confirmation.created) {
      CreateNewConfirmationAndAppendToRetryQueue(confirmation);
    } else {
      AppendToRetryQueue(confirmation);
    }
  }

  NotifyConfirmAdFailed(confirmation);
}

void Confirmations::NotifyConfirmAd(
    const double estimated_redemption_value,
    const ConfirmationInfo& confirmation) {
  for (ConfirmationsObserver& observer : observers_) {
    observer.OnConfirmAd(estimated_redemption_value, confirmation);
  }
}

void Confirmations::NotifyConfirmAdFailed(
    const ConfirmationInfo& confirmation) {
  for (ConfirmationsObserver& observer : observers_) {
    observer.OnConfirmAdFailed(confirmation);
  }
}

}  // namespace ads
