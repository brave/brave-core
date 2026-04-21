/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoints/auth_validate.h"
#include "brave/components/brave_account/endpoints/login_finalize.h"
#include "brave/components/brave_account/endpoints/login_init.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/endpoints/service_token.h"
#include "brave/components/brave_account/endpoints/verify_complete.h"
#include "brave/components/brave_account/endpoints/verify_resend.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/pref_member.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class OSCryptAsync;
}  // namespace os_crypt_async

namespace brave_account {

// BraveAccountService has no non-Mojom callers. Its only public entrypoint is
// `BindInterface()`, and this should remain the case. Receiver binding is
// deferred until `FinishInitialization()` installs the encryptor, and any
// service-initiated work that can reach `Encrypt()`/`Decrypt()` is also
// started only after that point.
class BraveAccountService : public KeyedService, public mojom::Authentication {
 public:
  using OSCryptCallback =
      base::RepeatingCallback<bool(const std::string&, std::string*)>;

  BraveAccountService(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      os_crypt_async::OSCryptAsync* os_crypt_async);

  BraveAccountService(const BraveAccountService&) = delete;
  BraveAccountService& operator=(const BraveAccountService&) = delete;

  ~BraveAccountService() override;

  static void SetOSCryptCallbacksForTesting(OSCryptCallback encrypt_callback,
                                            OSCryptCallback decrypt_callback);

  void BindInterface(
      mojo::PendingReceiver<mojom::Authentication> pending_receiver);

 private:
  template <typename TestCase>
  friend class BraveAccountServiceTest;

  void FinishInitialization(os_crypt_async::Encryptor encryptor);

  void AddObserver(
      mojo::PendingRemote<mojom::AuthenticationObserver> observer) override;

  void RegisterInitialize(mojom::Service initiating_service,
                          const std::string& email,
                          const std::string& blinded_message,
                          RegisterInitializeCallback callback) override;

  void RegisterFinalize(const std::string& encrypted_verification_token,
                        const std::string& serialized_record,
                        RegisterFinalizeCallback callback) override;

  void RegisterVerify(const std::string& code,
                      RegisterVerifyCallback callback) override;

  void ResendConfirmationEmail(
      ResendConfirmationEmailCallback callback) override;

  void CancelRegistration() override;

  void LoginInitialize(mojom::Service initiating_service,
                       const std::string& email,
                       const std::string& serialized_ke1,
                       LoginInitializeCallback callback) override;

  void LoginFinalize(const std::string& encrypted_login_token,
                     const std::string& client_mac,
                     LoginFinalizeCallback callback) override;

  void LogOut() override;

  void GetServiceToken(mojom::Service service,
                       GetServiceTokenCallback callback) override;

  void OnRegisterInitialize(RegisterInitializeCallback callback,
                            endpoints::PasswordInit::Response response);

  void OnRegisterFinalize(RegisterFinalizeCallback callback,
                          const std::string& encrypted_verification_token,
                          endpoints::PasswordFinalize::Response response);

  void OnRegisterVerify(RegisterVerifyCallback callback,
                        endpoints::VerifyComplete::Response response);

  void OnResendConfirmationEmail(ResendConfirmationEmailCallback callback,
                                 endpoints::VerifyResend::Response response);

  void OnVerificationTokenChanged();

  void OnLoginInitialize(LoginInitializeCallback callback,
                         endpoints::LoginInit::Response response);

  void OnLoginFinalize(LoginFinalizeCallback callback,
                       endpoints::LoginFinalize::Response response);

  void OnAuthenticationTokenChanged();

  void ScheduleAuthValidate(
      base::TimeDelta delay = base::Seconds(0),
      endpoint_client::RequestHandle current_auth_validate_request = {});

  void AuthValidate(
      endpoint_client::RequestHandle current_auth_validate_request);

  void OnAuthValidate(endpoints::AuthValidate::Response response);

  void OnEmailAddressChanged();

  void NotifyObservers();

  mojom::AccountStatePtr GetAccountState() const;

  void OnGetServiceToken(
      const std::string& expected_encrypted_authentication_token,
      const std::string& service_name,
      GetServiceTokenCallback callback,
      endpoints::ServiceToken::Response response);

  std::string GetCachedServiceToken(const std::string& service_name) const;

  std::string Encrypt(const std::string& plain_text) const;

  std::string Decrypt(const std::string& base64) const;

  const raw_ptr<PrefService> pref_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::optional<os_crypt_async::Encryptor> encryptor_;
  std::vector<mojo::PendingReceiver<mojom::Authentication>> pending_receivers_;
  mojo::ReceiverSet<mojom::Authentication> authentication_receivers_;
  mojo::RemoteSet<mojom::AuthenticationObserver> observers_;
  StringPrefMember pref_verification_token_;
  StringPrefMember pref_authentication_token_;
  StringPrefMember pref_email_address_;
  base::OneShotTimer auth_validate_timer_;
  base::WeakPtrFactory<BraveAccountService> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
