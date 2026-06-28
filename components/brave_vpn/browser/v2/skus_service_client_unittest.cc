/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"

#include <string>

#include "base/functional/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/skus/browser/test/fake_skus_service.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {

class SkusServiceClientTest : public testing::Test {
 protected:
  mojo::PendingRemote<skus::mojom::SkusService> GetService() {
    ++bind_count_;
    return fake_service_.MakeRemote();
  }

  GetSkusServiceCallback ServiceGetter() {
    return base::BindRepeating(&SkusServiceClientTest::GetService,
                               base::Unretained(this));
  }

  base::test::TaskEnvironment task_environment_;
  skus::FakeSkusService fake_service_;
  int bind_count_ = 0;
  SkusServiceClient client_{ServiceGetter()};
};

// Constructing the client must not touch the getter; the remote is bound lazily
// on first use.
TEST_F(SkusServiceClientTest, DoesNotConnectUntilFirstCall) {
  EXPECT_EQ(bind_count_, 0);
}

// The configured response travels back out through the client untouched,
// proving the call reaches the service and the result is forwarded to the
// caller's callback.
TEST_F(SkusServiceClientTest, GetCredentialSummaryReturnsConfiguredResponse) {
  fake_service_.SetCredentialSummaryResponse(R"({"active":true})");

  base::test::TestFuture<skus::mojom::SkusResultPtr> future;
  client_.GetCredentialSummary("vpn.brave.com", future.GetCallback());

  skus::mojom::SkusResultPtr result = future.Take();
  ASSERT_TRUE(result);
  EXPECT_EQ(result->code, skus::mojom::SkusResultCode::Ok);
  EXPECT_EQ(result->message, R"({"active":true})");
  EXPECT_EQ(bind_count_, 1);
}

TEST_F(SkusServiceClientTest, PrepareCredentialsPresentationCompletes) {
  base::test::TestFuture<skus::mojom::SkusResultPtr> future;
  client_.PrepareCredentialsPresentation("vpn.brave.com", "/credentials/path",
                                         future.GetCallback());

  skus::mojom::SkusResultPtr result = future.Take();
  ASSERT_TRUE(result);
  EXPECT_EQ(result->code, skus::mojom::SkusResultCode::Ok);
  EXPECT_EQ(bind_count_, 1);
}

// Repeated calls share a single bound remote.
TEST_F(SkusServiceClientTest, ReusesConnectionAcrossCalls) {
  {
    base::test::TestFuture<skus::mojom::SkusResultPtr> future;
    client_.GetCredentialSummary("vpn.brave.com", future.GetCallback());
    EXPECT_TRUE(future.Wait());
  }
  {
    base::test::TestFuture<skus::mojom::SkusResultPtr> future;
    client_.GetCredentialSummary("vpn.brave.com", future.GetCallback());
    EXPECT_TRUE(future.Wait());
  }
  EXPECT_EQ(bind_count_, 1);
}

// On disconnect the client eagerly re-binds (OnConnectionError ->
// EnsureConnected) and keeps serving requests on the new pipe.
TEST_F(SkusServiceClientTest, ReconnectsAfterDisconnect) {
  {
    base::test::TestFuture<skus::mojom::SkusResultPtr> future;
    client_.GetCredentialSummary("vpn.brave.com", future.GetCallback());
    EXPECT_TRUE(future.Wait());
  }
  EXPECT_EQ(bind_count_, 1);

  fake_service_.CloseConnection();

  // The disconnect handler fires asynchronously, so wait for rebind.
  EXPECT_TRUE(base::test::RunUntil([this]() { return bind_count_ == 2; }));
  {
    base::test::TestFuture<skus::mojom::SkusResultPtr> future;
    client_.GetCredentialSummary("vpn.brave.com", future.GetCallback());
    EXPECT_TRUE(future.Wait());
  }
  EXPECT_EQ(bind_count_, 2);
}

// Reset() drops the remote but, unlike a disconnect, does not eagerly rebind;
// the connection is re-established lazily on the next call.
TEST_F(SkusServiceClientTest, ResetForcesLazyReconnectOnNextCall) {
  {
    base::test::TestFuture<skus::mojom::SkusResultPtr> future;
    client_.GetCredentialSummary("vpn.brave.com", future.GetCallback());
    EXPECT_TRUE(future.Wait());
  }
  EXPECT_EQ(bind_count_, 1);
  client_.Reset();
  EXPECT_EQ(bind_count_, 1);
  {
    base::test::TestFuture<skus::mojom::SkusResultPtr> future;
    client_.GetCredentialSummary("vpn.brave.com", future.GetCallback());
    EXPECT_TRUE(future.Wait());
  }
  EXPECT_EQ(bind_count_, 2);
}

}  // namespace brave_vpn::v2
