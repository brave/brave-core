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
#include "base/notimplemented.h"
#include "base/strings/strcat.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
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
using endpoint_client::RequestCancelability;
using endpoint_client::RequestHandle;
using endpoint_client::WithHeaders;
using endpoints::Error;
using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::VerifyResult;

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : BraveAccountService(pref_service,
                          std::move(url_loader_factory),
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
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    OSCryptCallback encrypt_callback,
    OSCryptCallback decrypt_callback,
    std::unique_ptr<base::OneShotTimer> verify_result_timer)
    : pref_service_(pref_service),
      url_loader_factory_(std::move(url_loader_factory)),
      encrypt_callback_(std::move(encrypt_callback)),
      decrypt_callback_(std::move(decrypt_callback)),
      verify_result_timer_(std::move(verify_result_timer)) {
  CHECK(pref_service_);
  CHECK(url_loader_factory_);
  CHECK(encrypt_callback_);
  CHECK(decrypt_callback_);
  CHECK(verify_result_timer_);

  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      prefs::kBraveAccountVerificationToken,
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

  const std::string verification_token = Decrypt(encrypted_verification_token);
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

void BraveAccountService::ResendConfirmationEmail() {
  // TODO(https://github.com/brave/brave-browser/issues/50653)
  NOTIMPLEMENTED();
}

void BraveAccountService::CancelRegistration() {
  pref_service_->ClearPref(prefs::kBraveAccountVerificationToken);
}

void BraveAccountService::LogOut() {
  // TODO(https://github.com/brave/brave-browser/issues/50651)
  pref_service_->ClearPref(prefs::kBraveAccountAuthenticationToken);
}

void BraveAccountService::OnRegisterInitialize(
    RegisterInitializeCallback callback,
    PasswordInit::Response response) {
  if (!response.body) {
    return std::move(callback).Run(base::unexpected(mojom::RegisterError::New(
        response.status_code.value_or(response.net_error), std::nullopt)));
  }

  CHECK(response.status_code);
  const auto status_code = *response.status_code;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody    ]> ==>
          // expected<SuccessBody, [RegisterError]>
          .transform_error([&](auto error_body) {
            return mojom::RegisterError::New(
                status_code, TransformError(std::move(error_body)));
          })
          // expected<[SuccessBody                ], RegisterError> ==>
          // expected<[RegisterInitializeResultPtr], RegisterError>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::RegisterInitializeResultPtr,
                                          mojom::RegisterErrorPtr> {
            if (success_body.verification_token.empty() ||
                success_body.serialized_response.empty()) {
              return base::unexpected(
                  mojom::RegisterError::New(status_code, std::nullopt));
            }

            std::string encrypted_verification_token =
                Encrypt(success_body.verification_token);
            if (encrypted_verification_token.empty()) {
              return base::unexpected(mojom::RegisterError::New(
                  std::nullopt, mojom::RegisterErrorCode::
                                    kVerificationTokenEncryptionFailed));
            }

            return mojom::RegisterInitializeResult::New(
                std::move(encrypted_verification_token),
                std::move(success_body.serialized_response));
          });

  std::move(callback).Run(std::move(result));
}

void BraveAccountService::OnRegisterFinalize(
    RegisterFinalizeCallback callback,
    const std::string& encrypted_verification_token,
    PasswordFinalize::Response response) {
  if (!response.body) {
    return std::move(callback).Run(base::unexpected(mojom::RegisterError::New(
        response.status_code.value_or(response.net_error), std::nullopt)));
  }

  CHECK(response.status_code);
  const auto status_code = *response.status_code;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody    ]> ==>
          // expected<SuccessBody, [RegisterError]>
          .transform_error([&](auto error_body) {
            return mojom::RegisterError::New(
                status_code, TransformError(std::move(error_body)));
          })
          // expected<[SuccessBody              ], RegisterError> ==>
          // expected<[RegisterFinalizeResultPtr], RegisterError>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::RegisterFinalizeResultPtr,
                                          mojom::RegisterErrorPtr> {
            pref_service_->SetString(prefs::kBraveAccountVerificationToken,
                                     encrypted_verification_token);

            return mojom::RegisterFinalizeResult::New();
          });

  std::move(callback).Run(std::move(result));
}

std::optional<mojom::RegisterErrorCode> BraveAccountService::TransformError(
    Error error_body) {
  if (!error_body.code.is_int()) {
    return std::nullopt;
  }

  const auto error_code =
      static_cast<mojom::RegisterErrorCode>(error_body.code.GetInt());
  return mojom::IsKnownEnumValue(error_code) ? std::optional(error_code)
                                             : std::nullopt;
}

void BraveAccountService::OnVerificationTokenChanged() {
  if (pref_service_->GetString(prefs::kBraveAccountVerificationToken).empty()) {
    return CHECK_DEREF(verify_result_timer_.get()).Stop();
  }

  ScheduleVerifyResult();
}

void BraveAccountService::ScheduleVerifyResult(
    base::TimeDelta delay,
    RequestHandle current_verify_result_request) {
  CHECK_DEREF(verify_result_timer_.get())
      .Start(FROM_HERE, delay,
             base::BindOnce(&BraveAccountService::VerifyResult,
                            base::Unretained(this),
                            std::move(current_verify_result_request)));
}

void BraveAccountService::VerifyResult(
    RequestHandle current_verify_result_request) {
  current_verify_result_request.reset();

  const auto encrypted_verification_token =
      pref_service_->GetString(prefs::kBraveAccountVerificationToken);
  if (encrypted_verification_token.empty()) {
    return;
  }

  const auto verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return;
  }

  WithHeaders<VerifyResult::Request> request;
  request.network_traffic_annotation_tag =
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
  request.wait = false;
  request.headers.SetHeader("Authorization",
                            base::StrCat({"Bearer ", verification_token}));
  current_verify_result_request =
      Client<endpoints::VerifyResult>::Send<RequestCancelability::kCancelable>(
          url_loader_factory_, std::move(request),
          base::BindOnce(&BraveAccountService::OnVerifyResult,
                         weak_factory_.GetWeakPtr()));

  // Replace normal cadence with the watchdog timer.
  ScheduleVerifyResult(kVerifyResultWatchdogInterval,
                       std::move(current_verify_result_request));
}

void BraveAccountService::OnVerifyResult(VerifyResult::Response response) {
  const auto authentication_token =
      response.body
          ? std::move(*response.body)
                .transform([](auto success_body) {
                  if (auto* auth_token = success_body.auth_token.GetIfString();
                      !auth_token || auth_token->empty()) {
                    return std::string();
                  }
                  return std::move(success_body.auth_token).TakeString();
                })
                .value_or("")
          : "";

  if (!authentication_token.empty()) {
    // We stop polling regardless of encryption success,
    // since the auth token is transient on the server
    // and cannot be retrieved again.
    // TODO(https://github.com/brave/brave-browser/issues/50307)
    pref_service_->ClearPref(prefs::kBraveAccountVerificationToken);

    if (const auto encrypted_authentication_token =
            Encrypt(authentication_token);
        !encrypted_authentication_token.empty()) {
      pref_service_->SetString(prefs::kBraveAccountAuthenticationToken,
                               encrypted_authentication_token);
    }

    return;
  }

  if (const auto status_code = response.status_code.value_or(-1);
      status_code >= 300 && status_code < 500) {
    // Polling cannot recover from these errors, so we stop further attempts.
    return pref_service_->ClearPref(prefs::kBraveAccountVerificationToken);
  }

  // Replace watchdog timer with the normal cadence.
  ScheduleVerifyResult(kVerifyResultPollInterval);
}

}  // namespace brave_account
