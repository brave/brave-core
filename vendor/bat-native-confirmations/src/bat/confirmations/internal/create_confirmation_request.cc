/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/confirmation_type.h"
#include "bat/confirmations/internal/confirmation_info.h"
#include "bat/confirmations/internal/token_info.h"
#include "bat/confirmations/internal/create_confirmation_request.h"
#include "bat/confirmations/internal/ads_serve_helper.h"
#include "bat/confirmations/internal/security_helper.h"
#include "base/logging.h"
#include "base/json/json_writer.h"
#include "base/values.h"

namespace confirmations {

CreateConfirmationRequest::CreateConfirmationRequest() = default;

CreateConfirmationRequest::~CreateConfirmationRequest() = default;

// POST /v1/confirmation/{confirmation_id}/{credential}

std::string CreateConfirmationRequest::BuildUrl(
    const std::string& confirmation_id,
    const std::string& credential) const {
  DCHECK(!confirmation_id.empty());
  DCHECK(!credential.empty());

  std::string endpoint = "/v1/confirmation/";
  endpoint += confirmation_id;
  endpoint += "/";
  endpoint += credential;

  return helper::AdsServe::GetURL().append(endpoint);
}

URLRequestMethod CreateConfirmationRequest::GetMethod() const {
  return URLRequestMethod::POST;
}

std::string CreateConfirmationRequest::BuildBody(
    const std::string& payload) const {
  DCHECK(!payload.empty());

  return payload;
}

std::vector<std::string> CreateConfirmationRequest::BuildHeaders() const {
  std::string accept_header = "accept: ";
  accept_header += GetAcceptHeaderValue();

  return {
    accept_header
  };
}

std::string CreateConfirmationRequest::GetAcceptHeaderValue() const {
  return "application/json";
}

std::string CreateConfirmationRequest::GetContentType() const {
  return "application/json";
}

std::string CreateConfirmationRequest::CreateConfirmationRequestDTO(
    const ConfirmationInfo& info) const {
  DCHECK(!info.creative_instance_id.empty());

  base::Value payload(base::Value::Type::DICTIONARY);

  payload.SetKey("creativeInstanceId", base::Value(info.creative_instance_id));

  payload.SetKey("payload", base::Value(base::Value::Type::DICTIONARY));

  auto token_base64 = info.blinded_payment_token.encode_base64();
  payload.SetKey("blindedPaymentToken", base::Value(token_base64));

  auto type = std::string(info.type);
  payload.SetKey("type", base::Value(type));

  if (!info.channel_id.empty()) {
    payload.SetKey("channelId", base::Value(info.channel_id));
  }

  std::string json;
  base::JSONWriter::Write(payload, &json);

  return json;
}

std::string CreateConfirmationRequest::CreateCredential(
    const TokenInfo& token_info,
    const std::string& payload) const {
  DCHECK(!payload.empty());

  base::Value dictionary(base::Value::Type::DICTIONARY);

  dictionary.SetKey("payload", base::Value(payload));

  auto verification_key = token_info.unblinded_token.derive_verification_key();
  auto signed_verification_key = verification_key.sign(payload);
  auto signed_verification_key_base64 = signed_verification_key.encode_base64();
  dictionary.SetKey("signature", base::Value(signed_verification_key_base64));

  auto preimage = token_info.unblinded_token.preimage();
  auto preimage_base64 = preimage.encode_base64();
  dictionary.SetKey("t", base::Value(preimage_base64));

  std::string json;
  base::JSONWriter::Write(dictionary, &json);

  std::vector<uint8_t> credential(json.begin(), json.end());
  std::string credential_base64 = helper::Security::GetBase64(credential);

  return credential_base64;
}

}  // namespace confirmations
