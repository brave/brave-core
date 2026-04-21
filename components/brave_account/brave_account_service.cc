/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <concepts>
#include <type_traits>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/check_deref.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/values_util.h"
#include "base/no_destructor.h"
#include "base/notimplemented.h"
#include "base/strings/strcat.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/brave_account_utils.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/endpoints/auth_logout.h"
#include "brave/components/brave_account/endpoints/error_body.h"
#include "brave/components/brave_account/endpoints/verify_delete.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
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
using endpoints::AuthLogout;
using endpoints::AuthValidate;
using endpoints::ErrorBody;
using endpoints::LoginFinalize;
using endpoints::LoginInit;
using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::ServiceToken;
using endpoints::VerifyComplete;
using endpoints::VerifyDelete;
using endpoints::VerifyResend;

namespace {

BraveAccountService::OSCryptCallback& EncryptCallbackForTesting() {
  static base::NoDestructor<BraveAccountService::OSCryptCallback> callback;
  return *callback;
}

BraveAccountService::OSCryptCallback& DecryptCallbackForTesting() {
  static base::NoDestructor<BraveAccountService::OSCryptCallback> callback;
  return *callback;
}

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

template <typename Error>
using ClientErrorOf = typename std::remove_cvref_t<
    decltype(std::declval<Error>().get_client_error())>::element_type;

template <typename Error>
using ClientErrorCodeOf = decltype(ClientErrorOf<Error>::error_code);

template <typename Error>
using ServerErrorOf = typename std::remove_cvref_t<
    decltype(std::declval<Error>().get_server_error())>::element_type;

template <typename Error>
using ServerErrorCodeOf = decltype(ServerErrorOf<Error>::error_code);

template <typename Error, typename ClientErrorCode>
  requires std::same_as<ClientErrorCode, ClientErrorCodeOf<Error>>
auto MakeClientError(ClientErrorCode client_error_code) {
  return Error::NewClientError(ClientErrorOf<Error>::New(client_error_code));
}

template <typename ServerErrorCode>
auto MakeServerErrorCode(ErrorBody error_body) {
  if (error_body.code.is_none()) {
    return ServerErrorCode::kNull;
  } else if (error_body.code.is_int()) {
    if (const auto error_code =
            static_cast<ServerErrorCode>(error_body.code.GetInt());
        mojom::IsKnownEnumValue(error_code)) {
      return error_code;
    }
  }

  return ServerErrorCode::kUnknown;
}

template <typename Error, typename ServerErrorCode>
  requires std::same_as<ServerErrorCode, ServerErrorCodeOf<Error>>
auto MakeServerError(int net_error_or_http_status,
                     ServerErrorCode server_error_code) {
  return Error::NewServerError(
      ServerErrorOf<Error>::New(net_error_or_http_status, server_error_code));
}

template <typename Error>
auto MakeServerError(int net_error_or_http_status, ErrorBody error_body) {
  return MakeServerError<Error>(
      net_error_or_http_status,
      MakeServerErrorCode<ServerErrorCodeOf<Error>>(std::move(error_body)));
}

}  // namespace

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    os_crypt_async::OSCryptAsync* os_crypt_async)
    : pref_service_(pref_service),
      url_loader_factory_(std::move(url_loader_factory)) {
  CHECK(pref_service_);
  CHECK(url_loader_factory_);

  CHECK_DEREF(os_crypt_async)
      .GetInstance(base::BindOnce(&BraveAccountService::FinishInitialization,
                                  weak_factory_.GetWeakPtr()));
}

BraveAccountService::~BraveAccountService() = default;

// static
void BraveAccountService::SetOSCryptCallbacksForTesting(  // IN-TEST
    OSCryptCallback encrypt_callback,
    OSCryptCallback decrypt_callback) {
  EncryptCallbackForTesting() = std::move(encrypt_callback);  // IN-TEST
  DecryptCallbackForTesting() = std::move(decrypt_callback);  // IN-TEST
}

void BraveAccountService::BindInterface(
    mojo::PendingReceiver<mojom::Authentication> pending_receiver) {
  if (!encryptor_) {
    return pending_receivers_.push_back(std::move(pending_receiver));
  }
  authentication_receivers_.Add(this, std::move(pending_receiver));
}

