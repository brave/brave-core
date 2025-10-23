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
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
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

}  // namespace

using endpoint_client::Client;
using endpoint_client::WithHeaders;
using endpoints::Error;
using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::VerifyResult;

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : BraveAccountService(
          pref_service,
          std::make_unique<api_request_helper::APIRequestHelper>(
              kTrafficAnnotation,
              std::move(url_loader_factory)),
          base::BindRepeating(&OSCrypt::EncryptString),
          base::BindRepeating(&OSCrypt::DecryptString),
          std::make_unique<base::OneShotTimer>()) {}

BraveAccountService::~BraveAccountService() = default;

void BraveAccountService::BindInterface(
    mojo::PendingReceiver<mojom::Authentication> pending_receiver) {
  authentication_receivers_.Add(this, std::move(pending_receiver));
}

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper,
    OSCryptCallback encrypt_callback,
    OSCryptCallback decrypt_callback,
    std::unique_ptr<base::OneShotTimer> verify_result_timer)
    : pref_service_(pref_service),
      api_request_helper_(std::move(api_request_helper)),
      encrypt_callback_(std::move(encrypt_callback)),
      decrypt_callback_(std::move(decrypt_callback)),
      verify_result_timer_(std::move(verify_result_timer)) {
  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      prefs::kVerificationToken,
      base::BindRepeating(&BraveAccountService::OnVerificationTokenChanged,
                          base::Unretained(this)));
  OnVerificationTokenChanged();
}

std::string BraveAccountService::Encrypt(const std::string& plain_text) const {
  if (plain_text.empty()) {
    return std::string();
  }

  std::string encrypted;
  if (!encrypt_callback_.Run(plain_text, &encrypted)) {
    return std::string();
  }

  return base::Base64Encode(encrypted);
}

std::string BraveAccountService::Decrypt(const std::string& base64) const {
  if (base64.empty()) {
    return std::string();
  }

  std::string encrypted;
  if (!base::Base64Decode(base64, &encrypted)) {
    return std::string();
  }

  std::string plain_text;
  if (!decrypt_callback_.Run(encrypted, &plain_text)) {
    return std::string();
  }

  return plain_text;
}

void BraveAccountService::RegisterInitialize(
    const std::string& email,
    const std::string& blinded_message,
    RegisterInitializeCallback callback) {
  if (email.empty() || blinded_message.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterError::New()));
  }

  PasswordInit::Request request;
  request.blinded_message = blinded_message;
  request.new_account_email = email;
  request.serialize_response = true;
  Client<PasswordInit>::Send(
      CHECK_DEREF(api_request_helper_.get()), std::move(request),
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

  const std::string verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(base::unexpected(mojom::RegisterError::New(
        std::nullopt,
        mojom::RegisterErrorCode::kVerificationTokenDecryptionFailed)));
  }

  WithHeaders<PasswordFinalize::Request> request;
  request.serialized_record = serialized_record;
  request.headers.SetHeader("Authorization",
                            base::StrCat({"Bearer ", verification_token}));
  Client<PasswordFinalize>::Send(
      CHECK_DEREF(api_request_helper_.get()), std::move(request),
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
                Encrypt(response->verification_token);
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

void BraveAccountService::OnVerificationTokenChanged() {
  if (pref_service_->GetString(prefs::kVerificationToken).empty()) {
    return CHECK_DEREF(verify_result_timer_.get()).Stop();
  }

  ScheduleVerifyResult();
}

void BraveAccountService::ScheduleVerifyResult(base::TimeDelta delay) {
  CHECK_DEREF(verify_result_timer_.get())
      .Start(FROM_HERE, delay,
             base::BindOnce(&BraveAccountService::VerifyResult,
                            base::Unretained(this)));
}

void BraveAccountService::VerifyResult() {
  const auto encrypted_verification_token =
      pref_service_->GetString(prefs::kVerificationToken);
  if (encrypted_verification_token.empty()) {
    return;
  }

  const auto verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/50428):
  // Cancel the previous in-flight request before issuing the next.

  WithHeaders<VerifyResult::Request> request;
  request.wait = false;
  request.headers.SetHeader("Authorization",
                            base::StrCat({"Bearer ", verification_token}));
  Client<endpoints::VerifyResult>::Send(
      CHECK_DEREF(api_request_helper_.get()), std::move(request),
      base::BindOnce(&BraveAccountService::OnVerifyResult,
                     weak_factory_.GetWeakPtr()));

  // Replace normal cadence with the watchdog timer.
  ScheduleVerifyResult(kVerifyResultWatchdogInterval);
}

void BraveAccountService::OnVerifyResult(
    int response_code,
    base::expected<std::optional<VerifyResult::Response>,
                   std::optional<VerifyResult::Error>> reply) {
  const auto authentication_token = [&] {
    auto response = std::move(reply).value_or(std::nullopt);

    if (auto* auth_token =
            response ? response->auth_token.GetIfString() : nullptr;
        !auth_token || auth_token->empty()) {
      return std::string();
    }

    return std::move(response->auth_token).TakeString();
  }();

  if (!authentication_token.empty()) {
    // We stop polling regardless of encryption success,
    // since the auth token is transient on the server
    // and cannot be retrieved again.
    // TODO(https://github.com/brave/brave-browser/issues/50307)
    pref_service_->ClearPref(prefs::kVerificationToken);

    if (const auto encrypted_authentication_token =
            Encrypt(authentication_token);
        !encrypted_authentication_token.empty()) {
      pref_service_->SetString(prefs::kAuthenticationToken,
                               encrypted_authentication_token);
    }

    return;
  }

  if (response_code == net::HTTP_BAD_REQUEST ||
      response_code == net::HTTP_UNAUTHORIZED) {
    // Polling cannot recover from these errors, so we stop further attempts.
    return pref_service_->ClearPref(prefs::kVerificationToken);
  }

  // Replace watchdog timer with the normal cadence.
  ScheduleVerifyResult(kVerifyResultPollInterval);
}

}  // namespace brave_account
