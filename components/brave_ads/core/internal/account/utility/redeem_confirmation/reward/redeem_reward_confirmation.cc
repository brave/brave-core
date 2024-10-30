/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_feature.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/fetch_payment_token_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/utility/tokens_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {
constexpr char kPaymentTokenKey[] = "paymentToken";
}  // namespace

RedeemRewardConfirmation::RedeemRewardConfirmation(
    RedeemRewardConfirmation&&) noexcept = default;

RedeemRewardConfirmation& RedeemRewardConfirmation::operator=(
    RedeemRewardConfirmation&&) noexcept = default;

RedeemRewardConfirmation::~RedeemRewardConfirmation() = default;

// static
void RedeemRewardConfirmation::CreateAndRedeem(
    base::WeakPtr<RedeemConfirmationDelegate> delegate,
    const ConfirmationInfo& confirmation) {
  RedeemRewardConfirmation redeem_confirmation(std::move(delegate));
  RedeemRewardConfirmation::Redeem(std::move(redeem_confirmation),
                                   confirmation);
}

///////////////////////////////////////////////////////////////////////////////

RedeemRewardConfirmation::RedeemRewardConfirmation(
    base::WeakPtr<RedeemConfirmationDelegate> delegate) {
  CHECK(delegate);

  delegate_ = std::move(delegate);
}

// static
void RedeemRewardConfirmation::Redeem(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));
  CHECK(confirmation.reward);

  BLOG(1, "Redeem reward confirmation");

  if (!HasIssuers()) {
    BLOG(1, "Failed to redeem confirmation token due to missing issuers");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/true);
  }

  CreateConfirmation(std::move(redeem_confirmation), confirmation);
}

// static
void RedeemRewardConfirmation::CreateConfirmation(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Create reward confirmation");

  CreateRewardConfirmationUrlRequestBuilder url_request_builder(confirmation);
  mojom::UrlRequestInfoPtr mojom_url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(mojom_url_request));
  BLOG(7, UrlRequestHeadersToString(mojom_url_request));

  GetAdsClient()->UrlRequest(
      std::move(mojom_url_request),
      base::BindOnce(&RedeemRewardConfirmation::CreateConfirmationCallback,
                     std::move(redeem_confirmation), confirmation));
}

// static
void RedeemRewardConfirmation::CreateConfirmationCallback(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation,
    const mojom::UrlResponseInfo& mojom_url_response) {
  BLOG(6, UrlResponseToString(mojom_url_response));
  BLOG(7, UrlResponseHeadersToString(mojom_url_response));

  FetchPaymentTokenAfter(kFetchPaymentTokenAfter.Get(),
                         std::move(redeem_confirmation), confirmation);
}

// static
void RedeemRewardConfirmation::FetchPaymentTokenAfter(
    const base::TimeDelta delay,
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Fetch payment token in " << delay);

  GlobalState::GetInstance()->PostDelayedTask(
      base::BindOnce(&RedeemRewardConfirmation::FetchPaymentToken,
                     std::move(redeem_confirmation), confirmation),
      delay);
}

// static
void RedeemRewardConfirmation::FetchPaymentToken(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Fetch payment token");

  FetchPaymentTokenUrlRequestBuilder url_request_builder(confirmation);
  mojom::UrlRequestInfoPtr mojom_url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(mojom_url_request));
  BLOG(7, UrlRequestHeadersToString(mojom_url_request));

  GetAdsClient()->UrlRequest(
      std::move(mojom_url_request),
      base::BindOnce(&RedeemRewardConfirmation::FetchPaymentTokenCallback,
                     std::move(redeem_confirmation), confirmation));
}

