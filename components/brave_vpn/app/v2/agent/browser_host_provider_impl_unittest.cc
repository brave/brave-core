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

// Records what the provider forwards, then answers from a separate task. For
// Initialize() that mirrors the verification hop BrowserRegistry takes before
// it replies; BindBrowserHost() has no such hop in the registry, but posting
// its reply too is what lets every test wait on the client's reply rather than
// draining the sequence.
class FakeBrowserHostProviderImplDelegate
    : public BrowserHostProviderImpl::Delegate {
 public:
  struct InitializeCall {
    uint32_t protocol_version = 0;
    mojo::PlatformHandle identity_channel;
  };

  struct BindBrowserHostCall {
    mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint;
    mojo::PendingReceiver<mojom::BrowserHost> host;
  };

  FakeBrowserHostProviderImplDelegate() = default;
  ~FakeBrowserHostProviderImplDelegate() override = default;

  void SetInitializeResult(mojom::InitializeResult result) {
    default_initialize_result_ = result;
  }

  // Lets a test tell two in-flight calls apart by the version they carried.
  void SetInitializeResultForVersion(uint32_t protocol_version,
                                     mojom::InitializeResult result) {
    initialize_results_.insert_or_assign(protocol_version, result);
  }

  void SetBindBrowserHostResult(mojom::BindBrowserHostResult result) {
    default_bind_browser_host_result_ = result;
  }

  std::vector<InitializeCall>& initialize_calls() { return initialize_calls_; }
  std::vector<BindBrowserHostCall>& bind_browser_host_calls() {
    return bind_browser_host_calls_;
  }

 private:
  // BrowserHostProviderImpl::Delegate:
  void InitializeBrowser(
      uint32_t protocol_version,
      mojo::PlatformHandle identity_channel,
      base::OnceCallback<void(mojom::InitializeResult)> callback) override {
    initialize_calls_.push_back(
        InitializeCall{.protocol_version = protocol_version,
                       .identity_channel = std::move(identity_channel)});
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  InitializeResultFor(protocol_version)));
  }

  void BindBrowserHost(
      mojo::PendingRemote<mojom::BrowserEndpoint> browser_endpoint,
      mojo::PendingReceiver<mojom::BrowserHost> host,
      base::OnceCallback<void(mojom::BindBrowserHostResult)> callback)
      override {
    bind_browser_host_calls_.push_back(
        BindBrowserHostCall{.browser_endpoint = std::move(browser_endpoint),
                            .host = std::move(host)});
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), default_bind_browser_host_result_));
  }

  mojom::InitializeResult InitializeResultFor(uint32_t protocol_version) const {
    const auto it = initialize_results_.find(protocol_version);
    return it == initialize_results_.end() ? default_initialize_result_
                                           : it->second;
  }

  mojom::InitializeResult default_initialize_result_ =
      mojom::InitializeResult::kSuccess;
  mojom::BindBrowserHostResult default_bind_browser_host_result_ =
      mojom::BindBrowserHostResult::kSuccess;
  base::flat_map<uint32_t, mojom::InitializeResult> initialize_results_;
  std::vector<InitializeCall> initialize_calls_;
  std::vector<BindBrowserHostCall> bind_browser_host_calls_;
};

// Parameters for parameterized tests.
struct InitializeResultParam {
  mojom::InitializeResult result;
  const char* name;
};

struct BindBrowserHostResultParam {
  mojom::BindBrowserHostResult result;
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
                     browser.GetInitializeReplyCallback());

  // The reply only arrives after the delegate has been called, so waiting for
  // it is also how the test waits for the forwarded arguments.
  EXPECT_EQ(mojom::InitializeResult::kSuccess,
            browser.WaitForInitializeReply());
  ASSERT_EQ(1u, delegate_.initialize_calls().size());
  EXPECT_EQ(kProtocolVersion, delegate_.initialize_calls()[0].protocol_version);
  EXPECT_TRUE(delegate_.initialize_calls()[0].identity_channel.is_valid());
}

