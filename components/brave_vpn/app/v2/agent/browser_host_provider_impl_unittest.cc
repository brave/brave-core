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
#include "brave/components/brave_vpn/app/v2/agent/test/fake_browser.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {

// Binds the host receiver the provider forwarded, which is the agent's side of
// that pipe rather than the browser's.
class FakeBrowserHost : public mojom::BrowserHost {
 public:
  ~FakeBrowserHost() override = default;
};

// Records what the provider forwards, then answers from a separate task,
// mirroring the asynchronous verification hop BrowserRegistry takes. Replying
// out of band is what lets every test wait on the client's reply rather than
// draining the sequence.
class FakeBrowserHostProviderImplDelegate
    : public BrowserHostProviderImpl::Delegate {
 public:
  struct InitCall {
    uint32_t protocol_version = 0;
    mojo::PlatformHandle identity_channel;
  };

  struct AuthCall {
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint;
    mojo::PendingReceiver<mojom::BrowserHost> host;
  };

  FakeBrowserHostProviderImplDelegate() = default;
  ~FakeBrowserHostProviderImplDelegate() override = default;

  void SetInitResult(mojom::BrowserInitResult result) {
    default_init_result_ = result;
  }

  // Lets a test tell two in-flight calls apart by the version they carried.
  void SetInitResultForVersion(uint32_t protocol_version,
                               mojom::BrowserInitResult result) {
    init_results_.insert_or_assign(protocol_version, result);
  }

  void SetAuthResult(mojom::BrowserAuthResult result) {
    default_auth_result_ = result;
  }

  std::vector<InitCall>& init_calls() { return init_calls_; }
  std::vector<AuthCall>& auth_calls() { return auth_calls_; }

 private:
  // BrowserHostProviderImpl::Delegate:
  void InitializeBrowser(
      uint32_t protocol_version,
      mojo::PlatformHandle identity_channel,
      base::OnceCallback<void(mojom::BrowserInitResult)> callback) override {
    init_calls_.push_back(
        InitCall{.protocol_version = protocol_version,
                 .identity_channel = std::move(identity_channel)});
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), InitResultFor(protocol_version)));
  }

  void AuthenticateBrowser(
      mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
      mojo::PendingReceiver<mojom::BrowserHost> host,
      base::OnceCallback<void(mojom::BrowserAuthResult)> callback) override {
    auth_calls_.push_back(
        AuthCall{.browser_endpoint = std::move(browser_endpoint),
                 .host = std::move(host)});
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), default_auth_result_));
  }

  mojom::BrowserInitResult InitResultFor(uint32_t protocol_version) const {
    const auto it = init_results_.find(protocol_version);
    return it == init_results_.end() ? default_init_result_ : it->second;
  }

  mojom::BrowserInitResult default_init_result_ =
      mojom::BrowserInitResult::kInitialized;
  mojom::BrowserAuthResult default_auth_result_ =
      mojom::BrowserAuthResult::kAccepted;
  base::flat_map<uint32_t, mojom::BrowserInitResult> init_results_;
  std::vector<InitCall> init_calls_;
  std::vector<AuthCall> auth_calls_;
};

// Parameters for parameterized tests.
struct BrowserInitResultParam {
  mojom::BrowserInitResult result;
  const char* name;
};

struct BrowserAuthResultParam {
  mojom::BrowserAuthResult result;
  const char* name;
};

}  // namespace

