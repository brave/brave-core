/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/redeem_non_reward_confirmation.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

RedeemNonRewardConfirmation::RedeemNonRewardConfirmation(
    RedeemNonRewardConfirmation&&) noexcept = default;

RedeemNonRewardConfirmation& RedeemNonRewardConfirmation::operator=(
    RedeemNonRewardConfirmation&&) noexcept = default;

RedeemNonRewardConfirmation::~RedeemNonRewardConfirmation() = default;

// static
void RedeemNonRewardConfirmation::CreateAndRedeem(
    base::WeakPtr<RedeemConfirmationDelegate> delegate,
    const ConfirmationInfo& confirmation) {
  RedeemNonRewardConfirmation redeem_confirmation(std::move(delegate));
  RedeemNonRewardConfirmation::Redeem(std::move(redeem_confirmation),
                                      confirmation);
}

///////////////////////////////////////////////////////////////////////////////

RedeemNonRewardConfirmation::RedeemNonRewardConfirmation(
    base::WeakPtr<RedeemConfirmationDelegate> delegate) {
  CHECK(delegate);

  delegate_ = std::move(delegate);
}

// static
void RedeemNonRewardConfirmation::Redeem(
    RedeemNonRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));
  CHECK(!confirmation.reward);

  BLOG(1, "Redeem non-reward confirmation");

  CreateConfirmation(std::move(redeem_confirmation), confirmation);
}

// static
void RedeemNonRewardConfirmation::CreateConfirmation(
    RedeemNonRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Create non-reward confirmation");

  CreateNonRewardConfirmationUrlRequestBuilder url_request_builder(
      confirmation);
  mojom::UrlRequestInfoPtr mojom_url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(mojom_url_request));
  BLOG(7, UrlRequestHeadersToString(mojom_url_request));

  GetAdsClient().UrlRequest(
      std::move(mojom_url_request),
      base::BindOnce(&RedeemNonRewardConfirmation::CreateConfirmationCallback,
                     std::move(redeem_confirmation), confirmation));
}

// static
void RedeemNonRewardConfirmation::CreateConfirmationCallback(
    RedeemNonRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation,
    const mojom::UrlResponseInfo& mojom_url_response) {
  BLOG(6, UrlResponseToString(mojom_url_response));
  BLOG(7, UrlResponseHeadersToString(mojom_url_response));

  if (mojom_url_response.status_code != net::kHttpImATeapot) {
    const bool should_retry =
        mojom_url_response.status_code != net::HTTP_CONFLICT &&
        mojom_url_response.status_code != net::HTTP_BAD_REQUEST &&
        mojom_url_response.status_code != net::HTTP_CREATED;
    return redeem_confirmation.FailedToRedeemConfirmation(confirmation,
                                                          should_retry);
  }

  redeem_confirmation.SuccessfullyRedeemedConfirmation(confirmation);
}

void RedeemNonRewardConfirmation::SuccessfullyRedeemedConfirmation(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Successfully redeemed non-reward "
              << confirmation.type << " confirmation for "
              << confirmation.ad_type << " with transaction id "
              << confirmation.transaction_id << " and creative instance id "
              << confirmation.creative_instance_id);

  NotifyDidRedeemConfirmation(confirmation);
}

void RedeemNonRewardConfirmation::FailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation,
    bool should_retry) {
  BLOG(1, "Failed to redeem non-reward "
              << confirmation.type << " confirmation for "
              << confirmation.ad_type << " with transaction id "
              << confirmation.transaction_id << " and creative instance id "
              << confirmation.creative_instance_id);

  NotifyFailedToRedeemConfirmation(confirmation, should_retry);
}

void RedeemNonRewardConfirmation::NotifyDidRedeemConfirmation(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnDidRedeemConfirmation(confirmation);
  }
}

void RedeemNonRewardConfirmation::NotifyFailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation,
    bool should_retry) const {
  if (delegate_) {
    delegate_->OnFailedToRedeemConfirmation(confirmation, should_retry);
  }
}

}  // namespace brave_ads