// The argument is optional, and a null handle is what the browser sends on
// Windows and Linux. It must arrive as a null handle rather than failing the
// call.
TEST_F(BrowserHostProviderImplTest, ForwardsNullIdentityChannelToDelegate) {
  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  FakeBrowser browser;
  client->Initialize(mojom::kProtocolVersion, mojo::PlatformHandle(),
                     browser.GetInitializeReplyCallback());

  EXPECT_EQ(mojom::InitializeResult::kSuccess,
            browser.WaitForInitializeReply());
  ASSERT_EQ(1u, delegate_.initialize_calls().size());
  EXPECT_FALSE(delegate_.initialize_calls()[0].identity_channel.is_valid());
}

TEST_F(BrowserHostProviderImplTest, ForwardsBindBrowserHostToDelegate) {
  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  FakeBrowser browser;
  client->BindBrowserHost(browser.BindEndpoint(), browser.BindHost(),
                          browser.GetBindBrowserHostReplyCallback());

  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            browser.WaitForBindBrowserHostReply());
  ASSERT_EQ(1u, delegate_.bind_browser_host_calls().size());
  FakeBrowserHostProviderImplDelegate::BindBrowserHostCall& call =
      delegate_.bind_browser_host_calls()[0];

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
// state: two clients in flight at once each get their own reply.
TEST_F(BrowserHostProviderImplTest, ServesConcurrentInitializeCalls) {
  constexpr uint32_t kFirstVersion = 1;
  constexpr uint32_t kSecondVersion = 2;

  delegate_.SetInitializeResultForVersion(kFirstVersion,
                                          mojom::InitializeResult::kSuccess);
  delegate_.SetInitializeResultForVersion(
      kSecondVersion, mojom::InitializeResult::kVersionMismatch);

  mojo::Remote<mojom::BrowserHostProvider> first_client = ConnectClient();
  mojo::Remote<mojom::BrowserHostProvider> second_client = ConnectClient();
  FakeBrowser first_browser;
  FakeBrowser second_browser;

  first_client->Initialize(kFirstVersion, mojo::PlatformHandle(),
                           first_browser.GetInitializeReplyCallback());
  second_client->Initialize(kSecondVersion, mojo::PlatformHandle(),
                            second_browser.GetInitializeReplyCallback());

  EXPECT_EQ(mojom::InitializeResult::kSuccess,
            first_browser.WaitForInitializeReply());
  EXPECT_EQ(mojom::InitializeResult::kVersionMismatch,
            second_browser.WaitForInitializeReply());
  EXPECT_EQ(2u, delegate_.initialize_calls().size());
}

// Two BindBrowserHost() requests in flight at once must each get their own
// reply and their own handles, with nothing carried between them by the shared
// provider instance.
TEST_F(BrowserHostProviderImplTest, ServesConcurrentBindBrowserHostCalls) {
  mojo::Remote<mojom::BrowserHostProvider> first_client = ConnectClient();
  mojo::Remote<mojom::BrowserHostProvider> second_client = ConnectClient();
  FakeBrowser first_browser;
  FakeBrowser second_browser;

  first_client->BindBrowserHost(
      first_browser.BindEndpoint(), first_browser.BindHost(),
      first_browser.GetBindBrowserHostReplyCallback());
  second_client->BindBrowserHost(
      second_browser.BindEndpoint(), second_browser.BindHost(),
      second_browser.GetBindBrowserHostReplyCallback());

  // One reply per call, whichever order they were dispatched in.
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            first_browser.WaitForBindBrowserHostReply());
  EXPECT_EQ(mojom::BindBrowserHostResult::kSuccess,
            second_browser.WaitForBindBrowserHostReply());
  ASSERT_EQ(2u, delegate_.bind_browser_host_calls().size());

  first_browser.WatchHost();
  second_browser.WatchHost();

  // Binding both forwarded host receivers must connect both browsers' remotes
  // and close neither. Handles crossed between the calls, or a handle dropped
  // on the way, would leave one of the two unaffected either way round.
  FakeBrowserHost first_host_impl;
  FakeBrowserHost second_host_impl;
  mojo::Receiver<mojom::BrowserHost> first_host_receiver(
      &first_host_impl, std::move(delegate_.bind_browser_host_calls()[0].host));
  mojo::Receiver<mojom::BrowserHost> second_host_receiver(
      &second_host_impl,
      std::move(delegate_.bind_browser_host_calls()[1].host));
  first_browser.FlushHost();
  second_browser.FlushHost();

  EXPECT_TRUE(first_browser.host_connected());
  EXPECT_TRUE(second_browser.host_connected());
  EXPECT_FALSE(first_browser.host_closed());
  EXPECT_FALSE(second_browser.host_closed());

  // Same for the endpoints, in the other direction.
  mojo::Remote<mojom::BrowserEndpoint> first_endpoint(
      std::move(delegate_.bind_browser_host_calls()[0].browser_endpoint));
  mojo::Remote<mojom::BrowserEndpoint> second_endpoint(
      std::move(delegate_.bind_browser_host_calls()[1].browser_endpoint));
  first_endpoint.FlushForTesting();
  second_endpoint.FlushForTesting();

  EXPECT_TRUE(first_endpoint.is_connected());
  EXPECT_TRUE(second_endpoint.is_connected());
}

