/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "refill_tokens.h"
#include "static_values.h"
#include "logging.h"
#include "ads_serve_helper.h"
#include "string_helper.h"
#include "security_helper.h"

#include "base/rand_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"

using namespace std::placeholders;

namespace confirmations {

RefillTokens::RefillTokens(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client) :
    public_key_(nullptr),
    confirmations_(confirmations),
    confirmations_client_(confirmations_client) {
  BLOG(INFO) << "Initializing refill tokens";
}

RefillTokens::~RefillTokens() {
  BLOG(INFO) << "Deinitializing refill tokens";
}

void RefillTokens::Refill(
    const WalletInfo& wallet_info,
    const std::string& public_key) {
  BLOG(INFO) << "Refill";

  payment_id_ = wallet_info.payment_id;
  secret_key_ = helper::String::decode_hex(wallet_info.signing_key);

  public_key_ = PublicKey::decode_base64(public_key);

  RequestSignedTokens();
}

void RefillTokens::RetryGettingSignedTokens() {
  BLOG(INFO) << "Retry getting signed tokens";

  GetSignedTokens();
}

///////////////////////////////////////////////////////////////////////////////

void RefillTokens::RequestSignedTokens() {
  BLOG(INFO) << "RequestSignedTokens";

  auto unblinded_tokens = confirmations_->GetUnblindedTokens();
  auto unblinded_tokens_count = unblinded_tokens.size();

  if (unblinded_tokens_count >= kMinimumUnblindedTokens) {
    BLOG(INFO) << "No need to refill tokens as we already have "
        << unblinded_tokens_count << " unblinded tokens which is above the"
        << " minimum threshold of " << kMinimumUnblindedTokens;
    return;
  }

  // Generate tokens
  auto refill_amount = kMaximumUnblindedTokens - unblinded_tokens_count;
  tokens_ = helper::Security::GenerateTokens(refill_amount);
  BLOG(INFO) << "Generated " << tokens_.size() << " tokens";

  // Blind tokens
  blinded_tokens_ = helper::Security::BlindTokens(tokens_);
  BLOG(INFO) << "Blinded " << blinded_tokens_.size() << " tokens";

  // Create request body
  std::string body = "";

  body.append("{\"blindedTokens\":");
  body.append("[");

  for (size_t i = 0; i < blinded_tokens_.size(); i++) {
    if (i > 0) {
      body.append(",");
    }

    body.append("\"");
    auto blinded_token = blinded_tokens_.at(i).encode_base64();
    body.append(blinded_token);
    body.append("\"");
  }

  body.append("]");
  body.append("}");

  // Create request headers
  std::vector<std::string> headers;

  auto body_sha256 = helper::Security::GetSHA256(body);
  auto body_sha256_base64 = helper::Security::GetBase64(body_sha256);
  auto digest_header_value = std::string("SHA-256=").append(body_sha256_base64);
  auto digest_header = std::string("digest: ").append(digest_header_value);
  headers.push_back(digest_header);

  auto signature_header_value = helper::Security::Sign(
      {"digest"}, {digest_header_value}, 1, {"primary"}, secret_key_);
  auto signature_header = std::string("signature: ").append(
      signature_header_value);
  headers.push_back(signature_header);

  auto accept_header = std::string("accept: ").append("application/json");
  headers.push_back(accept_header);

  // Create request content type
  std::string content_type = "application/json";

  // POST /v1/confirmation/token/{payment_id}
  BLOG(INFO) << "POST /v1/confirmation/token/{payment_id}";

  auto endpoint = std::string("/v1/confirmation/token/").append(payment_id_);
  auto ads_serve_url = helper::AdsServe::GetURL().append(endpoint);

  BLOG(INFO) << "URL Request:";
  BLOG(INFO) << "  URL: " << ads_serve_url;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header;
  }
  BLOG(INFO) << "  Body: " << body;
  BLOG(INFO) << "  Content_type: " << content_type;

  auto callback = std::bind(&RefillTokens::OnRequestSignedTokens,
      this, ads_serve_url, _1, _2, _3);

  confirmations_client_->URLRequest(ads_serve_url, headers, body, content_type,
      URLRequestMethod::POST, callback);
}

void RefillTokens::OnRequestSignedTokens(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  BLOG(INFO) << "OnRequestSignedTokens";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 201) {
    BLOG(ERROR) << "Failed to receive blinded tokens";
    OnRefill(FAILED);
    return;
  }

  // Parse JSON response
  std::unique_ptr<base::Value> value(base::JSONReader::Read(response));
  base::DictionaryValue* dictionary;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(ERROR) << "Invalid response";
    OnRefill(FAILED);
    return;
  }

  base::Value *v;

  // Get nonce
  if (!(v = dictionary->FindKey("nonce"))) {
    BLOG(ERROR) << "Response missing nonce";
    OnRefill(FAILED);
    return;
  }

  auto nonce = v->GetString();

  // GET /v1/confirmation/token/{payment_id}?nonce={nonce}
  auto endpoint = std::string("/v1/confirmation/token/").append(payment_id_)
      .append("?nonce=").append(nonce);
  last_fetch_tokens_ads_serve_url_ =
      helper::AdsServe::GetURL().append(endpoint);

  GetSignedTokens();
}

