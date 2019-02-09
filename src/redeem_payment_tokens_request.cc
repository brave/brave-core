/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "redeem_payment_tokens_request.h"
#include "ads_serve_helper.h"

#include "base/json/json_writer.h"

namespace confirmations {

RedeemPaymentTokensRequest::RedeemPaymentTokensRequest() = default;

RedeemPaymentTokensRequest::~RedeemPaymentTokensRequest() = default;

// PUT /v1/confirmation/payment/{payment_id}

std::string RedeemPaymentTokensRequest::BuildUrl(
    const WalletInfo& wallet_info) const {
  std::string endpoint = "/v1/confirmation/payment/";
  endpoint += wallet_info.payment_id;

  return helper::AdsServe::GetURL().append(endpoint);
}

URLRequestMethod RedeemPaymentTokensRequest::GetMethod() const {
  return PUT;
}

std::string RedeemPaymentTokensRequest::BuildBody(
    const std::vector<UnblindedToken>& tokens,
    const std::string& payload,
    const WalletInfo& wallet_info) const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  auto payment_request_dto = CreatePaymentRequestDTO(tokens, payload,
      wallet_info);

  dictionary.SetKey("paymentCredentials", std::move(payment_request_dto));

  dictionary.SetKey("payload", base::Value(payload));

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  return json;
}

std::string RedeemPaymentTokensRequest::CreatePayload(
    const WalletInfo& wallet_info) const {
  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetKey("paymentId", base::Value(wallet_info.payment_id));

  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

std::vector<std::string> RedeemPaymentTokensRequest::BuildHeaders() const {
  std::string accept_header = "accept: ";
  accept_header += GetAcceptHeaderValue();

  return {
    accept_header
  };
}

std::string RedeemPaymentTokensRequest::GetAcceptHeaderValue() const {
  return "application/json";
}

std::string RedeemPaymentTokensRequest::GetContentType() const {
  return "application/json";
}

///////////////////////////////////////////////////////////////////////////////

base::Value RedeemPaymentTokensRequest::CreatePaymentRequestDTO(
    const std::vector<UnblindedToken>& tokens,
    const std::string& payload,
    const WalletInfo& wallet_info) const {
  base::Value payment_credentials(base::Value::Type::LIST);

  for (const auto& token : tokens) {
    base::Value payment_credential(base::Value::Type::DICTIONARY);

    auto credential = CreateCredential(token, payload);
    payment_credential.SetKey("credential", base::Value(std::move(credential)));

    payment_credential.SetKey("publicKey", base::Value(wallet_info.public_key));

    payment_credentials.GetList().push_back(std::move(payment_credential));
  }

  return payment_credentials;
}

base::Value RedeemPaymentTokensRequest::CreateCredential(
    const UnblindedToken& token,
    const std::string& payload) const {
  base::Value dictionary(base::Value::Type::DICTIONARY);

  auto verification_key = token.derive_verification_key();
  auto signed_verification_key = verification_key.sign(payload);
  auto signed_verification_key_base64 = signed_verification_key.encode_base64();
  dictionary.SetKey("signature", base::Value(signed_verification_key_base64));

  auto preimage = token.preimage();
  auto preimage_base64 = preimage.encode_base64();
  dictionary.SetKey("t", base::Value(preimage_base64));

  return dictionary;
}

}  // namespace confirmations
