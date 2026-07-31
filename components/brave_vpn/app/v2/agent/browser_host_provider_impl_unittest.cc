/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_host_provider_impl.h"

#include <stdint.h>

#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/gtest_util.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {

// Fakes that own the browser's ends of the pipes, so the test can tell whether
// the forwarded handles arrived intact.
class FakeBrowserEndpointImpl : public mojom::BrowserEndpoint {
 public:
  ~FakeBrowserEndpointImpl() override = default;
};

class FakeBrowserHostImpl : public mojom::BrowserHost {
 public:
  ~FakeBrowserHostImpl() override = default;
};

// Records what the provider forwards, then answers from a separate task,
// mirroring the asynchronous verification hop BrowserRegistry takes. Replying
// out of band is what lets every test wait on the client's reply rather than
// draining the sequence.
class FakeBrowserHostProviderImplDelegate
    : public BrowserHostProviderImpl::Delegate {
 public:
  struct Call {
    uint32_t protocol_version = 0;
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint;
    mojo::PendingReceiver<mojom::BrowserHost> host;
  };

  FakeBrowserHostProviderImplDelegate() = default;
  ~FakeBrowserHostProviderImplDelegate() override = default;

  void SetResult(mojom::BrowserAuthResult result) { default_result_ = result; }

  // Lets a test tell two in-flight calls apart by what they asked for.
  void SetResultForVersion(uint32_t protocol_version,
                           mojom::BrowserAuthResult result) {
    results_.insert_or_assign(protocol_version, result);
  }

  std::vector<Call>& calls() { return calls_; }

 private:
  // BrowserHostProviderImpl::Delegate:
  void Authenticate(
      uint32_t protocol_version,
      mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
      mojo::PendingReceiver<mojom::BrowserHost> host,
      base::OnceCallback<void(mojom::BrowserAuthResult)> callback) override {
    calls_.push_back(Call{.protocol_version = protocol_version,
                          .browser_endpoint = std::move(browser_endpoint),
                          .host = std::move(host)});
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), ResultFor(protocol_version)));
  }

  mojom::BrowserAuthResult ResultFor(uint32_t protocol_version) const {
    const auto it = results_.find(protocol_version);
    return it == results_.end() ? default_result_ : it->second;
  }

  mojom::BrowserAuthResult default_result_ =
      mojom::BrowserAuthResult::kAccepted;
  base::flat_map<uint32_t, mojom::BrowserAuthResult> results_;
  std::vector<Call> calls_;
};

// Parameter for parameterized tests.
struct BrowserAuthResultParam {
  mojom::BrowserAuthResult result;
  const char* name;
};

}  // namespace

class BrowserHostProviderImplTest : public testing::Test {
 protected:
  struct BrowserPipes {
    FakeBrowserEndpointImpl endpoint_impl;
    mojo::Receiver<mojom::BrowserEndpoint> endpoint{&endpoint_impl};
    mojo::Remote<mojom::BrowserHost> host;
  };

  // Every connection is handed the same provider instance.
  mojo::Remote<mojom::BrowserHostProvider> ConnectClient() {
    mojo::Remote<mojom::BrowserHostProvider> client;
    receivers_.Add(&provider_, client.BindNewPipeAndPassReceiver());
    return client;
  }

  base::test::TaskEnvironment task_environment_;
  FakeBrowserHostProviderImplDelegate delegate_;
  BrowserHostProviderImpl provider_{&delegate_};
  mojo::ReceiverSet<mojom::BrowserHostProvider> receivers_;
};