void BraveAccountService::FinishInitialization(
    os_crypt_async::Encryptor encryptor) {
  encryptor_ = std::move(encryptor);

  pref_verification_token_.Init(
      prefs::kBraveAccountVerificationToken, pref_service_,
      base::BindRepeating(&BraveAccountService::OnVerificationTokenChanged,
                          base::Unretained(this)));

  pref_authentication_token_.Init(
      prefs::kBraveAccountAuthenticationToken, pref_service_,
      base::BindRepeating(&BraveAccountService::OnAuthenticationTokenChanged,
                          base::Unretained(this)));
  OnAuthenticationTokenChanged();

  pref_email_address_.Init(
      prefs::kBraveAccountEmailAddress, pref_service_,
      base::BindRepeating(&BraveAccountService::OnEmailAddressChanged,
                          base::Unretained(this)));

  for (auto& pending_receiver : pending_receivers_) {
    authentication_receivers_.Add(this, std::move(pending_receiver));
  }
  decltype(pending_receivers_)().swap(pending_receivers_);
}

void BraveAccountService::AddObserver(
    mojo::PendingRemote<mojom::AuthenticationObserver> observer) {
  const auto observer_id = observers_.Add(std::move(observer));
  CHECK_DEREF(observers_.Get(observer_id))
      .OnAccountStateChanged(GetAccountState());
}

