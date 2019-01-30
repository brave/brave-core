/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "redeem_token.h"
#include "static_values.h"
#include "logging.h"
#include "ads_serve_helper.h"
#include "security_helper.h"

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"

#include "net/base/escape.h"

using namespace std::placeholders;

namespace confirmations {

RedeemToken::RedeemToken(
    ConfirmationsImpl* confirmations,
    ConfirmationsClient* confirmations_client) :
    public_key_(nullptr),
    confirmations_(confirmations),
    confirmations_client_(confirmations_client) {
  BLOG(INFO) << "Initializing redeem token";
}

RedeemToken::~RedeemToken() {
  BLOG(INFO) << "Deinitializing redeem token";
}

void RedeemToken::Redeem(
    const std::string& creative_instance_id,
    const std::string& public_key) {
  BLOG(INFO) << "Redeem";

  auto unblinded_tokens = confirmations_->GetUnblindedTokens();
  if (unblinded_tokens.size() == 0) {
    BLOG(ERROR) << "Failed to redeem token as there are no unblinded tokens";
    OnRedeem(FAILED);
    return;
  }

  public_key_ = PublicKey::decode_base64(public_key);

  auto unblinded_token = unblinded_tokens.front();

  auto unblinded_token_base64 = unblinded_token.encode_base64();
  if (unblinded_token_base64.empty()) {
    BLOG(ERROR) << "Invalid unblinded token";
    OnRedeem(FAILED);
    return;
  }

  CreateConfirmation(creative_instance_id, unblinded_token_base64);
}

///////////////////////////////////////////////////////////////////////////////

void RedeemToken::CreateConfirmation(
    const std::string& creative_instance_id,
    const std::string& unblinded_token_base64) {
  BLOG(INFO) << "CreateConfirmation";

  // Create blinded token
  auto payment_tokens = helper::Security::GenerateTokens(1);
  auto payment_token = payment_tokens.front();
  auto payment_token_base64 = payment_token.encode_base64();

  auto blinded_payment_tokens = helper::Security::BlindTokens(payment_tokens);
  auto blinded_payment_token = blinded_payment_tokens.front();
  auto blinded_payment_token_base64 = blinded_payment_token.encode_base64();

  // Generate confirmation id
  auto confirmation_id = base::GenerateGUID();

  // Create payload
  base::DictionaryValue payload_dictionary;
  payload_dictionary.SetKey("creativeInstanceId",
      base::Value(creative_instance_id));
  payload_dictionary.SetKey("payload",
      base::Value(base::Value::Type::DICTIONARY));
  payload_dictionary.SetKey("blindedPaymentToken",
      base::Value(blinded_payment_token_base64));
  payload_dictionary.SetKey("type", base::Value("landed"));

  std::string payload_json;
  base::JSONWriter::Write(payload_dictionary, &payload_json);

  // Create credential
  base::DictionaryValue credential_dictionary;

  credential_dictionary.SetKey("payload", base::Value(payload_json));

  auto unblinded_token = UnblindedToken::decode_base64(unblinded_token_base64);
  auto unblinded_token_verification_key =
      unblinded_token.derive_verification_key();
  auto unblinded_token_verification_signature =
      unblinded_token_verification_key.sign(payload_json);
  auto unblinded_token_verification_signature_base64 =
      unblinded_token_verification_signature.encode_base64();
  credential_dictionary.SetKey("signature",
      base::Value(unblinded_token_verification_signature_base64));

  auto unblinded_token_preimage = unblinded_token.preimage();
  auto unblinded_token_preimage_base64 =
      unblinded_token_preimage.encode_base64();
  credential_dictionary.SetKey("t",
      base::Value(unblinded_token_preimage_base64));

  std::string credential_json;
  base::JSONWriter::Write(credential_dictionary, &credential_json);

  std::vector<uint8_t> credential(credential_json.begin(),
      credential_json.end());
  std::string credential_base64 = helper::Security::GetBase64(credential);

  std::string uri_encoded_credential =
      net::EscapeQueryParamValue(credential_base64, true);

  // Create request body
  std::string body = payload_json;

  // Create request headers
  std::vector<std::string> headers;

  auto accept_header = std::string("accept: ").append("application/json");
  headers.push_back(accept_header);

  // Create request content type
  std::string content_type = "application/json";

  // POST /v1/confirmation/{confirmation_id_}/{credential}
  BLOG(INFO) << "POST /v1/confirmation/{confirmation_id_}/{credential}";

  auto endpoint = std::string("/v1/confirmation/").append(confirmation_id)
      .append("/").append(uri_encoded_credential);
  auto ads_serve_url = helper::AdsServe::GetURL().append(endpoint);

  BLOG(INFO) << "URL Request:";
  BLOG(INFO) << "  URL: " << ads_serve_url;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header;
  }
  BLOG(INFO) << "  Body: " << body;
  BLOG(INFO) << "  Content_type: " << content_type;

