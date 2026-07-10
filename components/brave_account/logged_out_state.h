/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_OUT_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_OUT_STATE_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/login_finalize.h"
#include "brave/components/brave_account/endpoints/login_init.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/register.h"
#include "brave/components/brave_account/reset_password.h"
#include "brave/components/brave_account/state_base.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

// `mojom::Authentication` surface available before login:
// `LoginInitialize()` and `LoginFinalize()`,
// plus the registration and password-reset steps,
// which are delegated to the `register_` and `reset_password_` helpers.
// `ResendVerificationEmail()` and `CancelVerification()` are fully
// handled by `StateBase` for both states.
// All other methods inherit `StateBase`'s wrong-state default.
class LoggedOutState : public StateBase {
 public:
  LoggedOutState(
      AccountStatePrefs& account_state_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const os_crypt_async::Encryptor& encryptor,
      AddObserverCallback add_observer);

  LoggedOutState(const LoggedOutState&) = delete;
  LoggedOutState& operator=(const LoggedOutState&) = delete;

  ~LoggedOutState() override;

 private:
  void RegisterPasswordInit(mojom::Service initiating_service,
                            const std::string& email,
                            const std::string& blinded_message,
                            RegisterPasswordInitCallback callback) override;

  void RegisterPasswordFinalize(
      const std::string& encrypted_verification_token,
      const std::string& serialized_record,
      RegisterPasswordFinalizeCallback callback) override;

  void RegisterVerifyComplete(const std::string& code,
                              RegisterVerifyCompleteCallback callback) override;

  void ResetPasswordVerifyInit(
      const std::string& email,
      ResetPasswordVerifyInitCallback callback) override;

  void ResetPasswordVerifyComplete(
      const std::string& code,
      ResetPasswordVerifyCompleteCallback callback) override;

  void ResetPasswordPasswordInit(
      const std::string& blinded_message,
      ResetPasswordPasswordInitCallback callback) override;

  void ResetPasswordPasswordFinalize(
      const std::string& serialized_record,
      const std::string& email,
      ResetPasswordPasswordFinalizeCallback callback) override;

  void LoginInitialize(mojom::Service initiating_service,
                       const std::string& email,
                       const std::string& serialized_ke1,
                       LoginInitializeCallback callback) override;

  void LoginFinalize(const std::string& encrypted_login_token,
                     const std::string& client_mac,
                     LoginFinalizeCallback callback) override;

  void OnLoginInitialize(LoginInitializeCallback callback,
                         endpoints::LoginInit::Response response);

  void OnLoginFinalize(LoginFinalizeCallback callback,
                       endpoints::LoginFinalize::Response response);

  Register register_{*this};
  ResetPassword reset_password_{*this};

  base::WeakPtrFactory<LoggedOutState> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_OUT_STATE_H_