void RefillTokens::GetSignedTokens() {
  BLOG(INFO) << "GetSignedTokens";

  BLOG(INFO) << "GET /v1/confirmation/token/{payment_id}?nonce={nonce}";

  BLOG(INFO) << "URL Request:";
  BLOG(INFO) << "  URL: " << last_fetch_tokens_ads_serve_url_;

  auto callback = std::bind(&RefillTokens::OnGetSignedTokens, this,
      last_fetch_tokens_ads_serve_url_, _1, _2, _3);

  confirmations_client_->URLRequest(last_fetch_tokens_ads_serve_url_, {}, "",
      "", URLRequestMethod::GET, callback);
}

void RefillTokens::OnGetSignedTokens(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  BLOG(INFO) << "OnGetSignedTokens";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 200) {
    BLOG(ERROR) << "Failed to get signed tokens";
    confirmations_->StartRetryGettingSignedTokens(
        kRetryGettingSignedTokensAfterSeconds);
    return;
  }

  // Parse JSON response
  std::unique_ptr<base::Value> value(base::JSONReader::Read(response));
  base::DictionaryValue* dictionary;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(ERROR) << "Invalid response";
    OnRefill(FAILED);
    return;
  }

  base::Value *v;

  // Get public key
  if (!(v = dictionary->FindKey("publicKey"))) {
    BLOG(ERROR) << "Response missing publicKey";
    OnRefill(FAILED);
    return;
  }

  auto public_key_base64 = v->GetString();

  // Validate public key
  auto catalog_issuers_public_key_base64 = public_key_.encode_base64();
  if (public_key_base64 != catalog_issuers_public_key_base64) {
    BLOG(ERROR) << "Response public_key: " << public_key_base64
        << " does not match catalog issuers public key: "
        << catalog_issuers_public_key_base64;
    OnRefill(FAILED);
    return;
  }

  // Get batch proof
  if (!(v = dictionary->FindKey("batchProof"))) {
    BLOG(ERROR) << "Response missing batchProof";
    OnRefill(FAILED);
    return;
  }

  auto batch_dleq_proof_base64 = v->GetString();

  // Get signed tokens
  if (!(v = dictionary->FindKey("signedTokens"))) {
    BLOG(ERROR) << "Response missing signedTokens";
    OnRefill(FAILED);
    return;
  }

  base::ListValue list(v->GetList());
  std::vector<SignedToken> signed_tokens;
  for (size_t i = 0; i < list.GetSize(); i++) {
    base::Value *signed_token_value;
    list.Get(i, &signed_token_value);

    auto signed_token_base64 = signed_token_value->GetString();
    auto signed_token = SignedToken::decode_base64(signed_token_base64);
    signed_tokens.push_back(signed_token);
  }

  auto batch_dleq_proof =
      BatchDLEQProof::decode_base64(batch_dleq_proof_base64);
  auto unblinded_tokens = batch_dleq_proof.verify_and_unblind(tokens_,
      blinded_tokens_, signed_tokens, public_key_);

  if (unblinded_tokens.size() == 0) {
    BLOG(ERROR) << "Failed to unblinded tokens";

    BLOG(ERROR) << "  Batch proof: " << batch_dleq_proof_base64;

    BLOG(ERROR) << "  Tokens:";
    for (auto& token : tokens_) {
      auto token_base64 = token.encode_base64();
      BLOG(ERROR) << "    " << token_base64;
    }

    BLOG(ERROR) << "  Blinded tokens:";
    for (auto& blinded_token : blinded_tokens_) {
      auto blinded_token_base64 = blinded_token.encode_base64();
      BLOG(ERROR) << "    " << blinded_token_base64;
    }

    BLOG(ERROR) << "  Signed tokens:";
    for (auto& signed_token : signed_tokens) {
      auto signed_token_base64 = signed_token.encode_base64();
      BLOG(ERROR) << "    " << signed_token_base64;
    }

    auto public_key_base64 = public_key_.encode_base64();
    BLOG(ERROR) << "  Public key: " << public_key_base64;

    OnRefill(FAILED);
    return;
  }

  BLOG(INFO) << "Unblinded " << unblinded_tokens.size() << " tokens";

  AppendUnblindedTokens(unblinded_tokens);

  OnRefill(SUCCESS);
}

void RefillTokens::OnRefill(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to refill tokens";
  } else {
    confirmations_->SaveState();
    BLOG(INFO) << "Successfully refilled tokens";
  }

  blinded_tokens_.clear();
  tokens_.clear();

  auto start_timer_in = kRefillTokensAfterSeconds;
  auto rand_delay = base::RandInt(0, start_timer_in / 10);
  start_timer_in += rand_delay;

  confirmations_->StartRefillingConfirmations(start_timer_in);
}

void RefillTokens::AppendUnblindedTokens(
    const std::vector<UnblindedToken>& tokens) {
  auto unblinded_tokens = confirmations_->GetUnblindedTokens();

  unblinded_tokens.insert(unblinded_tokens.end(),
      tokens.begin(), tokens.end());

  BLOG(INFO) << "Added " << tokens.size()
      << " unblinded tokens, you now have " << unblinded_tokens.size()
      << " unblinded tokens";

  confirmations_->SetUnblindedTokens(unblinded_tokens);
}

}  // namespace confirmations
