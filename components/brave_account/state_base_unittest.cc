/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/state_base.h"

#include <memory>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoint_client/test_support.h"
#include "brave/components/brave_account/prefs.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_account {

namespace {

struct TestEndpoint {
  using Request = endpoint_client::POST<endpoint_client::JSONRequestBody>;
  using Response = endpoint_client::Response<endpoint_client::JSONSuccessBody,
                                             endpoint_client::JSONErrorBody>;
  static GURL URL() { return GURL("https://example.com/api/query"); }
};

class TestState : public StateBase {
 public:
  TestState(AccountStatePrefs& account_state_prefs,
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
            const os_crypt_async::Encryptor& encryptor,
            AddObserverCallback add_observer)
      : StateBase(account_state_prefs,
                  std::move(url_loader_factory),
                  encryptor,
                  std::move(add_observer)) {}

  using StateBase::SendStateOwnedRequest;
};

class StateBaseTest : public testing::Test {
 protected:
  void SetUp() override {
    prefs::RegisterPrefs(pref_service_.registry());
    account_state_prefs_ = std::make_unique<AccountStatePrefs>(pref_service_);
    encryptor_ = os_crypt_async::GetTestEncryptorForTesting();
    state_ = std::make_unique<TestState>(
        *account_state_prefs_, test_url_loader_factory_.GetSafeWeakWrapper(),
        *encryptor_, base::DoNothing());
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<AccountStatePrefs> account_state_prefs_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  scoped_refptr<os_crypt_async::Encryptor> encryptor_;
  std::unique_ptr<TestState> state_;
};

}  // namespace

// Happy path: the user callback fires with the response.
// Smoke test that the `.Then(RemoveRequestHandle)` continuation doesn't
// break delivery.
TEST_F(StateBaseTest, StateOwnedRequest_DeliversResponse) {
  endpoint_client::MockResponseFor<TestEndpoint>(test_url_loader_factory_,
                                                 TestEndpoint::Response{
                                                     .net_error = net::OK,
                                                 });
  base::test::TestFuture<TestEndpoint::Response> future;
  state_->SendStateOwnedRequest<TestEndpoint>(TestEndpoint::Request(),
                                              future.GetCallback());
  EXPECT_TRUE(future.Wait());
}

// `~StateBase` must cancel still-pending state-owned requests:
// no response is registered, so the loader hangs until the state is
// destroyed, and the user callback must never run.
TEST_F(StateBaseTest, StateOwnedRequest_DestructionCancelsPendingRequest) {
  base::test::TestFuture<TestEndpoint::Response> future;
  state_->SendStateOwnedRequest<TestEndpoint>(TestEndpoint::Request(),
                                              future.GetCallback());
  state_.reset();
  base::RunLoop run_loop;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();
  EXPECT_FALSE(future.IsReady());
}

}  // namespace brave_account
