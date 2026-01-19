/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <utility>

#include "base/base64.h"
#include "base/containers/fixed_flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/values_util.h"
#include "base/notimplemented.h"
#include "base/strings/strcat.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/endpoints/error_body.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using endpoint_client::Client;
using endpoint_client::RequestCancelability;
using endpoint_client::RequestHandle;
using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::AuthValidate;
using endpoints::ErrorBody;
using endpoints::LoginFinalize;
using endpoints::LoginInit;
using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::ServiceToken;
using endpoints::VerifyResend;
using endpoints::VerifyResult;

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

template <typename Request>
auto MakeRequest() {
  Request request;
  request.network_traffic_annotation_tag =
      net::MutableNetworkTrafficAnnotationTag(kTrafficAnnotation);
  return request;
}

template <typename MojomError>
auto MakeMojomError(int status_code, ErrorBody error_body) {
  auto mojom_error = MojomError::New(status_code, std::nullopt);

  if (!error_body.code.is_int()) {
    return mojom_error;
  }

  const auto error_code =
      static_cast<decltype(mojom_error->errorCode)::value_type>(
          error_body.code.GetInt());
  mojom_error->errorCode = mojom::IsKnownEnumValue(error_code)
                               ? std::optional(error_code)
                               : std::nullopt;
  return mojom_error;
}

}  // namespace

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
      decrypt_callback_(std::move(decrypt_callback)) {
  CHECK(pref_service_);
  CHECK(url_loader_factory_);
  CHECK(encrypt_callback_);
  CHECK(decrypt_callback_);

  pref_verification_token_.Init(
      prefs::kBraveAccountVerificationToken, pref_service,
      base::BindRepeating(&BraveAccountService::OnVerificationTokenChanged,
                          base::Unretained(this)));
  OnVerificationTokenChanged();

  pref_authentication_token_.Init(
      prefs::kBraveAccountAuthenticationToken, pref_service,
      base::BindRepeating(&BraveAccountService::OnAuthenticationTokenChanged,
                          base::Unretained(this)));
  OnAuthenticationTokenChanged();
}

void BraveAccountService::RegisterInitialize(
    const std::string& email,
    const std::string& blinded_message,
    RegisterInitializeCallback callback) {
  if (email.empty() || blinded_message.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterError::New()));
  }

  auto request = MakeRequest<PasswordInit::Request>();
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

  auto request = MakeRequest<WithHeaders<PasswordFinalize::Request>>();
  SetBearerToken(request, verification_token);
  request.serialized_record = serialized_record;
  Client<PasswordFinalize>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     encrypted_verification_token));
}

