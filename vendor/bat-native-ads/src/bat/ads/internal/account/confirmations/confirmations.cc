/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmations.h"

#include <cstdint>
#include <functional>
#include <vector>

#include "base/guid.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards_util.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/privacy_util.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/redeem_unblinded_token.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_dto_user_data_builder.h"

namespace ads {

namespace {
const int64_t kRetryAfterSeconds = 5 * base::Time::kSecondsPerMinute;
}  // namespace

Confirmations::Confirmations(privacy::TokenGeneratorInterface* token_generator,
                             AdRewards* ad_rewards)
    : token_generator_(token_generator),
      confirmations_state_(std::make_unique<ConfirmationsState>(ad_rewards)),
      redeem_unblinded_token_(std::make_unique<RedeemUnblindedToken>()) {
  DCHECK(token_generator_);

  redeem_unblinded_token_->set_delegate(this);
}

Confirmations::~Confirmations() {
  redeem_unblinded_token_->set_delegate(nullptr);
}

void Confirmations::AddObserver(ConfirmationsObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Confirmations::RemoveObserver(ConfirmationsObserver* observer) {
  DCHECK(observer);
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

void Confirmations::ConfirmAd(const std::string& creative_instance_id,
                              const ConfirmationType& confirmation_type) {
  BLOG(1, "Confirming " << std::string(confirmation_type)
                        << " ad for creative instance id "
                        << creative_instance_id);

  dto::user_data::Build(
      creative_instance_id, confirmation_type,
      [=](const base::Value& user_data) {
        const base::DictionaryValue* user_data_dictionary = nullptr;
        user_data.GetAsDictionary(&user_data_dictionary);
        const ConfirmationInfo confirmation = CreateConfirmation(
            creative_instance_id, confirmation_type, *user_data_dictionary);
        redeem_unblinded_token_->Redeem(confirmation);
      });
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

ConfirmationInfo Confirmations::CreateConfirmation(
    const std::string& creative_instance_id,
    const ConfirmationType& confirmation_type,
    const base::DictionaryValue& user_data) const {
  DCHECK(!creative_instance_id.empty());
  DCHECK(confirmation_type != ConfirmationType::kUndefined);
  ConfirmationInfo confirmation;

  confirmation.id = base::GenerateGUID();
  confirmation.creative_instance_id = creative_instance_id;
  confirmation.type = confirmation_type;

  if (ShouldRewardUser() &&
      !ConfirmationsState::Get()->get_unblinded_tokens()->IsEmpty()) {
    const privacy::UnblindedTokenInfo unblinded_token =
        ConfirmationsState::Get()->get_unblinded_tokens()->GetToken();
    confirmation.unblinded_token = unblinded_token;

    const std::vector<Token> tokens = token_generator_->Generate(1);
    confirmation.payment_token = tokens.front();

    const std::vector<BlindedToken> blinded_tokens =
        privacy::BlindTokens(tokens);
    const BlindedToken blinded_token = blinded_tokens.front();
    confirmation.blinded_payment_token = blinded_token;

    std::string json;
    base::JSONWriter::Write(user_data, &json);
    confirmation.user_data = json;

    confirmation.timestamp =
        static_cast<int64_t>(base::Time::Now().ToDoubleT());

    const std::string payload = CreateConfirmationRequestDTO(confirmation);
    confirmation.credential = CreateCredential(unblinded_token, payload);

    ConfirmationsState::Get()->get_unblinded_tokens()->RemoveToken(
        unblinded_token);
    ConfirmationsState::Get()->Save();
  }

  return confirmation;
}

void Confirmations::CreateNewConfirmationAndAppendToRetryQueue(
    const ConfirmationInfo& confirmation) {
  if (ConfirmationsState::Get()->get_unblinded_tokens()->IsEmpty()) {
    AppendToRetryQueue(confirmation);
    return;
  }

  dto::user_data::Build(
      confirmation.creative_instance_id, confirmation.type,
      [=](const base::Value& user_data) {
        const base::DictionaryValue* user_data_dictionary = nullptr;
        user_data.GetAsDictionary(&user_data_dictionary);

        const ConfirmationInfo new_confirmation =
            CreateConfirmation(confirmation.creative_instance_id,
                               confirmation.type, *user_data_dictionary);
        AppendToRetryQueue(new_confirmation);
      });
}

void Confirmations::AppendToRetryQueue(const ConfirmationInfo& confirmation) {
  ConfirmationsState::Get()->append_failed_confirmation(confirmation);
  ConfirmationsState::Get()->Save();

  BLOG(1, "Added confirmation id " << confirmation.id
                                   << ", creative instance "
                                      "id "
                                   << confirmation.creative_instance_id
                                   << " and " << std::string(confirmation.type)
                                   << " to the confirmations queue");
}

void Confirmations::RemoveFromRetryQueue(const ConfirmationInfo& confirmation) {
  if (!ConfirmationsState::Get()->remove_failed_confirmation(confirmation)) {
    BLOG(0, "Failed to remove confirmation id "
                << confirmation.id << ", creative instance id "
                << confirmation.creative_instance_id << " and "
                << std::string(confirmation.type)
                << " from the confirmations queue");

    return;
  }

  BLOG(1, "Removed confirmation id " << confirmation.id
                                     << ", creative "
                                        "instance id "
                                     << confirmation.creative_instance_id
                                     << " and "
                                     << std::string(confirmation.type)
                                     << " from the confirmations queue");

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

void Confirmations::OnDidSendConfirmation(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Successfully sent confirmation with id "
              << confirmation.id << ", creative instance id "
              << confirmation.creative_instance_id << " and "
              << std::string(confirmation.type));
}

void Confirmations::OnDidRedeemUnblindedToken(
    const ConfirmationInfo& confirmation,
    const privacy::UnblindedTokenInfo& unblinded_payment_token) {
  if (ConfirmationsState::Get()->get_unblinded_payment_tokens()->TokenExists(
          unblinded_payment_token)) {
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

  const CatalogIssuersInfo catalog_issuers =
      ConfirmationsState::Get()->get_catalog_issuers();

  const base::Optional<double> estimated_redemption_value =
      catalog_issuers.GetEstimatedRedemptionValue(
          unblinded_payment_token.public_key.encode_base64());
  if (!estimated_redemption_value) {
    BLOG(1, "Invalid estimated redemption value");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
    return;
  }

  BLOG(1,
       "Added 1 unblinded payment token with an estimated redemption value "
       "of "
           << *estimated_redemption_value << " BAT, you now have "
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
    const ConfirmationInfo& confirmation) const {
  for (ConfirmationsObserver& observer : observers_) {
    observer.OnConfirmAd(estimated_redemption_value, confirmation);
  }
}

void Confirmations::NotifyConfirmAdFailed(
    const ConfirmationInfo& confirmation) const {
  for (ConfirmationsObserver& observer : observers_) {
    observer.OnConfirmAdFailed(confirmation);
  }
}

}  // namespace ads