TEST_F(BrowserHostProviderImplTest, ForwardsCallToDelegate) {
  constexpr uint32_t kProtocolVersion = 1;

  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  BrowserPipes pipes;
  base::test::TestFuture<mojom::BrowserAuthResult> replied;
  client->BindBrowserHost(
      kProtocolVersion, pipes.endpoint.BindNewPipeAndPassRemote(),
      pipes.host.BindNewPipeAndPassReceiver(), replied.GetCallback());

  // The reply only arrives after the delegate has been called, so waiting for
  // it is also how the test waits for the forwarded arguments.
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted, replied.Get());
  ASSERT_EQ(1u, delegate_.calls().size());
  FakeBrowserHostProviderImplDelegate::Call& call = delegate_.calls()[0];
  EXPECT_EQ(kProtocolVersion, call.protocol_version);

  // Both handles must arrive usable rather than merely non-empty. A round trip
  // in each direction proves the pipe survived forwarding; a handle dropped
  // along the way would have closed it.
  mojo::Remote<mojom::BrowserEndpoint> endpoint(
      std::move(call.browser_endpoint));
  FakeBrowserHostImpl host_impl;
  mojo::Receiver<mojom::BrowserHost> host_receiver(&host_impl,
                                                   std::move(call.host));
  endpoint.FlushForTesting();
  pipes.host.FlushForTesting();

  EXPECT_TRUE(endpoint.is_connected());
  EXPECT_TRUE(pipes.host.is_connected());
}

// The provider is shared by every connection, so it must hold no per-connection
// state: two clients in flight at once each get their own reply.
TEST_F(BrowserHostProviderImplTest, ServesConcurrentClients) {
  constexpr uint32_t kFirstVersion = 1;
  constexpr uint32_t kSecondVersion = 2;

  delegate_.SetResultForVersion(kFirstVersion,
                                mojom::BrowserAuthResult::kAccepted);
  delegate_.SetResultForVersion(kSecondVersion,
                                mojom::BrowserAuthResult::kRejected);

  mojo::Remote<mojom::BrowserHostProvider> first_client = ConnectClient();
  mojo::Remote<mojom::BrowserHostProvider> second_client = ConnectClient();
  BrowserPipes first_pipes;
  BrowserPipes second_pipes;

  base::test::TestFuture<mojom::BrowserAuthResult> first_replied;
  base::test::TestFuture<mojom::BrowserAuthResult> second_replied;
  first_client->BindBrowserHost(kFirstVersion,
                                first_pipes.endpoint.BindNewPipeAndPassRemote(),
                                first_pipes.host.BindNewPipeAndPassReceiver(),
                                first_replied.GetCallback());
  second_client->BindBrowserHost(
      kSecondVersion, second_pipes.endpoint.BindNewPipeAndPassRemote(),
      second_pipes.host.BindNewPipeAndPassReceiver(),
      second_replied.GetCallback());

  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted, first_replied.Get());
  EXPECT_EQ(mojom::BrowserAuthResult::kRejected, second_replied.Get());
  EXPECT_EQ(2u, delegate_.calls().size());
}

class BrowserHostProviderImplResultTest
    : public BrowserHostProviderImplTest,
      public testing::WithParamInterface<BrowserAuthResultParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BrowserHostProviderImplResultTest,
    testing::Values(
        BrowserAuthResultParam{mojom::BrowserAuthResult::kAccepted, "Accepted"},
        BrowserAuthResultParam{mojom::BrowserAuthResult::kRejected, "Rejected"},
        BrowserAuthResultParam{mojom::BrowserAuthResult::kVersionMismatch,
                               "VersionMismatch"},
        BrowserAuthResultParam{mojom::BrowserAuthResult::kAlreadyAuthenticated,
                               "AlreadyAuthenticated"}),
    [](const testing::TestParamInfo<BrowserAuthResultParam>& info) {
      return std::string(info.param.name);
    });

// Every outcome the delegate can produce must reach the browser unchanged.
TEST_P(BrowserHostProviderImplResultTest, RelaysDelegateResultToClient) {
  delegate_.SetResult(GetParam().result);

  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  BrowserPipes pipes;
  base::test::TestFuture<mojom::BrowserAuthResult> replied;
  client->BindBrowserHost(
      mojom::kProtocolVersion, pipes.endpoint.BindNewPipeAndPassRemote(),
      pipes.host.BindNewPipeAndPassReceiver(), replied.GetCallback());

  EXPECT_EQ(GetParam().result, replied.Get());
}

TEST(BrowserHostProviderImplDeathTest, NullDelegateChecks) {
  EXPECT_CHECK_DEATH({ BrowserHostProviderImpl provider(nullptr); });
}

}  // namespace brave_vpn::v2