void BraveAccountService::ResendConfirmationEmail(
    ResendConfirmationEmailCallback callback) {
  const auto encrypted_verification_token =
      pref_service_->GetString(prefs::kBraveAccountVerificationToken);
  if (encrypted_verification_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::ResendConfirmationEmailError::New(
            std::nullopt, mojom::ResendConfirmationEmailErrorCode::
                              kUserNotInTheVerificationState)));
  }

  const auto verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::ResendConfirmationEmailError::New(
            std::nullopt, mojom::ResendConfirmationEmailErrorCode::
                              kVerificationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<VerifyResend::Request>>();
  SetBearerToken(request, verification_token);
  // Server side will determine locale based on the Accept-Language request
  // header (which is included automatically by upstream).
  request.locale = "";
  Client<VerifyResend>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnResendConfirmationEmail,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::CancelRegistration() {
  pref_service_->ClearPref(prefs::kBraveAccountVerificationToken);
}

void BraveAccountService::LoginInitialize(const std::string& email,
                                          const std::string& serialized_ke1,
                                          LoginInitializeCallback callback) {
  if (email.empty() || serialized_ke1.empty()) {
    return std::move(callback).Run(base::unexpected(mojom::LoginError::New()));
  }

  auto request = MakeRequest<LoginInit::Request>();
  request.email = email;
  request.serialized_ke1 = serialized_ke1;
  Client<LoginInit>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnLoginInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::LoginFinalize(
    const std::string& encrypted_login_token,
    const std::string& client_mac,
    LoginFinalizeCallback callback) {
  if (encrypted_login_token.empty() || client_mac.empty()) {
    return std::move(callback).Run(base::unexpected(mojom::LoginError::New()));
  }

  const std::string login_token = Decrypt(encrypted_login_token);
  if (login_token.empty()) {
    return std::move(callback).Run(base::unexpected(mojom::LoginError::New(
        std::nullopt, mojom::LoginErrorCode::kLoginTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<LoginFinalize::Request>>();
  SetBearerToken(request, login_token);
  request.client_mac = client_mac;
  Client<endpoints::LoginFinalize>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnLoginFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::LogOut() {
  // TODO(https://github.com/brave/brave-browser/issues/50651)
  pref_service_->ClearPref(prefs::kBraveAccountAuthenticationToken);
}

void BraveAccountService::GetServiceToken(mojom::Service service,
                                          GetServiceTokenCallback callback) {
  static constexpr auto kServiceToNameMap =
      base::MakeFixedFlatMap<mojom::Service, const char*>({
          {mojom::Service::kEmailAliases, "email-aliases"},
          {mojom::Service::kPremium, "premium"},
          {mojom::Service::kSync, "sync"},
      });
  static_assert(
      kServiceToNameMap.size() ==
          static_cast<std::size_t>(mojom::Service::kMaxValue) + 1,
      "kServiceToNameMap must contain all mojom::Service enum values!");

  std::string service_name = kServiceToNameMap.at(service);
  if (auto service_token = GetCachedServiceToken(service_name);
      !service_token.empty()) {
    return std::move(callback).Run(
        mojom::GetServiceTokenResult::New(std::move(service_token)));
  }

  auto encrypted_authentication_token =
      pref_service_->GetString(prefs::kBraveAccountAuthenticationToken);
  if (encrypted_authentication_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::GetServiceTokenError::New(
            std::nullopt, mojom::GetServiceTokenErrorCode::kUserNotLoggedIn)));
  }

  const auto authentication_token = Decrypt(encrypted_authentication_token);
  if (authentication_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::GetServiceTokenError::New(
            std::nullopt, mojom::GetServiceTokenErrorCode::
                              kAuthenticationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<ServiceToken::Request>>();
  SetBearerToken(request, authentication_token);
  request.service = service_name;
  Client<ServiceToken>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnGetServiceToken,
                     weak_factory_.GetWeakPtr(),
                     std::move(encrypted_authentication_token),
                     std::move(service_name), std::move(callback)));
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
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [RegisterErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeMojomError<mojom::RegisterError>(status_code,
                                                        std::move(error_body));
          })
          // expected<[SuccessBody                ], RegisterErrorPtr> ==>
          // expected<[RegisterInitializeResultPtr], RegisterErrorPtr>
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
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [RegisterErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeMojomError<mojom::RegisterError>(status_code,
                                                        std::move(error_body));
          })
          // expected<[SuccessBody              ], RegisterErrorPtr> ==>
          // expected<[RegisterFinalizeResultPtr], RegisterErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::RegisterFinalizeResultPtr,
                                          mojom::RegisterErrorPtr> {
            pref_service_->SetString(prefs::kBraveAccountVerificationToken,
                                     encrypted_verification_token);

            return mojom::RegisterFinalizeResult::New();
          });

  std::move(callback).Run(std::move(result));
}

void BraveAccountService::OnResendConfirmationEmail(
    ResendConfirmationEmailCallback callback,
    VerifyResend::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(mojom::ResendConfirmationEmailError::New(
            response.status_code.value_or(response.net_error), std::nullopt)));
  }

  CHECK(response.status_code);
  const auto status_code = *response.status_code;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody                      ]> ==>
          // expected<SuccessBody, [ResendConfirmationEmailErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeMojomError<mojom::ResendConfirmationEmailError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody                     ],
          //                         ResendConfirmationEmailErrorPtr> ==>
          // expected<[ResendConfirmationEmailResultPtr],
          //                         ResendConfirmationEmailErrorPtr>
          .and_then(
              [](auto success_body)
                  -> base::expected<mojom::ResendConfirmationEmailResultPtr,
                                    mojom::ResendConfirmationEmailErrorPtr> {
                return mojom::ResendConfirmationEmailResult::New();
              });

  std::move(callback).Run(std::move(result));
}

void BraveAccountService::OnVerificationTokenChanged() {
  if (pref_verification_token_.GetValue().empty()) {
    return verify_result_timer_.Stop();
  }

  ScheduleVerifyResult();
}

void BraveAccountService::ScheduleVerifyResult(
    base::TimeDelta delay,
    RequestHandle current_verify_result_request) {
  verify_result_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&BraveAccountService::VerifyResult, base::Unretained(this),
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

  auto request = MakeRequest<WithHeaders<VerifyResult::Request>>();
  SetBearerToken(request, verification_token);
  request.wait = false;
  current_verify_result_request =
      Client<endpoints::VerifyResult>::Send<RequestCancelability::kCancelable>(
          url_loader_factory_, std::move(request),
          base::BindOnce(&BraveAccountService::OnVerifyResult,
                         weak_factory_.GetWeakPtr()));

  // Replace normal cadence with the watchdog timer.
  ScheduleVerifyResult(kWatchdogInterval,
                       std::move(current_verify_result_request));
}