class BrowserHostProviderImplTest : public testing::Test {
 protected:
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

TEST_F(BrowserHostProviderImplTest, ForwardsInitializeToDelegate) {
  constexpr uint32_t kProtocolVersion = 1;

  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  FakeBrowser browser;
  client->Initialize(kProtocolVersion, browser.BindIdentityChannel(),
                     browser.GetInitReplyCallback());

  // The reply only arrives after the delegate has been called, so waiting for
  // it is also how the test waits for the forwarded arguments.
  EXPECT_EQ(mojom::BrowserInitResult::kInitialized, browser.WaitForInitReply());
  ASSERT_EQ(1u, delegate_.init_calls().size());
  EXPECT_EQ(kProtocolVersion, delegate_.init_calls()[0].protocol_version);
  EXPECT_TRUE(delegate_.init_calls()[0].identity_channel.is_valid());
}

// The argument is optional, and a null handle is what the browser sends today.
// It must arrive as a null handle rather than failing the call.
TEST_F(BrowserHostProviderImplTest, ForwardsNullIdentityChannelToDelegate) {
  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  FakeBrowser browser;
  client->Initialize(mojom::kProtocolVersion, mojo::PlatformHandle(),
                     browser.GetInitReplyCallback());

  EXPECT_EQ(mojom::BrowserInitResult::kInitialized, browser.WaitForInitReply());
  ASSERT_EQ(1u, delegate_.init_calls().size());
  EXPECT_FALSE(delegate_.init_calls()[0].identity_channel.is_valid());
}

TEST_F(BrowserHostProviderImplTest, ForwardsBindBrowserHostToDelegate) {
  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  FakeBrowser browser;
  client->BindBrowserHost(browser.BindEndpoint(), browser.BindHost(),
                          browser.GetAuthReplyCallback());

  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted, browser.WaitForAuthReply());
  ASSERT_EQ(1u, delegate_.auth_calls().size());
  FakeBrowserHostProviderImplDelegate::AuthCall& call =
      delegate_.auth_calls()[0];

  // Both handles must arrive usable rather than merely non-empty. A round trip
  // in each direction proves the pipe survived forwarding; a handle dropped
  // along the way would have closed it.
  mojo::Remote<mojom::BrowserEndpoint> endpoint(
      std::move(call.browser_endpoint));
  FakeBrowserHost host_impl;
  mojo::Receiver<mojom::BrowserHost> host_receiver(&host_impl,
                                                   std::move(call.host));
  endpoint.FlushForTesting();
  browser.FlushHost();

  EXPECT_TRUE(endpoint.is_connected());
  EXPECT_TRUE(browser.host_connected());
}

// The provider is shared by every connection, so it must hold no per-connection
// state: two clients in flight at once each get their own reply. Initialize()
// is the call that still carries something to tell them apart by.
TEST_F(BrowserHostProviderImplTest, ServesConcurrentInitializeCalls) {
  constexpr uint32_t kFirstVersion = 1;
  constexpr uint32_t kSecondVersion = 2;

  delegate_.SetInitResultForVersion(kFirstVersion,
                                    mojom::BrowserInitResult::kInitialized);
  delegate_.SetInitResultForVersion(kSecondVersion,
                                    mojom::BrowserInitResult::kVersionMismatch);

  mojo::Remote<mojom::BrowserHostProvider> first_client = ConnectClient();
  mojo::Remote<mojom::BrowserHostProvider> second_client = ConnectClient();
  FakeBrowser first_browser;
  FakeBrowser second_browser;

  first_client->Initialize(kFirstVersion, mojo::PlatformHandle(),
                           first_browser.GetInitReplyCallback());
  second_client->Initialize(kSecondVersion, mojo::PlatformHandle(),
                            second_browser.GetInitReplyCallback());

  EXPECT_EQ(mojom::BrowserInitResult::kInitialized,
            first_browser.WaitForInitReply());
  EXPECT_EQ(mojom::BrowserInitResult::kVersionMismatch,
            second_browser.WaitForInitReply());
  EXPECT_EQ(2u, delegate_.init_calls().size());
}

