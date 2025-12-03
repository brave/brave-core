// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/email_aliases/test_utils.h"

#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "gtest/gtest.h"

namespace email_aliases::test {

AuthStateObserver::AuthStateObserver(
    mojo::PendingReceiver<email_aliases::mojom::EmailAliasesServiceObserver>
        pending) {
  receiver_.Bind(std::move(pending));
}

AuthStateObserver::~AuthStateObserver() = default;

// static
std::unique_ptr<AuthStateObserver> AuthStateObserver::Setup(
    EmailAliasesService* service,
    bool wait_initialized) {
  mojo::PendingRemote<email_aliases::mojom::EmailAliasesServiceObserver> remote;
  auto observer = base::WrapUnique(
      new AuthStateObserver(remote.InitWithNewPipeAndPassReceiver()));
  service->AddObserver(std::move(remote));

  if (wait_initialized) {
    EXPECT_TRUE(observer->WaitInitialized());
  }
  return observer;
}

const mojom::AuthState& AuthStateObserver::GetStatus() const {
  return *last_status_;
}

[[nodiscard]] bool AuthStateObserver::WaitFor(
    mojom::AuthenticationStatus status) {
  return base::test::RunUntil(
      [status, this]() { return GetStatus().status == status; });
}

[[nodiscard]] bool AuthStateObserver::WaitInitialized() {
  return base::test::RunUntil([this]() {
    return GetStatus().status != mojom::AuthenticationStatus::kStartup;
  });
}

void AuthStateObserver::OnAuthStateChanged(
    email_aliases::mojom::AuthStatePtr state) {
  last_status_ = std::move(state);
}

void AuthStateObserver::OnAliasesUpdated(
    std::vector<email_aliases::mojom::AliasPtr>) {}

os_crypt_async::Encryptor GetEncryptor(os_crypt_async::OSCryptAsync* os_crypt) {
  base::test::TestFuture<os_crypt_async::Encryptor> result;
  os_crypt->GetInstance(result.GetCallback());
  return result.Take();
}

}  // namespace email_aliases::test