class BrowserHostProviderImplInitializeResultTest
    : public BrowserHostProviderImplTest,
      public testing::WithParamInterface<InitializeResultParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BrowserHostProviderImplInitializeResultTest,
    testing::Values(
        InitializeResultParam{mojom::InitializeResult::kSuccess, "Success"},
        InitializeResultParam{mojom::InitializeResult::kVersionMismatch,
                              "VersionMismatch"},
        InitializeResultParam{mojom::InitializeResult::kNotIdentified,
                              "NotIdentified"},
        InitializeResultParam{mojom::InitializeResult::kRejected, "Rejected"},
        InitializeResultParam{mojom::InitializeResult::kInconclusive,
                              "Inconclusive"},
        InitializeResultParam{mojom::InitializeResult::kInvalidRequest,
                              "InvalidRequest"}),
    [](const testing::TestParamInfo<InitializeResultParam>& info) {
      return std::string(info.param.name);
    });

// Every outcome the delegate can produce must reach the browser unchanged.
TEST_P(BrowserHostProviderImplInitializeResultTest,
       RelaysDelegateResultToClient) {
  delegate_.SetInitializeResult(GetParam().result);

  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  FakeBrowser browser;
  client->Initialize(mojom::kProtocolVersion, mojo::PlatformHandle(),
                     browser.GetInitializeReplyCallback());

  EXPECT_EQ(GetParam().result, browser.WaitForInitializeReply());
}

class BrowserHostProviderImplBindBrowserHostResultTest
    : public BrowserHostProviderImplTest,
      public testing::WithParamInterface<BindBrowserHostResultParam> {};

INSTANTIATE_TEST_SUITE_P(
    ,
    BrowserHostProviderImplBindBrowserHostResultTest,
    testing::Values(
        BindBrowserHostResultParam{mojom::BindBrowserHostResult::kSuccess,
                                   "Success"},
        BindBrowserHostResultParam{mojom::BindBrowserHostResult::kUninitialized,
                                   "Uninitialized"},
        BindBrowserHostResultParam{mojom::BindBrowserHostResult::kAlreadyBound,
                                   "AlreadyBound"}),
    [](const testing::TestParamInfo<BindBrowserHostResultParam>& info) {
      return std::string(info.param.name);
    });

TEST_P(BrowserHostProviderImplBindBrowserHostResultTest,
       RelaysDelegateResultToClient) {
  delegate_.SetBindBrowserHostResult(GetParam().result);

  mojo::Remote<mojom::BrowserHostProvider> client = ConnectClient();
  FakeBrowser browser;
  client->BindBrowserHost(browser.BindEndpoint(), browser.BindHost(),
                          browser.GetBindBrowserHostReplyCallback());

  EXPECT_EQ(GetParam().result, browser.WaitForBindBrowserHostReply());
}

TEST(BrowserHostProviderImplDeathTest, NullDelegateChecks) {
  EXPECT_CHECK_DEATH({ BrowserHostProviderImpl provider(nullptr); });
}

}  // namespace brave_vpn::v2