  auto callback = std::bind(&RedeemToken::OnCreateConfirmation,
      this, ads_serve_url, _1, _2, _3, confirmation_id,
      payment_token_base64, blinded_payment_token_base64);

  confirmations_client_->URLRequest(ads_serve_url, headers, body,
      content_type, URLRequestMethod::POST, callback);
}

void RedeemToken::OnCreateConfirmation(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& confirmation_id,
    const std::string& payment_token_base64,
    const std::string& blinded_payment_token_base64) {
  BLOG(INFO) << "OnCreateConfirmation";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 201) {
    BLOG(ERROR) << "Failed to fetch confirmation id";
    OnRedeem(FAILED);
    return;
  }

  // Parse JSON response
  std::unique_ptr<base::Value> value(base::JSONReader::Read(response));
  base::DictionaryValue* dictionary;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(ERROR) << "Invalid response";
    OnRedeem(FAILED);
    return;
  }

  base::Value *v;

  // Get id
  if (!(v = dictionary->FindKey("id"))) {
    BLOG(ERROR) << "Response missing id";
    OnRedeem(FAILED);
    return;
  }

  auto id = v->GetString();

  // Validate id
  if (confirmation_id != id) {
    BLOG(ERROR) << "Response confirmation id: " << confirmation_id
        << " does not match id: " << id;
    OnRedeem(FAILED);
    return;
  }

  // Fetch payment token
  FetchPaymentToken(confirmation_id, payment_token_base64,
      blinded_payment_token_base64);
}

void RedeemToken::FetchPaymentToken(
    const std::string& confirmation_id,
    const std::string& payment_token_base64,
    const std::string& blinded_payment_token_base64) {
  BLOG(INFO) << "FetchPaymentToken";

  // GET /v1/confirmation/{confirmation_id}/paymentToken
  BLOG(INFO) << "GET /v1/confirmation/{confirmation_id}/paymentToken";

  auto endpoint = std::string("/v1/confirmation/").append(confirmation_id)
      .append("/paymentToken");
  auto ads_serve_url = helper::AdsServe::GetURL().append(endpoint);

  BLOG(INFO) << "URL Request:";
  BLOG(INFO) << "  URL: " << ads_serve_url;

  auto callback = std::bind(&RedeemToken::OnFetchPaymentToken,
      this, ads_serve_url, _1, _2, _3, payment_token_base64,
      blinded_payment_token_base64);

  confirmations_client_->URLRequest(ads_serve_url, {}, "", "",
      URLRequestMethod::GET, callback);
}