// static
void RedeemRewardConfirmation::FetchPaymentTokenCallback(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation,
    const mojom::UrlResponseInfo& mojom_url_response) {
  BLOG(6, UrlResponseToString(mojom_url_response));
  BLOG(7, UrlResponseHeadersToString(mojom_url_response));

  const auto handle_url_response_result =
      HandleFetchPaymentTokenUrlResponse(confirmation, mojom_url_response);
  if (!handle_url_response_result.has_value()) {
    const auto& [failure, should_retry] = handle_url_response_result.error();

    BLOG(0, failure);

    return redeem_confirmation.FailedToRedeemConfirmation(confirmation,
                                                          should_retry);
  }
  const PaymentTokenInfo& payment_token = handle_url_response_result.value();

  const auto add_payment_token_result = MaybeAddPaymentToken(payment_token);
  if (!add_payment_token_result.has_value()) {
    BLOG(1, add_payment_token_result.error());
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  redeem_confirmation.SuccessfullyRedeemedConfirmation(confirmation);
}

// static
base::expected<PaymentTokenInfo, std::tuple<std::string, bool>>
RedeemRewardConfirmation::HandleFetchPaymentTokenUrlResponse(
    const ConfirmationInfo& confirmation,
    const mojom::UrlResponseInfo& mojom_url_response) {
  if (mojom_url_response.status_code == net::HTTP_NOT_FOUND) {
    return base::unexpected(
        std::make_tuple("Confirmation not found", /*should_retry=*/true));
  }

  if (mojom_url_response.status_code == net::HTTP_BAD_REQUEST) {
    return base::unexpected(
        std::make_tuple("Credential is invalid", /*should_retry=*/false));
  }

  if (mojom_url_response.status_code == net::HTTP_ACCEPTED) {
    return base::unexpected(
        std::make_tuple("Payment token is not ready", /*should_retry=*/true));
  }

  if (mojom_url_response.status_code != net::HTTP_OK) {
    return base::unexpected(std::make_tuple("Failed to fetch payment token",
                                            /*should_retry=*/true));
  }

  const std::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(mojom_url_response.body);
  if (!dict) {
    return base::unexpected(std::make_tuple(
        base::StrCat({"Failed to parse response: ", mojom_url_response.body}),
        /*should_retry=*/true));
  }

  const std::string* const id = dict->FindString("id");
  if (!id) {
    return base::unexpected(std::make_tuple("Response is missing id",
                                            /*should_retry=*/false));
  }

  if (*id != confirmation.transaction_id) {
    return base::unexpected(std::make_tuple(
        base::ReplaceStringPlaceholders(
            "Response id $1 does not match confirmation transaction id $2",
            {*id, confirmation.transaction_id}, nullptr),
        /*should_retry=*/false));
  }

  const auto* const payment_token_dict = dict->FindDict(kPaymentTokenKey);
  if (!payment_token_dict) {
    return base::unexpected(std::make_tuple("Response is missing paymentToken",
                                            /*should_retry=*/false));
  }

  const std::optional<cbr::PublicKey> public_key =
      ParsePublicKey(*payment_token_dict);
  if (!public_key.has_value()) {
    return base::unexpected(std::make_tuple("Failed to parse public key",
                                            /*should_retry=*/false));
  }

  if (!TokenIssuerPublicKeyExistsForType(TokenIssuerType::kPayments,
                                         *public_key)) {
    return base::unexpected(
        std::make_tuple("Payments public key does not exist",
                        /*should_retry=*/true));
  }

  const std::optional<cbr::UnblindedTokenList> unblinded_tokens =
      ParseVerifyAndUnblindTokens(
          *payment_token_dict, {confirmation.reward->token},
          {confirmation.reward->blinded_token}, *public_key);
  if (!unblinded_tokens) {
    return base::unexpected(
        std::make_tuple("Failed to parse, verify and unblind payment tokens",
                        /*should_retry=*/false));
  }

  PaymentTokenInfo payment_token;
  payment_token.transaction_id = confirmation.transaction_id;
  payment_token.unblinded_token = unblinded_tokens->front();
  payment_token.public_key = *public_key;
  payment_token.confirmation_type = confirmation.type;
  payment_token.ad_type = confirmation.ad_type;
  return payment_token;
}

void RedeemRewardConfirmation::SuccessfullyRedeemedConfirmation(
    const ConfirmationInfo& confirmation) {
  LogPaymentTokenStatus();

  NotifyDidRedeemConfirmation(confirmation);
}

void RedeemRewardConfirmation::FailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  NotifyFailedToRedeemConfirmation(confirmation, should_retry);
}

void RedeemRewardConfirmation::NotifyDidRedeemConfirmation(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnDidRedeemConfirmation(confirmation);
  }
}

void RedeemRewardConfirmation::NotifyFailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation,
    const bool should_retry) const {
  if (delegate_) {
    delegate_->OnFailedToRedeemConfirmation(confirmation, should_retry);
  }
}

}  // namespace brave_ads
