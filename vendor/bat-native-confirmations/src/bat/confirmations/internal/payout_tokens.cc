/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/payout_tokens.h"
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/unblinded_tokens.h"
#include "bat/confirmations/internal/redeem_payment_tokens_request.h"

#include "base/rand_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace confirmations {

PayoutTokens::PayoutTokens(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client,
    UnblindedTokens* unblinded_payment_tokens) :
    next_retry_start_timer_in_(0),
    confirmations_(confirmations),
    confirmations_client_(confirmations_client),
    unblinded_payment_tokens_(unblinded_payment_tokens) {
  BLOG(INFO) << "Initializing payout tokens";
}

PayoutTokens::~PayoutTokens() {
  BLOG(INFO) << "Deinitializing payout tokens";
}

void PayoutTokens::Payout(const WalletInfo& wallet_info) {
  DCHECK(!wallet_info.payment_id.empty());
  DCHECK(!wallet_info.public_key.empty());

  BLOG(INFO) << "Payout";

  wallet_info_ = WalletInfo(wallet_info);

  RedeemPaymentTokens();
}

///////////////////////////////////////////////////////////////////////////////

void PayoutTokens::RedeemPaymentTokens() {
  BLOG(INFO) << "RedeemPaymentTokens";

  if (unblinded_payment_tokens_->IsEmpty()) {
    BLOG(INFO) << "No unblinded payment tokens to redeem";
    ScheduleNextPayout();
    return;
  }

  BLOG(INFO) << "PUT /v1/confirmation/payment/{payment_id}";
  RedeemPaymentTokensRequest request;

  auto tokens = unblinded_payment_tokens_->GetAllTokens();

  auto payload = request.CreatePayload(wallet_info_);

  BLOG(INFO) << "URL Request:";

  auto url = request.BuildUrl(wallet_info_);
  BLOG(INFO) << "  URL: " << url;

  auto method = request.GetMethod();

  auto body = request.BuildBody(tokens, payload);
  BLOG(INFO) << "  Body: " << body;

  auto headers = request.BuildHeaders();
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header;
  }

  auto content_type = request.GetContentType();
  BLOG(INFO) << "  Content_type: " << content_type;

  auto callback = std::bind(&PayoutTokens::OnRedeemPaymentTokens,
      this, url, _1, _2, _3);

  confirmations_client_->LoadURL(url, headers, body, content_type, method,
      callback);
}

void PayoutTokens::OnRedeemPaymentTokens(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  BLOG(INFO) << "OnRedeemPaymentTokens";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 200) {
    BLOG(ERROR) << "Failed to redeem payment tokens";
    OnPayout(FAILED);
    return;
  }

  OnPayout(SUCCESS);
}

void PayoutTokens::OnPayout(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to payout tokens";

    RetryNextPayout();
  } else {
    unblinded_payment_tokens_->RemoveAllTokens();

    BLOG(INFO) << "Successfully paid out tokens";

    ScheduleNextPayout();
  }
}

void PayoutTokens::ScheduleNextPayout() const {
  confirmations_->UpdateNextTokenRedemptionDate();
  confirmations_->SaveState();

  auto start_timer_in = confirmations_->CalculateTokenRedemptionTimeInSeconds();
  confirmations_->StartPayingOutRedeemedTokens(start_timer_in);
}

void PayoutTokens::RetryNextPayout() {
  BLOG(INFO) << "Retry next payout";

  if (next_retry_start_timer_in_ == 0) {
    next_retry_start_timer_in_ = 2 * base::Time::kSecondsPerMinute;
  } else {
    next_retry_start_timer_in_ *= 2;
  }

  auto rand_delay = base::RandInt(0, next_retry_start_timer_in_ / 10);
  next_retry_start_timer_in_ += rand_delay;

  confirmations_->StartPayingOutRedeemedTokens(next_retry_start_timer_in_);
}

}  // namespace confirmations
