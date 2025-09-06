/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <utility>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {
inline constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_account_endpoints",
                                        R"(
  semantics {
    sender: "Brave Account client"
    description:
      "Implements the creation or sign-in process for a Brave Account."
    trigger:
      "User attempts to create or sign in to a Brave Account from settings."
    user_data: {
      type: EMAIL
    }
    data:
      "Blinded cryptographic message for secure password setup "
      "and account email address."
      "Verification token for account activation and "
      "serialized cryptographic record for account finalization."
    destination: OTHER
    destination_other: "Brave Account service"
  }
  policy {
    cookies_allowed: NO
    policy_exception_justification:
      "These requests are essential for Brave Account creation and sign-in "
      "and cannot be disabled by policy."
  }
)");

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

using endpoint_client::Client;
using endpoint_client::WithHeaders;
using endpoints::PasswordFinalize;
using endpoints::PasswordInit;

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : BraveAccountService(
          pref_service,
          std::make_unique<api_request_helper::APIRequestHelper>(
              kTrafficAnnotation,
              std::move(url_loader_factory)),
          base::BindRepeating(&Encrypt),
          base::BindRepeating(&Decrypt)) {}

BraveAccountService::~BraveAccountService() = default;

void BraveAccountService::BindInterface(
    mojo::PendingReceiver<mojom::Authentication> pending_receiver) {
  authentication_receivers_.Add(this, std::move(pending_receiver));
}

void BraveAccountService::RegisterInitialize(
    const std::string& email,
    const std::string& blinded_message,
    mojom::Authentication::RegisterInitializeCallback callback) {
  DCHECK(!email.empty());
  DCHECK(!blinded_message.empty());

  if (email.empty() || blinded_message.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected));
  }

  PasswordInit::Request request;
  request.blinded_message = blinded_message;
  request.new_account_email = email;
  request.serialize_response = true;
  Client<PasswordInit>::Send(
      *api_request_helper_, std::move(request),
      base::BindOnce(&BraveAccountService::OnRegisterInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::RegisterFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    mojom::Authentication::RegisterFinalizeCallback callback) {
  DCHECK(!encrypted_verification_token.empty());
  DCHECK(!serialized_record.empty());

  if (encrypted_verification_token.empty() || serialized_record.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kFinalizeUnexpected));
  }

  std::string verification_token =
      decrypt_fn_.Run(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kFinalizeUnexpected));
  }

  WithHeaders<PasswordFinalize::Request> request;
  request.serialized_record = serialized_record;
  request.headers.SetHeader("Authorization",
                            base::StrCat({"Bearer ", verification_token}));
  Client<PasswordFinalize>::Send(
      *api_request_helper_, std::move(request),
      base::BindOnce(&BraveAccountService::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     encrypted_verification_token));
}

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper,
    CryptoFn encrypt_fn,
    CryptoFn decrypt_fn)
    : pref_service_(pref_service),
      api_request_helper_(std::move(api_request_helper)),
      encrypt_fn_(std::move(encrypt_fn)),
      decrypt_fn_(std::move(decrypt_fn)) {}

void BraveAccountService::OnRegisterInitialize(
    mojom::Authentication::RegisterInitializeCallback callback,
    int response_code,
    base::expected<std::optional<PasswordInit::Response>,
                   std::optional<PasswordInit::Error>> reply) {
  auto result =
      std::move(reply)
          // expected<optional<Response>, [optional<Error>      ]> ==>
          // expected<optional<Response>, [RegisterFailureReason]>
          .transform_error([](const auto& error) {
            if (!error) {
              return mojom::RegisterFailureReason::kInitializeUnexpected;
            }

            switch (error->status) {
              case net::HTTP_BAD_REQUEST:  // 400
                return mojom::RegisterFailureReason::kInitializeBadRequest;
              case net::HTTP_UNAUTHORIZED:  // 401
                return mojom::RegisterFailureReason::kInitializeUnauthorized;
              default:
                if (error->status >= 500 && error->status < 600) {
                  return mojom::RegisterFailureReason::kInitializeServerError;
                }
                return mojom::RegisterFailureReason::kInitializeUnknown;
            }
          })
          // expected<[optional<Response>         ], RegisterFailureReason> ==>
          // expected<[RegisterInitializeResultPtr], RegisterFailureReason>
          .and_then([&](const auto& response)
                        -> base::expected<mojom::RegisterInitializeResultPtr,
                                          mojom::RegisterFailureReason> {
            if (!response) {
              return base::unexpected(
                  mojom::RegisterFailureReason::kInitializeUnexpected);
            }

            if (response->verification_token.empty() ||
                response->serialized_response.empty()) {
              return base::unexpected(
                  mojom::RegisterFailureReason::kInitializeUnexpected);
            }

            std::string encrypted =
                encrypt_fn_.Run(response->verification_token);
            if (encrypted.empty()) {
              return base::unexpected(
                  mojom::RegisterFailureReason::kInitializeUnexpected);
            }

            return mojom::RegisterInitializeResult::New(
                std::move(encrypted), response->serialized_response);
          });

  std::move(callback).Run(std::move(result));
}

void BraveAccountService::OnRegisterFinalize(
    mojom::Authentication::RegisterFinalizeCallback callback,
    const std::string& encrypted_verification_token,
    int response_code,
    base::expected<std::optional<PasswordFinalize::Response>,
                   std::optional<PasswordFinalize::Error>> reply) {
  auto result =
      std::move(reply)
          // expected<optional<Response>, [optional<Error>      ]> ==>
          // expected<optional<Response>, [RegisterFailureReason]>
          .transform_error([](const auto& error) {
            if (!error) {
              return mojom::RegisterFailureReason::kFinalizeUnexpected;
            }

            switch (error->status) {
              case net::HTTP_BAD_REQUEST:  // 400
                return mojom::RegisterFailureReason::kFinalizeBadRequest;
              case net::HTTP_UNAUTHORIZED:  // 401
                return mojom::RegisterFailureReason::kFinalizeUnauthorized;
              case net::HTTP_FORBIDDEN:  // 403
                return mojom::RegisterFailureReason::kFinalizeForbidden;
              case net::HTTP_NOT_FOUND:  // 404
                return mojom::RegisterFailureReason::kFinalizeNotFound;
              default:
                if (error->status >= 500 && error->status < 600) {
                  return mojom::RegisterFailureReason::kFinalizeServerError;
                }
                return mojom::RegisterFailureReason::kFinalizeUnknown;
            }
          })
          // expected<[optional<Response>       ], RegisterFailureReason> ==>
          // expected<[RegisterFinalizeResultPtr], RegisterFailureReason>
          .and_then([&](const auto& response)
                        -> base::expected<mojom::RegisterFinalizeResultPtr,
                                          mojom::RegisterFailureReason> {
            pref_service_->SetString(prefs::kVerificationToken,
                                     encrypted_verification_token);

            return mojom::RegisterFinalizeResult::New();
          });

  std::move(callback).Run(std::move(result));
}

}  // namespace brave_account
