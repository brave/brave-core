/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoints/error.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/endpoints/verify_result.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

class BraveAccountService : public KeyedService, public mojom::Authentication {
 public:
  using OSCryptCallback =
      base::RepeatingCallback<bool(const std::string&, std::string*)>;

  BraveAccountService(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  BraveAccountService(const BraveAccountService&) = delete;
  BraveAccountService& operator=(const BraveAccountService&) = delete;

  ~BraveAccountService() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::Authentication> pending_receiver);

 private:
  template <typename TestCase>
  friend class BraveAccountServiceTest;

  // Provides dependency injection for testing.
  BraveAccountService(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      OSCryptCallback encrypt_callback,
      OSCryptCallback decrypt_callback,
      std::unique_ptr<base::OneShotTimer> verify_result_timer);

  std::string Encrypt(const std::string& plain_text) const;

  std::string Decrypt(const std::string& base64) const;

  void RegisterInitialize(const std::string& email,
                          const std::string& blinded_message,
                          RegisterInitializeCallback callback) override;

  void RegisterFinalize(const std::string& encrypted_verification_token,
                        const std::string& serialized_record,
                        RegisterFinalizeCallback callback) override;

  void ResendConfirmationEmail() override;

  void CancelRegistration() override;

  void LogOut() override;

  void OnRegisterInitialize(
      RegisterInitializeCallback callback,
      int response_code,
      base::expected<std::optional<endpoints::PasswordInit::Response>,
                     std::optional<endpoints::PasswordInit::Error>> reply);

  void OnRegisterFinalize(
      RegisterFinalizeCallback callback,
      const std::string& encrypted_verification_token,
      int response_code,
      base::expected<std::optional<endpoints::PasswordFinalize::Response>,
                     std::optional<endpoints::PasswordFinalize::Error>> reply);

  std::optional<mojom::RegisterErrorCode> TransformError(
      std::optional<endpoints::Error> error);

  void OnVerificationTokenChanged();

  void ScheduleVerifyResult(
      base::TimeDelta delay = base::Seconds(0),
      endpoint_client::RequestHandle current_verify_result_request = {});

  void VerifyResult(
      endpoint_client::RequestHandle current_verify_result_request);

  void OnVerifyResult(
      int response_code,
      base::expected<std::optional<endpoints::VerifyResult::Response>,
                     std::optional<endpoints::VerifyResult::Error>> reply);

  const raw_ptr<PrefService> pref_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  OSCryptCallback encrypt_callback_;
  OSCryptCallback decrypt_callback_;
  mojo::ReceiverSet<mojom::Authentication> authentication_receivers_;
  PrefChangeRegistrar pref_change_registrar_;
  std::unique_ptr<base::OneShotTimer> verify_result_timer_;
  base::WeakPtrFactory<BraveAccountService> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
