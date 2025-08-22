/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <utility>

#include "base/base64.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_account/endpoints/endpoints.h"
#include "brave/components/brave_account/endpoints/requests/password_finalize.h"
#include "brave/components/brave_account/endpoints/requests/password_init.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {
std::string Encrypt(const std::string& plain_text) {
  if (plain_text.empty()) {
    return std::string();
  }

  std::string encrypted;
  if (!OSCrypt::EncryptString(plain_text, &encrypted)) {
    return std::string();
  }

  return base::Base64Encode(encrypted);
}

std::string Decrypt(const std::string& base64) {
  if (base64.empty()) {
    return std::string();
  }

  std::string encrypted;
  if (!base::Base64Decode(base64, &encrypted)) {
    return std::string();
  }

  std::string plain_text;
  if (!OSCrypt::DecryptString(encrypted, &plain_text)) {
    return std::string();
  }

  return plain_text;
}
}  // namespace

namespace brave_account {

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : pref_service_(pref_service),
      url_loader_factory_(url_loader_factory),
      api_request_helper_(endpoints::kTrafficAnnotation, url_loader_factory_) {}

BraveAccountService::~BraveAccountService() = default;

void BraveAccountService::RegisterInitialize(
    const std::string& email,
    const std::string& blinded_message,
    mojom::PageHandler::RegisterInitializeCallback callback) {
  DCHECK(!email.empty());
  DCHECK(!blinded_message.empty());

  if (email.empty() || blinded_message.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected));
  }

  endpoints::PasswordInitRequest request;
  request.blinded_message = blinded_message;
  request.new_account_email = email;
  request.serialize_response = true;
  endpoints::Client<endpoints::PasswordInit>::Send(
      api_request_helper_, request,
      base::BindOnce(&BraveAccountService::OnRegisterInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::RegisterFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    mojom::PageHandler::RegisterFinalizeCallback callback) {
  DCHECK(!encrypted_verification_token.empty());
  DCHECK(!serialized_record.empty());

  if (encrypted_verification_token.empty() || serialized_record.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kFinalizeUnexpected));
  }

  std::string verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kFinalizeUnexpected));
  }

  base::flat_map<std::string, std::string> headers;
  headers.emplace("Authorization",
                  base::StrCat({"Bearer ", verification_token}));
  endpoints::PasswordFinalizeRequest request;
  request.serialized_record = serialized_record;
  endpoints::Client<endpoints::PasswordFinalize>::Send(
      api_request_helper_, request,
      base::BindOnce(&BraveAccountService::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     encrypted_verification_token),
      headers);
}

void BraveAccountService::OnRegisterInitialize(
    mojom::PageHandler::RegisterInitializeCallback callback,
    int response_code,
    std::optional<endpoints::PasswordInitResponse> response) {
  switch (response_code) {
    case 200:
      break;
    case 400:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kInitializeBadRequest));
    case 401:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kInitializeUnauthorized));
    case 403:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kInitializeForbidden));
    case 500:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kInitializeInternalServerError));
    default:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kInitializeUnknown));
  }

  if (!response) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected));
  }

  if (response->verification_token.empty() ||
      response->serialized_response.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected));
  }

  std::string encrypted_verification_token =
      Encrypt(response->verification_token);
  if (encrypted_verification_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected));
  }

  std::move(callback).Run(mojom::RegisterInitializeResult::New(
      std::move(encrypted_verification_token),
      std::move(response->serialized_response)));
}

void BraveAccountService::OnRegisterFinalize(
    mojom::PageHandler::RegisterFinalizeCallback callback,
    const std::string& encrypted_verification_token,
    int response_code,
    std::optional<endpoints::PasswordFinalizeResponse> response) {
  switch (response_code) {
    case 200:
      break;
    case 400:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kFinalizeBadRequest));
    case 401:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kFinalizeUnauthorized));
    case 403:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kFinalizeForbidden));
    case 404:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kFinalizeNotFound));
    case 500:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kFinalizeInternalServerError));
    default:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kFinalizeUnknown));
  }

  if (!response) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kFinalizeUnexpected));
  }

  pref_service_->SetString(prefs::kVerificationToken,
                           encrypted_verification_token);

  std::move(callback).Run(mojom::RegisterFinalizeResult::New());
}

}  // namespace brave_account