void BraveAccountService::OnVerifyResult(VerifyResult::Response response) {
  const auto [authentication_token, email] =
      response.body
          ? std::move(*response.body)
                .transform([](auto success_body) {
                  return std::pair(
                      success_body.auth_token.GetIfString()
                          ? std::move(success_body.auth_token).TakeString()
                          : "",
                      std::move(success_body.email).value_or(""));
                })
                .value_or({})
          : std::pair<std::string, std::string>{};

  if (!authentication_token.empty() && !email.empty()) {
    // We stop polling regardless of encryption success,
    // since the auth token is transient on the server
    // and cannot be retrieved again.
    // TODO(https://github.com/brave/brave-browser/issues/50307)
    pref_service_->ClearPref(prefs::kBraveAccountVerificationToken);

    if (const auto encrypted_authentication_token =
            Encrypt(authentication_token);
        !encrypted_authentication_token.empty()) {
      pref_service_->SetString(prefs::kBraveAccountEmailAddress, email);
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

void BraveAccountService::OnLoginInitialize(LoginInitializeCallback callback,
                                            LoginInit::Response response) {
  if (!response.body) {
    return std::move(callback).Run(base::unexpected(mojom::LoginError::New(
        response.status_code.value_or(response.net_error), std::nullopt)));
  }

  CHECK(response.status_code);
  const auto status_code = *response.status_code;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody    ]> ==>
          // expected<SuccessBody, [LoginErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeMojomError<mojom::LoginError>(status_code,
                                                     std::move(error_body));
          })
          // expected<[SuccessBody             ], LoginErrorPtr> ==>
          // expected<[LoginInitializeResultPtr], LoginErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::LoginInitializeResultPtr,
                                          mojom::LoginErrorPtr> {
            if (success_body.login_token.empty() ||
                success_body.serialized_ke2.empty()) {
              return base::unexpected(
                  mojom::LoginError::New(status_code, std::nullopt));
            }

            std::string encrypted_login_token =
                Encrypt(success_body.login_token);
            if (encrypted_login_token.empty()) {
              return base::unexpected(mojom::LoginError::New(
                  std::nullopt,
                  mojom::LoginErrorCode::kLoginTokenEncryptionFailed));
            }

            return mojom::LoginInitializeResult::New(
                std::move(encrypted_login_token),
                std::move(success_body.serialized_ke2));
          });

  std::move(callback).Run(std::move(result));
}

void BraveAccountService::OnLoginFinalize(LoginFinalizeCallback callback,
                                          LoginFinalize::Response response) {
  if (!response.body) {
    return std::move(callback).Run(base::unexpected(mojom::LoginError::New(
        response.status_code.value_or(response.net_error), std::nullopt)));
  }

  CHECK(response.status_code);
  const auto status_code = *response.status_code;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody    ]> ==>
          // expected<SuccessBody, [LoginErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeMojomError<mojom::LoginError>(status_code,
                                                     std::move(error_body));
          })
          // expected<[SuccessBody           ], LoginErrorPtr> ==>
          // expected<[LoginFinalizeResultPtr], LoginErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::LoginFinalizeResultPtr,
                                          mojom::LoginErrorPtr> {
            if (success_body.auth_token.empty() || success_body.email.empty()) {
              return base::unexpected(
                  mojom::LoginError::New(status_code, std::nullopt));
            }

            const std::string encrypted_authentication_token =
                Encrypt(success_body.auth_token);
            if (encrypted_authentication_token.empty()) {
              return base::unexpected(mojom::LoginError::New(
                  std::nullopt,
                  mojom::LoginErrorCode::kAuthenticationTokenEncryptionFailed));
            }

            pref_service_->SetString(prefs::kBraveAccountEmailAddress,
                                     success_body.email);
            pref_service_->SetString(prefs::kBraveAccountAuthenticationToken,
                                     encrypted_authentication_token);

            return mojom::LoginFinalizeResult::New();
          });

  std::move(callback).Run(std::move(result));
}

void BraveAccountService::OnAuthenticationTokenChanged() {
  if (pref_authentication_token_.GetValue().empty()) {
    pref_service_->ClearPref(prefs::kBraveAccountEmailAddress);
    pref_service_->ClearPref(prefs::kBraveAccountServiceTokens);
    return auth_validate_timer_.Stop();
  }

  ScheduleAuthValidate();
}

void BraveAccountService::ScheduleAuthValidate(
    base::TimeDelta delay,
    RequestHandle current_auth_validate_request) {
  auth_validate_timer_.Start(
      FROM_HERE, delay,
      base::BindOnce(&BraveAccountService::AuthValidate, base::Unretained(this),
                     std::move(current_auth_validate_request)));
}