void RedeemToken::OnFetchPaymentToken(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& payment_token_base64,
    const std::string& blinded_payment_token_base64) {
  BLOG(INFO) << "OnFetchPaymentToken";

  BLOG(INFO) << "URL Request Response:";
  BLOG(INFO) << "  URL: " << url;
  BLOG(INFO) << "  Response Status Code: " << response_status_code;
  BLOG(INFO) << "  Response: " << response;
  BLOG(INFO) << "  Headers:";
  for (const auto& header : headers) {
    BLOG(INFO) << "    " << header.first << ": " << header.second;
  }

  if (response_status_code != 200) {
    BLOG(ERROR) << "Failed to fetch payment token";
    OnRedeem(FAILED);
    return;
  }

  // Parse JSON response
  std::unique_ptr<base::Value> value(base::JSONReader::Read(response));
  base::DictionaryValue* dictionary;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(ERROR) << "Invalid response";
    OnRedeem(FAILED);
    return;
  }

  base::Value *v;

  // Get id
  if (!(v = dictionary->FindKey("id"))) {
    BLOG(ERROR) << "Response missing id";
    OnRedeem(FAILED);
    return;
  }
  auto id = v->GetString();

  // Get payment token
  if (!(v = dictionary->FindKey("paymentToken"))) {
    BLOG(ERROR) << "Response missing paymentToken";
    OnRedeem(FAILED);
    return;
  }

  // Get payment token dictionary
  base::DictionaryValue* payment_token_dictionary;
  if (!v->GetAsDictionary(&payment_token_dictionary)) {
    BLOG(ERROR) << "Response missing paymentToken dictionary";
    OnRedeem(FAILED);
    return;
  }

  // Get public key
  if (!(v = payment_token_dictionary->FindKey("publicKey"))) {
    BLOG(ERROR) << "Response missing publicKey";
    OnRedeem(FAILED);
    return;
  }
  auto public_key_base64 = v->GetString();
  auto public_key = PublicKey::decode_base64(public_key_base64);

  // Validate public key
  auto catalog_issuers = confirmations_->GetCatalogIssuers();

  auto iterator = catalog_issuers.find(public_key_base64);
  if (iterator == catalog_issuers.end()) {
    BLOG(ERROR) << "Response public_key: " << public_key_base64
        << " was not found in the catalog issuers";
    OnRedeem(FAILED);
    return;
  }
  auto issuer_name = iterator->second;

  // Get batch proof
  if (!(v = payment_token_dictionary->FindKey("batchProof"))) {
    BLOG(ERROR) << "Response missing batchProof";
    OnRedeem(FAILED);
    return;
  }
  auto batch_dleq_proof_base64 = v->GetString();
  auto batch_dleq_proof =
      BatchDLEQProof::decode_base64(batch_dleq_proof_base64);

  // Get signed tokens
  if (!(v = payment_token_dictionary->FindKey("signedTokens"))) {
    BLOG(ERROR) << "Response missing signedTokens";
    OnRedeem(FAILED);
    return;
  }

  base::ListValue list(v->GetList());

  if (list.GetSize() != 1) {
    BLOG(ERROR) << "Invalid signedTokens list size";
    OnRedeem(FAILED);
    return;
  }

  std::vector<SignedToken> signed_tokens;
  for (size_t i = 0; i < list.GetSize(); i++) {
    base::Value *signed_token_value;
    list.Get(i, &signed_token_value);

    auto signed_token_base64 = signed_token_value->GetString();
    auto signed_token = SignedToken::decode_base64(signed_token_base64);
    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind payment tokens
  auto payment_token = Token::decode_base64(payment_token_base64);

  auto blinded_payment_token =
      BlindedToken::decode_base64(blinded_payment_token_base64);

  auto unblinded_payment_tokens = batch_dleq_proof.verify_and_unblind(
     {payment_token}, {blinded_payment_token}, signed_tokens, public_key);

  if (unblinded_payment_tokens.size() == 0) {
    BLOG(ERROR) << "Failed to verify and unblind payment tokens";
    OnRedeem(FAILED);
    return;
  }

  BLOG(INFO) << "Generated " << unblinded_payment_tokens.size()
      << " unblinded payment token worth " << issuer_name;

  AppendUnblindedPaymentTokens(unblinded_payment_tokens);

  OnRedeem(SUCCESS);
}

void RedeemToken::OnRedeem(const Result result) {
  if (result != SUCCESS) {
    BLOG(ERROR) << "Failed to redeem token";
  } else {
    BLOG(INFO) << "Successfully redeemed token";
  }

  RemoveUnblindedToken();
}

void RedeemToken::RemoveUnblindedToken() {
  BLOG(INFO) << "Removing unblinded token";

  auto tokens = confirmations_->GetUnblindedTokens();
  if (tokens.size() == 0) {
    BLOG(ERROR) << "Failed to remove unblinded payment token";
    return;
  }

  tokens.erase(tokens.begin());

  confirmations_->SetUnblindedTokens(tokens);

  BLOG(INFO) << "Removed unblinded token";
}

void RedeemToken::AppendUnblindedPaymentTokens(
    const std::vector<UnblindedToken>& tokens) {
  auto unblinded_payment_tokens =
      confirmations_->GetUnblindedPaymentTokens();

  unblinded_payment_tokens.insert(unblinded_payment_tokens.end(),
      tokens.begin(), tokens.end());

  BLOG(INFO) << "Added " << tokens.size()
      << " unblinded payment tokens, you now have "
      << unblinded_payment_tokens.size() << " unblinded payment tokens";

  confirmations_->SetUnblindedPaymentTokens(unblinded_payment_tokens);
}

}  // namespace confirmations
