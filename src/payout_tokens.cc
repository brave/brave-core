/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "payout_tokens.h"
#include "static_values.h"
#include "logging.h"
#include "redeem_payment_tokens_request.h"

#include "base/rand_util.h"

using namespace std::placeholders;

namespace confirmations {

PayoutTokens::PayoutTokens(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client,
    UnblindedTokens* unblinded_payment_tokens) :
    confirmations_(confirmations),
    confirmations_client_(confirmations_client),
    unblinded_payment_tokens_(unblinded_payment_tokens) {
  BLOG(INFO) << "Initializing payout tokens";
}

PayoutTokens::~PayoutTokens() {
  BLOG(INFO) << "Deinitializing payout tokens";
}

void PayoutTokens::Payout(const WalletInfo& wallet_info) {
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

  auto body = request.BuildBody(tokens, payload, wallet_info_);
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

  confirmations_client_->URLRequest(url, headers, body, content_type, method,
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
  } else {
    unblinded_payment_tokens_->RemoveAllTokens();

    BLOG(INFO) << "Successfully paid out tokens";
  }

  ScheduleNextPayout();
}

void PayoutTokens::ScheduleNextPayout() {
  auto start_timer_in = CalculateTimerForNextPayout();
  confirmations_->StartPayingOutRedeemedTokens(start_timer_in);
}

uint64_t PayoutTokens::CalculateTimerForNextPayout() {
  auto start_timer_in = kPayoutAfterSeconds;
  auto rand_delay = base::RandInt(0, start_timer_in / 10);
  start_timer_in += rand_delay;

  return start_timer_in;
}

}  // namespace confirmations
