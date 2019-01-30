/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "payout_tokens.h"
#include "static_values.h"
#include "logging.h"
#include "ads_serve_helper.h"

#include "base/rand_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"

using namespace std::placeholders;

namespace confirmations {

PayoutTokens::PayoutTokens(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client) :
    public_key_(nullptr),
    confirmations_(confirmations),
    confirmations_client_(confirmations_client) {
  BLOG(INFO) << "Initializing payout tokens";
}

PayoutTokens::~PayoutTokens() {
  BLOG(INFO) << "Deinitializing payout tokens";
}

void PayoutTokens::Payout(
    const WalletInfo& wallet_info,
    const std::string& public_key) {
  payment_id_ = wallet_info.payment_id;

  public_key_ = PublicKey::decode_base64(public_key);

  RedeemPaymentTokens();
}

///////////////////////////////////////////////////////////////////////////////

void PayoutTokens::RedeemPaymentTokens() {
  BLOG(INFO) << "RedeemPaymentTokens";

  auto unblinded_payment_tokens = confirmations_->GetUnblindedPaymentTokens();
  if (unblinded_payment_tokens.size() == 0) {
    BLOG(ERROR) << "No unblinded payment tokens";

    return;
  }

  // Create payload
  base::DictionaryValue payload;
  payload.SetKey("paymentId", base::Value(payment_id_));

  std::string payload_json;
  base::JSONWriter::Write(payload, &payload_json);

  // Create payment credentials
  auto payment_credentials_list = std::make_unique<base::ListValue>();
  for (auto& unblinded_payment_token : unblinded_payment_tokens) {
    // Create credential
    base::DictionaryValue credential_dictionary;

    auto verification_key = unblinded_payment_token.derive_verification_key();

    auto verification_signature = verification_key.sign(payload_json);
    auto verification_signature_base64 = verification_signature.encode_base64();
    credential_dictionary.SetKey("signature",
        base::Value(verification_signature_base64));

    auto preimage = unblinded_payment_token.preimage();
    auto preimage_base64 = preimage.encode_base64();
    credential_dictionary.SetKey("t", base::Value(preimage_base64));

    // Create payment credential
    auto* payment_credential_dictionary = new base::DictionaryValue();

    payment_credential_dictionary->SetKey("credential",
        base::DictionaryValue(std::move(credential_dictionary)));

    auto public_key_base64 = public_key_.encode_base64();
    payment_credential_dictionary->SetKey("publicKey",
        base::Value(public_key_base64));

    payment_credentials_list->Append(
        std::unique_ptr<base::DictionaryValue>(payment_credential_dictionary));
  }

  // Create request body
  base::DictionaryValue body_dictionary;

  body_dictionary.SetWithoutPathExpansion("paymentCredentials",
      std::move(payment_credentials_list));

  body_dictionary.SetKey("payload", base::Value(payload_json));

  std::string json;
  base::JSONWriter::Write(body_dictionary, &json);
  std::string body = json;

  // Create request headers
  std::vector<std::string> headers;

  auto accept_header = std::string("accept: ").append("application/json");
  headers.push_back(accept_header);

  // Create request content type
  std::string content_type = "application/json";

  // PUT /v1/confirmation/payment/{payment_id}
  BLOG(INFO) << "PUT /v1/confirmation/payment/{payment_id}";

  auto endpoint = std::string("/v1/confirmation/payment/").append(payment_id_);
  auto ads_serve_url = helper::AdsServe::GetURL().append(endpoint);

  BLOG(INFO) << "URL Request:";
  BLOG(INFO) << "  URL: " << ads_serve_url;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header;
  }
  BLOG(INFO) << "  Body: " << body;
  BLOG(INFO) << "  Content_type: " << content_type;

  auto callback = std::bind(&PayoutTokens::OnRedeemPaymentTokens,
      this, ads_serve_url, _1, _2, _3);

  confirmations_client_->URLRequest(ads_serve_url, headers, body, content_type,
      URLRequestMethod::PUT, callback);
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
    BLOG(ERROR) << "Failed to redeem payout tokens";
    OnPayout(FAILED);
    return;
  }

  OnPayout(SUCCESS);
}

void PayoutTokens::OnPayout(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to payout tokens";
  } else {
    RemoveAllUnblindedPaymentTokens();
    BLOG(INFO) << "Successfully paid out tokens";
  }

  auto start_timer_in = kPayoutTokensAfterSeconds;
  auto rand_delay = base::RandInt(0, start_timer_in / 10);
  start_timer_in += rand_delay;

  confirmations_->StartPayingOutConfirmations(start_timer_in);
}

void PayoutTokens::RemoveAllUnblindedPaymentTokens() {
  BLOG(INFO) << "Removing all unblinded payment tokens";

  confirmations_->SetUnblindedPaymentTokens({});

  BLOG(INFO) << "Removed all unblinded payment tokens";
}

}  // namespace confirmations