void BraveAccountService::AuthValidate(
    RequestHandle current_auth_validate_request) {
  current_auth_validate_request.reset();

  const auto encrypted_authentication_token =
      pref_service_->GetString(prefs::kBraveAccountAuthenticationToken);
  if (encrypted_authentication_token.empty()) {
    return;
  }

  const auto authentication_token = Decrypt(encrypted_authentication_token);
  if (authentication_token.empty()) {
    return;
  }

  auto request = MakeRequest<WithHeaders<AuthValidate::Request>>();
  SetBearerToken(request, authentication_token);
  current_auth_validate_request =
      Client<endpoints::AuthValidate>::Send<RequestCancelability::kCancelable>(
          url_loader_factory_, std::move(request),
          base::BindOnce(&BraveAccountService::OnAuthValidate,
                         weak_factory_.GetWeakPtr()));

  // Replace normal cadence with the watchdog timer.
  ScheduleAuthValidate(kWatchdogInterval,
                       std::move(current_auth_validate_request));
}

void BraveAccountService::OnAuthValidate(AuthValidate::Response response) {
  const auto email =
      response.body
          ? std::move(*response.body)
                .transform([](auto success_body) { return success_body.email; })
                .value_or("")
          : "";

  if (!email.empty()) {
    pref_service_->SetString(prefs::kBraveAccountEmailAddress, email);
  } else if (const auto status_code = response.status_code.value_or(-1);
             status_code >= 400 && status_code < 500) {
    // Clear the auth token (and stop polling) to prevent
    // presenting invalid state to the user and issuing invalid requests.
    return pref_service_->ClearPref(prefs::kBraveAccountAuthenticationToken);
  }

  // Replace watchdog timer with the normal cadence.
  ScheduleAuthValidate(kAuthValidatePollInterval);
}

void BraveAccountService::OnGetServiceToken(
    const std::string& expected_encrypted_authentication_token,
    const std::string& service_name,
    GetServiceTokenCallback callback,
    ServiceToken::Response response) {
  // Check if the authentication token is still the same as when we sent the
  // request. If the user logged out, logged out and back in, or switched
  // accounts while the request was in flight, don't cache or return the service
  // token as it belongs to a different (or no longer valid) authentication
  // session.
  if (const auto current_encrypted_authentication_token =
          pref_service_->GetString(prefs::kBraveAccountAuthenticationToken);
      current_encrypted_authentication_token !=
      expected_encrypted_authentication_token) {
    return std::move(callback).Run(
        base::unexpected(mojom::GetServiceTokenError::New(
            std::nullopt,
            mojom::GetServiceTokenErrorCode::kAuthenticationSessionChanged)));
  }

  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(mojom::GetServiceTokenError::New(
            response.status_code.value_or(response.net_error), std::nullopt)));
  }

  CHECK(response.status_code);
  const auto status_code = *response.status_code;

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody              ]> ==>
          // expected<SuccessBody, [GetServiceTokenErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeMojomError<mojom::GetServiceTokenError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody             ], GetServiceTokenErrorPtr> ==>
          // expected<[GetServiceTokenResultPtr], GetServiceTokenErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::GetServiceTokenResultPtr,
                                          mojom::GetServiceTokenErrorPtr> {
            if (success_body.auth_token.empty()) {
              return base::unexpected(
                  mojom::GetServiceTokenError::New(status_code, std::nullopt));
            }

            auto encrypted_service_token = Encrypt(success_body.auth_token);
            if (encrypted_service_token.empty()) {
              return base::unexpected(mojom::GetServiceTokenError::New(
                  std::nullopt, mojom::GetServiceTokenErrorCode::
                                    kServiceTokenEncryptionFailed));
            }

            auto service_tokens =
                pref_service_->GetDict(prefs::kBraveAccountServiceTokens)
                    .Clone();
            service_tokens.Set(service_name,
                               base::Value::Dict()
                                   .Set(prefs::keys::kServiceToken,
                                        std::move(encrypted_service_token))
                                   .Set(prefs::keys::kLastFetched,
                                        base::TimeToValue(base::Time::Now())));

            pref_service_->SetDict(prefs::kBraveAccountServiceTokens,
                                   std::move(service_tokens));

            return mojom::GetServiceTokenResult::New(
                std::move(success_body.auth_token));
          });

  std::move(callback).Run(std::move(result));
}

std::string BraveAccountService::GetCachedServiceToken(
    const std::string& service_name) const {
  const auto* service =
      pref_service_->GetDict(prefs::kBraveAccountServiceTokens)
          .FindDict(service_name);
  if (!service) {
    return "";
  }

  const auto* encrypted_service_token =
      service->FindString(prefs::keys::kServiceToken);
  const auto* last_fetched_value = service->Find(prefs::keys::kLastFetched);

  if (!encrypted_service_token || !last_fetched_value) {
    return "";
  }

  const auto last_fetched_time = base::ValueToTime(*last_fetched_value);
  if (!last_fetched_time) {
    return "";
  }

  if (base::Time::Now() - *last_fetched_time >= kServiceTokenMaxAge) {
    return "";
  }

  return Decrypt(*encrypted_service_token);
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

}  // namespace brave_account