void BraveAccountService::RegisterInitialize(
    mojom::Service initiating_service,
    const std::string& email,
    const std::string& blinded_message,
    RegisterInitializeCallback callback) {
  CHECK(!email.empty());
  CHECK(!blinded_message.empty());

  auto request = MakeRequest<PasswordInit::Request>();
  request.body.blinded_message = blinded_message;
  request.body.initiating_service_name =
      kServiceToString.at(initiating_service);
  request.body.new_account_email = email;
  request.body.serialize_response = true;
  Client<PasswordInit>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnRegisterInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::RegisterFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    RegisterFinalizeCallback callback) {
  CHECK(!encrypted_verification_token.empty());
  CHECK(!serialized_record.empty());

  const std::string verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(base::unexpected(MakeClientError<
                                                    mojom::RegisterError>(
        mojom::RegisterClientErrorCode::kVerificationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<PasswordFinalize::Request>>();
  SetBearerToken(request, verification_token);
  request.body.serialized_record = serialized_record;
  Client<PasswordFinalize>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     encrypted_verification_token));
}

void BraveAccountService::RegisterVerify(const std::string& code,
                                         RegisterVerifyCallback callback) {
  CHECK(!code.empty());

  const auto encrypted_verification_token =
      pref_service_->GetString(prefs::kBraveAccountVerificationToken);
  if (encrypted_verification_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::RegisterError>(
            mojom::RegisterClientErrorCode::kUserNotInTheVerificationState)));
  }

  const auto verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(base::unexpected(MakeClientError<
                                                    mojom::RegisterError>(
        mojom::RegisterClientErrorCode::kVerificationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<VerifyComplete::Request>>();
  SetBearerToken(request, verification_token);
  request.body.code = code;
  Client<VerifyComplete>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnRegisterVerify,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::ResendConfirmationEmail(
    ResendConfirmationEmailCallback callback) {
  const auto encrypted_verification_token =
      pref_service_->GetString(prefs::kBraveAccountVerificationToken);
  if (encrypted_verification_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::ResendConfirmationEmailError>(
            mojom::ResendConfirmationEmailClientErrorCode::
                kUserNotInTheVerificationState)));
  }

  const auto verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::ResendConfirmationEmailError>(
            mojom::ResendConfirmationEmailClientErrorCode::
                kVerificationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<VerifyResend::Request>>();
  SetBearerToken(request, verification_token);
  // Server side will determine locale based on the Accept-Language request
  // header (which is included automatically by upstream).
  request.body.locale = "";
  request.timeout_duration = kVerifyResendTimeout;
  Client<VerifyResend>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnResendConfirmationEmail,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::CancelRegistration() {
  const auto encrypted_verification_token =
      pref_service_->GetString(prefs::kBraveAccountVerificationToken);

  pref_service_->ClearPref(prefs::kBraveAccountVerificationToken);

  const auto verification_token = Decrypt(encrypted_verification_token);
  if (verification_token.empty()) {
    return;
  }

  // Best-effort notification to the server, since server side will clean up
  // verification tokens automatically (currently after 30 minutes).
  auto request = MakeRequest<WithHeaders<VerifyDelete::Request>>();
  SetBearerToken(request, verification_token);
  Client<VerifyDelete>::Send(url_loader_factory_, std::move(request),
                             base::BindOnce([](VerifyDelete::Response) {}));
}

void BraveAccountService::LoginInitialize(mojom::Service initiating_service,
                                          const std::string& email,
                                          const std::string& serialized_ke1,
                                          LoginInitializeCallback callback) {
  CHECK(!email.empty());
  CHECK(!serialized_ke1.empty());

  auto request = MakeRequest<LoginInit::Request>();
  request.body.email = email;
  request.body.initiating_service_name =
      kServiceToString.at(initiating_service);
  request.body.serialized_ke1 = serialized_ke1;
  Client<LoginInit>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnLoginInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::LoginFinalize(
    const std::string& encrypted_login_token,
    const std::string& client_mac,
    LoginFinalizeCallback callback) {
  CHECK(!encrypted_login_token.empty());
  CHECK(!client_mac.empty());

  const std::string login_token = Decrypt(encrypted_login_token);
  if (login_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::LoginError>(
            mojom::LoginClientErrorCode::kLoginTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<LoginFinalize::Request>>();
  SetBearerToken(request, login_token);
  request.body.client_mac = client_mac;
  Client<endpoints::LoginFinalize>::Send(
      url_loader_factory_, std::move(request),
      base::BindOnce(&BraveAccountService::OnLoginFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::LogOut() {
  const auto encrypted_authentication_token =
      pref_service_->GetString(prefs::kBraveAccountAuthenticationToken);

  pref_service_->ClearPref(prefs::kBraveAccountAuthenticationToken);

  const auto authentication_token = Decrypt(encrypted_authentication_token);
  if (authentication_token.empty()) {
    return;
  }

  // Best-effort notification to the server, since server side will clean up
  // authentication tokens automatically (currently in 6 months of inactivity).
  auto request = MakeRequest<WithHeaders<AuthLogout::Request>>();
  SetBearerToken(request, authentication_token);
  Client<AuthLogout>::Send(url_loader_factory_, std::move(request),
                           base::BindOnce([](AuthLogout::Response) {}));
}

void BraveAccountService::GetServiceToken(mojom::Service service,
                                          GetServiceTokenCallback callback) {
  CHECK(service != mojom::Service::kAccounts);
  std::string service_name(kServiceToString.at(service));
  if (auto service_token = GetCachedServiceToken(service_name);
      !service_token.empty()) {
    return std::move(callback).Run(
        mojom::GetServiceTokenResult::New(std::move(service_token)));
  }

  auto encrypted_authentication_token =
      pref_service_->GetString(prefs::kBraveAccountAuthenticationToken);
  if (encrypted_authentication_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::GetServiceTokenError>(
            mojom::GetServiceTokenClientErrorCode::kUserNotLoggedIn)));
  }

  const auto authentication_token = Decrypt(encrypted_authentication_token);
  if (authentication_token.empty()) {
    return std::move(callback).Run(
        base::unexpected(MakeClientError<mojom::GetServiceTokenError>(
            mojom::GetServiceTokenClientErrorCode::
                kAuthenticationTokenDecryptionFailed)));
  }

  auto request = MakeRequest<WithHeaders<ServiceToken::Request>>();
  SetBearerToken(request, authentication_token);
  request.body.service = service_name;
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
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::RegisterError>(
            response.status_code.value_or(response.net_error),
            mojom::RegisterServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [RegisterErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::RegisterError>(status_code,
                                                         std::move(error_body));
          })
          // expected<[SuccessBody                ], RegisterErrorPtr> ==>
          // expected<[RegisterInitializeResultPtr], RegisterErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::RegisterInitializeResultPtr,
                                          mojom::RegisterErrorPtr> {
            if (success_body.verification_token.empty() ||
                success_body.serialized_response.empty()) {
              return base::unexpected(MakeServerError<mojom::RegisterError>(
                  status_code,
                  mojom::RegisterServerErrorCode::kInvalidResponse));
            }

            std::string encrypted_verification_token =
                Encrypt(success_body.verification_token);
            if (encrypted_verification_token.empty()) {
              return base::unexpected(MakeClientError<mojom::RegisterError>(
                  mojom::RegisterClientErrorCode::
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
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::RegisterError>(
            response.status_code.value_or(response.net_error),
            mojom::RegisterServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [RegisterErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::RegisterError>(status_code,
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

void BraveAccountService::OnRegisterVerify(RegisterVerifyCallback callback,
                                           VerifyComplete::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::RegisterError>(
            response.status_code.value_or(response.net_error),
            mojom::RegisterServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody       ]> ==>
          // expected<SuccessBody, [RegisterErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::RegisterError>(status_code,
                                                         std::move(error_body));
          })
          // expected<[SuccessBody            ], RegisterErrorPtr> ==>
          // expected<[RegisterVerifyResultPtr], RegisterErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::RegisterVerifyResultPtr,
                                          mojom::RegisterErrorPtr> {
            if (success_body.auth_token.empty() || success_body.email.empty()) {
              return base::unexpected(MakeServerError<mojom::RegisterError>(
                  status_code,
                  mojom::RegisterServerErrorCode::kInvalidResponse));
            }

            const std::string encrypted_authentication_token =
                Encrypt(success_body.auth_token);
            if (encrypted_authentication_token.empty()) {
              return base::unexpected(MakeClientError<mojom::RegisterError>(
                  mojom::RegisterClientErrorCode::
                      kAuthenticationTokenEncryptionFailed));
            }

            pref_service_->SetString(prefs::kBraveAccountEmailAddress,
                                     success_body.email);
            pref_service_->SetString(prefs::kBraveAccountAuthenticationToken,
                                     encrypted_authentication_token);
            pref_service_->ClearPref(prefs::kBraveAccountVerificationToken);

            return mojom::RegisterVerifyResult::New();
          });

  std::move(callback).Run(std::move(result));
}

void BraveAccountService::OnResendConfirmationEmail(
    ResendConfirmationEmailCallback callback,
    VerifyResend::Response response) {
  if (response.status_code == net::HTTP_NO_CONTENT) {
    return std::move(callback).Run(mojom::ResendConfirmationEmailResult::New());
  }

  if (!response.body || response.body->has_value()) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ResendConfirmationEmailError>(
            response.status_code.value_or(response.net_error),
            mojom::ResendConfirmationEmailServerErrorCode::kInvalidResponse)));
  }

  std::move(callback).Run(
      base::unexpected(MakeServerError<mojom::ResendConfirmationEmailError>(
          CHECK_DEREF(response.status_code),
          std::move(response.body->error()))));
}

void BraveAccountService::OnVerificationTokenChanged() {
  NotifyObservers();
}

void BraveAccountService::OnLoginInitialize(LoginInitializeCallback callback,
                                            LoginInit::Response response) {
  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::LoginError>(
            response.status_code.value_or(response.net_error),
            mojom::LoginServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody    ]> ==>
          // expected<SuccessBody, [LoginErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::LoginError>(status_code,
                                                      std::move(error_body));
          })
          // expected<[SuccessBody             ], LoginErrorPtr> ==>
          // expected<[LoginInitializeResultPtr], LoginErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::LoginInitializeResultPtr,
                                          mojom::LoginErrorPtr> {
            if (success_body.login_token.empty() ||
                success_body.serialized_ke2.empty()) {
              return base::unexpected(MakeServerError<mojom::LoginError>(
                  status_code, mojom::LoginServerErrorCode::kInvalidResponse));
            }

            std::string encrypted_login_token =
                Encrypt(success_body.login_token);
            if (encrypted_login_token.empty()) {
              return base::unexpected(MakeClientError<mojom::LoginError>(
                  mojom::LoginClientErrorCode::kLoginTokenEncryptionFailed));
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
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::LoginError>(
            response.status_code.value_or(response.net_error),
            mojom::LoginServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody    ]> ==>
          // expected<SuccessBody, [LoginErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::LoginError>(status_code,
                                                      std::move(error_body));
          })
          // expected<[SuccessBody           ], LoginErrorPtr> ==>
          // expected<[LoginFinalizeResultPtr], LoginErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::LoginFinalizeResultPtr,
                                          mojom::LoginErrorPtr> {
            if (success_body.auth_token.empty() || success_body.email.empty()) {
              return base::unexpected(MakeServerError<mojom::LoginError>(
                  status_code, mojom::LoginServerErrorCode::kInvalidResponse));
            }

            const std::string encrypted_authentication_token =
                Encrypt(success_body.auth_token);
            if (encrypted_authentication_token.empty()) {
              return base::unexpected(MakeClientError<mojom::LoginError>(
                  mojom::LoginClientErrorCode::
                      kAuthenticationTokenEncryptionFailed));
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
  NotifyObservers();

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
  } else if (response.status_code >= 400 && response.status_code < 500) {
    // Clear the auth token (and stop polling) to prevent
    // presenting invalid state to the user and issuing invalid requests.
    return pref_service_->ClearPref(prefs::kBraveAccountAuthenticationToken);
  }

  // Replace watchdog timer with the normal cadence.
  ScheduleAuthValidate(kAuthValidatePollInterval);
}

void BraveAccountService::OnEmailAddressChanged() {
  // Only notify observers if logged in, since the email is only relevant in
  // the LoggedIn state.
  if (!pref_authentication_token_.GetValue().empty()) {
    NotifyObservers();
  }
}

void BraveAccountService::NotifyObservers() {
  const auto state = GetAccountState();
  for (auto& observer : observers_) {
    observer->OnAccountStateChanged(state.Clone());
  }
}

mojom::AccountStatePtr BraveAccountService::GetAccountState() const {
  if (!pref_service_->GetString(prefs::kBraveAccountAuthenticationToken)
           .empty()) {
    std::string email =
        pref_service_->GetString(prefs::kBraveAccountEmailAddress);
    CHECK(!email.empty());
    return mojom::AccountState::NewLoggedIn(
        mojom::LoggedInState::New(std::move(email)));
  }

  if (!pref_service_->GetString(prefs::kBraveAccountVerificationToken)
           .empty()) {
    return mojom::AccountState::NewVerification(
        mojom::VerificationState::New());
  }

  return mojom::AccountState::NewLoggedOut(mojom::LoggedOutState::New());
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
        base::unexpected(MakeClientError<mojom::GetServiceTokenError>(
            mojom::GetServiceTokenClientErrorCode::
                kAuthenticationSessionChanged)));
  }

  if (!response.body) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::GetServiceTokenError>(
            response.status_code.value_or(response.net_error),
            mojom::GetServiceTokenServerErrorCode::kInvalidResponse)));
  }

  const auto status_code = CHECK_DEREF(response.status_code);

  auto result =
      std::move(*response.body)
          // expected<SuccessBody, [ErrorBody              ]> ==>
          // expected<SuccessBody, [GetServiceTokenErrorPtr]>
          .transform_error([&](auto error_body) {
            return MakeServerError<mojom::GetServiceTokenError>(
                status_code, std::move(error_body));
          })
          // expected<[SuccessBody             ], GetServiceTokenErrorPtr> ==>
          // expected<[GetServiceTokenResultPtr], GetServiceTokenErrorPtr>
          .and_then([&](auto success_body)
                        -> base::expected<mojom::GetServiceTokenResultPtr,
                                          mojom::GetServiceTokenErrorPtr> {
            if (success_body.auth_token.empty()) {
              return base::unexpected(
                  MakeServerError<mojom::GetServiceTokenError>(
                      status_code,
                      mojom::GetServiceTokenServerErrorCode::kInvalidResponse));
            }

            auto encrypted_service_token = Encrypt(success_body.auth_token);
            if (encrypted_service_token.empty()) {
              return base::unexpected(
                  MakeClientError<mojom::GetServiceTokenError>(
                      mojom::GetServiceTokenClientErrorCode::
                          kServiceTokenEncryptionFailed));
            }

            auto service_tokens =
                pref_service_->GetDict(prefs::kBraveAccountServiceTokens)
                    .Clone();
            service_tokens.Set(service_name,
                               base::DictValue()
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
  if (const auto& callback = EncryptCallbackForTesting()) {
    CHECK_IS_TEST();
    if (!callback.Run(plain_text, &encrypted)) {
      return std::string();
    }
  } else {
    if (!CHECK_DEREF(encryptor_).EncryptString(plain_text, &encrypted)) {
      return std::string();
    }
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
  if (const auto& callback = DecryptCallbackForTesting()) {
    CHECK_IS_TEST();
    if (!callback.Run(encrypted, &plain_text)) {
      return std::string();
    }
  } else {
    if (!CHECK_DEREF(encryptor_).DecryptString(encrypted, &plain_text)) {
      return std::string();
    }
  }

  return plain_text;
}

}  // namespace brave_account