// Two BindBrowserHost() requests in flight at once must each get their own
// reply and their own handles, with nothing carried between them by the shared
// provider instance.
TEST_F(BrowserHostProviderImplTest, ServesConcurrentBindBrowserHostCalls) {
  mojo::Remote<mojom::BrowserHostProvider> first_client = ConnectClient();
  mojo::Remote<mojom::BrowserHostProvider> second_client = ConnectClient();
  FakeBrowser first_browser;
  FakeBrowser second_browser;

  first_client->BindBrowserHost(first_browser.BindEndpoint(),
                                first_browser.BindHost(),
                                first_browser.GetAuthReplyCallback());
  second_client->BindBrowserHost(second_browser.BindEndpoint(),
                                 second_browser.BindHost(),
                                 second_browser.GetAuthReplyCallback());

  // One reply per call, whichever order they were dispatched in.
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            first_browser.WaitForAuthReply());
  EXPECT_EQ(mojom::BrowserAuthResult::kAccepted,
            second_browser.WaitForAuthReply());
  ASSERT_EQ(2u, delegate_.auth_calls().size());

  first_browser.WatchHost();
  second_browser.WatchHost();

  // Binding both forwarded host receivers must connect both browsers' remotes
  // and close neither. Handles crossed between the calls, or a handle dropped
  // on the way, would leave one of the two unaffected either way round.
  FakeBrowserHost first_host_impl;
  FakeBrowserHost second_host_impl;
  mojo::Receiver<mojom::BrowserHost> first_host_receiver(
      &first_host_impl, std::move(delegate_.auth_calls()[0].host));
  mojo::Receiver<mojom::BrowserHost> second_host_receiver(
      &second_host_impl, std::move(delegate_.auth_calls()[1].host));
  first_browser.FlushHost();
  second_browser.FlushHost();

  EXPECT_TRUE(first_browser.host_connected());
  EXPECT_TRUE(second_browser.host_connected());
  EXPECT_FALSE(first_browser.host_closed());
  EXPECT_FALSE(second_browser.host_closed());

  // Same for the endpoints, in the other direction.
  mojo::Remote<mojom::BrowserEndpoint> first_endpoint(
      std::move(delegate_.auth_calls()[0].browser_endpoint));
  mojo::Remote<mojom::BrowserEndpoint> second_endpoint(
      std::move(delegate_.auth_calls()[1].browser_endpoint));
  first_endpoint.FlushForTesting();
  second_endpoint.FlushForTesting();

  EXPECT_TRUE(first_endpoint.is_connected());
  EXPECT_TRUE(second_endpoint.is_connected());
}

class BrowserHostProviderImplInitResultTest
    : public BrowserHostProviderImplTest,
      public testing::WithParamInterface<BrowserInitResultParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BrowserHostProviderImplInitResultTest,
    testing::Values(
        BrowserInitResultParam{mojom::BrowserInitResult::kInitialized,
                               "Initialized"},
        BrowserInitResultParam{mojom::BrowserInitResult::kVersionMismatch,
                               "VersionMismatch"},
        BrowserInitResultParam{mojom::BrowserInitResult::kNotIdentified,
                               "NotIdentified"},
        BrowserInitResultParam{mojom::BrowserInitResult::kInvalidRequest,
                               "InvalidRequest"}),
    [](const testing::TestParamInfo<BrowserInitResultParam>& info) {
      return std::string(info.param.name);
    });

// Every outcome the delegate can produce must reach the browser unchanged.
TEST_P(BrowserHostProviderImplInitResultTest, RelaysDelegateResultToClient) {
  delegate_.SetInitResult(GetParam().result);

  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  FakeBrowser browser;
  client->Initialize(mojom::kProtocolVersion, mojo::PlatformHandle(),
                     browser.GetInitReplyCallback());

  EXPECT_EQ(GetParam().result, browser.WaitForInitReply());
}

class BrowserHostProviderImplAuthResultTest
    : public BrowserHostProviderImplTest,
      public testing::WithParamInterface<BrowserAuthResultParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BrowserHostProviderImplAuthResultTest,
    testing::Values(
        BrowserAuthResultParam{mojom::BrowserAuthResult::kAccepted, "Accepted"},
        BrowserAuthResultParam{mojom::BrowserAuthResult::kRejected, "Rejected"},
        BrowserAuthResultParam{mojom::BrowserAuthResult::kHostAlreadyRequested,
                               "HostAlreadyRequested"},
        BrowserAuthResultParam{mojom::BrowserAuthResult::kInconclusive,
                               "Inconclusive"},
        BrowserAuthResultParam{mojom::BrowserAuthResult::kInvalidRequest,
                               "InvalidRequest"}),
    [](const testing::TestParamInfo<BrowserAuthResultParam>& info) {
      return std::string(info.param.name);
    });

TEST_P(BrowserHostProviderImplAuthResultTest, RelaysDelegateResultToClient) {
  delegate_.SetAuthResult(GetParam().result);

  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  FakeBrowser browser;
  client->BindBrowserHost(browser.BindEndpoint(), browser.BindHost(),
                          browser.GetAuthReplyCallback());

  EXPECT_EQ(GetParam().result, browser.WaitForAuthReply());
}

TEST(BrowserHostProviderImplDeathTest, NullDelegateChecks) {
  EXPECT_CHECK_DEATH({ BrowserHostProviderImpl provider(nullptr); });
}

}  // namespace brave_vpn::v2
