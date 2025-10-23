/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <utility>

#include "base/base64.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

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

std::string Encrypt(const std::string& plain_text,
                    BraveAccountService::OSCryptCallback encrypt_callback) {
  if (plain_text.empty()) {
    return std::string();
  }

  std::string encrypted;
  if (!encrypt_callback.Run(plain_text, &encrypted)) {
    return std::string();
  }

  return base::Base64Encode(encrypted);
}

std::string Decrypt(const std::string& base64,
                    BraveAccountService::OSCryptCallback decrypt_callback) {
  if (base64.empty()) {
    return std::string();
  }

  std::string encrypted;
  if (!base::Base64Decode(base64, &encrypted)) {
    return std::string();
  }

  std::string plain_text;
  if (!decrypt_callback.Run(encrypted, &plain_text)) {
    return std::string();
  }

  return plain_text;
}

}  // namespace

using endpoint_client::Client;
using endpoint_client::WithHeaders;
using endpoints::Error;
using endpoints::PasswordFinalize;
using endpoints::PasswordInit;

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : BraveAccountService(pref_service,
                          std::move(url_loader_factory),
                          base::BindRepeating(&OSCrypt::EncryptString),
                          base::BindRepeating(&OSCrypt::DecryptString)) {}

BraveAccountService::~BraveAccountService() = default;

void BraveAccountService::BindInterface(
    mojo::PendingReceiver<mojom::Authentication> pending_receiver) {
  authentication_receivers_.Add(this, std::move(pending_receiver));
}

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    OSCryptCallback encrypt_callback,
    OSCryptCallback decrypt_callback)
    : pref_service_(pref_service),
      url_loader_factory_(std::move(url_loader_factory)),
      encrypt_callback_(std::move(encrypt_callback)),
      decrypt_callback_(std::move(decrypt_callback)) {}

void BraveAccountService::RegisterInitialize(
    const std::string& email,
    const std::string& blinded_message,
    RegisterInitializeCallback callback) {
  if (email.empty() || blinded_message.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterError::New()));
  }

  PasswordInit::Request request;
  request.network_traffic_annotation_tag =
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
  request.blinded_message = blinded_message;
  request.new_account_email = email;
  request.serialize_response = true;
  Client<PasswordInit>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnRegisterInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::RegisterFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    RegisterFinalizeCallback callback) {
  if (encrypted_verification_token.empty() || serialized_record.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterError::New()));
  }

  const std::string verification_token =
      Decrypt(encrypted_verification_token, decrypt_callback_);
  if (verification_token.empty()) {
    return std::move(callback).Run(base::unexpected(mojom::RegisterError::New(
        std::nullopt,
        mojom::RegisterErrorCode::kVerificationTokenDecryptionFailed)));
  }

  WithHeaders<PasswordFinalize::Request> request;
  request.network_traffic_annotation_tag =
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
  request.serialized_record = serialized_record;
  request.headers.SetHeader("Authorization",
                            base::StrCat({"Bearer ", verification_token}));
  Client<PasswordFinalize>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     encrypted_verification_token));
}

void BraveAccountService::OnRegisterInitialize(
    RegisterInitializeCallback callback,
    int response_code,
    base::expected<std::optional<PasswordInit::Response>,
                   std::optional<PasswordInit::Error>> reply) {
  auto result =
      std::move(reply)
          // expected<optional<Response>, [optional<Error>]> ==>
          // expected<optional<Response>, [RegisterError  ]>
          .transform_error([&](auto error) {
            return mojom::RegisterError::New(response_code,
                                             TransformError(std::move(error)));
          })
          // expected<[optional<Response>         ], RegisterError> ==>
          // expected<[RegisterInitializeResultPtr], RegisterError>
          .and_then([&](auto response)
                        -> base::expected<mojom::RegisterInitializeResultPtr,
                                          mojom::RegisterErrorPtr> {
            if (!response) {
              return base::unexpected(
                  mojom::RegisterError::New(response_code, std::nullopt));
            }

            if (response->verification_token.empty() ||
                response->serialized_response.empty()) {
              return base::unexpected(
                  mojom::RegisterError::New(response_code, std::nullopt));
            }

            std::string encrypted_verification_token =
                Encrypt(response->verification_token, encrypt_callback_);
            if (encrypted_verification_token.empty()) {
              return base::unexpected(mojom::RegisterError::New(
                  std::nullopt, mojom::RegisterErrorCode::
                                    kVerificationTokenEncryptionFailed));
            }

            return mojom::RegisterInitializeResult::New(
                std::move(encrypted_verification_token),
                std::move(response->serialized_response));
          });

  std::move(callback).Run(std::move(result));
}

void BraveAccountService::OnRegisterFinalize(
    RegisterFinalizeCallback callback,
    const std::string& encrypted_verification_token,
    int response_code,
    base::expected<std::optional<PasswordFinalize::Response>,
                   std::optional<PasswordFinalize::Error>> reply) {
  auto result =
      std::move(reply)
          // expected<optional<Response>, [optional<Error>]> ==>
          // expected<optional<Response>, [RegisterError  ]>
          .transform_error([&](auto error) {
            return mojom::RegisterError::New(response_code,
                                             TransformError(std::move(error)));
          })
          // expected<[optional<Response>       ], RegisterError> ==>
          // expected<[RegisterFinalizeResultPtr], RegisterError>
          .and_then([&](auto response)
                        -> base::expected<mojom::RegisterFinalizeResultPtr,
                                          mojom::RegisterErrorPtr> {
            pref_service_->SetString(prefs::kVerificationToken,
                                     encrypted_verification_token);

            return mojom::RegisterFinalizeResult::New();
          });

  std::move(callback).Run(std::move(result));
}

std::optional<mojom::RegisterErrorCode> BraveAccountService::TransformError(
    std::optional<Error> error) {
  if (!error || !error->code.is_int()) {
    return std::nullopt;
  }

  const auto error_code =
      static_cast<mojom::RegisterErrorCode>(error->code.GetInt());
  return mojom::IsKnownEnumValue(error_code) ? std::optional(error_code)
                                             : std::nullopt;
}

}  // namespace brave_account
