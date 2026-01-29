// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_TEST_UTILS_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_TEST_UTILS_H_

#include <memory>

#include "brave/components/email_aliases/email_aliases.mojom.h"
#include "brave/components/email_aliases/email_aliases_auth.h"
#include "brave/components/email_aliases/email_aliases_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace email_aliases::test {

// Test observer for authentication state changes
class AuthStateObserver
    : public email_aliases::mojom::EmailAliasesServiceObserver {
 public:
  ~AuthStateObserver() override;

  static std::unique_ptr<AuthStateObserver> Setup(
      EmailAliasesService* service,
      bool wait_initialized = false);

  const mojom::AuthState& GetStatus() const;
  [[nodiscard]] bool WaitFor(mojom::AuthenticationStatus status);
  [[nodiscard]] bool WaitInitialized();

 private:
  explicit AuthStateObserver(
      mojo::PendingReceiver<email_aliases::mojom::EmailAliasesServiceObserver>
          pending);

  void OnAuthStateChanged(email_aliases::mojom::AuthStatePtr state) override;
  void OnAliasesUpdated(std::vector<email_aliases::mojom::AliasPtr>) override;

  mojom::AuthStatePtr last_status_ =
      mojom::AuthState::New(mojom::AuthenticationStatus::kStartup,
                            std::string());

  mojo::Receiver<email_aliases::mojom::EmailAliasesServiceObserver> receiver_{
      this};
};

class MockBraveAccountAuthentication
    : public brave_account::mojom::Authentication {
 public:
  MockBraveAccountAuthentication();
  ~MockBraveAccountAuthentication() override;

  MOCK_METHOD(void,
              RegisterInitialize,
              (const std::string& email,
               const std::string& blinded_message,
               brave_account::mojom::Authentication::RegisterInitializeCallback
                   callback),
              (override));

  MOCK_METHOD(
      void,
      RegisterFinalize,
      (const std::string& encrypted_verification_token,
       const std::string& serialized_record,
       brave_account::mojom::Authentication::RegisterFinalizeCallback callback),
      (override));

  MOCK_METHOD(
      void,
      ResendConfirmationEmail,
      (brave_account::mojom::Authentication::ResendConfirmationEmailCallback
           callback),
      (override));

  MOCK_METHOD(void, CancelRegistration, (), (override));

  MOCK_METHOD(void,
              LoginInitialize,
              (const std::string& email,
               const std::string& serialized_ke1,
               LoginInitializeCallback callback),
              (override));

  MOCK_METHOD(void,
              LoginFinalize,
              (const std::string& encrypted_login_token,
               const std::string& client_mac,
               LoginFinalizeCallback callback),
              (override));

  MOCK_METHOD(void, LogOut, (), (override));

  MOCK_METHOD(void,
              GetServiceToken,
              (brave_account::mojom::Service service,
               GetServiceTokenCallback callback),
              (override));
};

GURL GetEmailAliasesServiceURL();

}  // namespace email_aliases::test

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_TEST_UTILS_H_
