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
                            std::string(),
                            std::nullopt);

  mojo::Receiver<email_aliases::mojom::EmailAliasesServiceObserver> receiver_{
      this};
};

os_crypt_async::Encryptor GetEncryptor(os_crypt_async::OSCryptAsync* os_crypt);

}  // namespace email_aliases::test

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_TEST_UTILS_H_
