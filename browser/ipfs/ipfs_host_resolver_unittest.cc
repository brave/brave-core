/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_host_resolver.h"

#include <memory>
#include <utility>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"

#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/stub_resolver_config_reader.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/browser_task_environment.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/host_resolver.h"
#include "services/network/network_context.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/resolve_host_client_base.h"
#include "services/network/public/mojom/host_resolver.mojom.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/test/test_network_context.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

class FakeHostResolver : public network::mojom::HostResolver {
 public:
  explicit FakeHostResolver(const std::string& expected_host)
      : expected_host_(expected_host) {}
  ~FakeHostResolver() override {}

  // network::mojom::HostResolver
  void ResolveHost(const net::HostPortPair& host,
                   const net::NetworkIsolationKey& network_isolation_key,
                   network::mojom::ResolveHostParametersPtr parameters,
                   mojo::PendingRemote<network::mojom::ResolveHostClient>
                       pending_response_client) override {
    EXPECT_EQ(expected_host_, host.host());
    EXPECT_EQ(parameters->dns_query_type, net::DnsQueryType::TXT);
    mojo::Remote<network::mojom::ResolveHostClient> response_client;
    response_client.Bind(std::move(pending_response_client));
    response_client->OnTextResults(text_results_);
    resolve_host_called_++;
  }

  void MdnsListen(
      const ::net::HostPortPair& host,
      ::net::DnsQueryType query_type,
      ::mojo::PendingRemote<network::mojom::MdnsListenClient> response_client,
      MdnsListenCallback callback) override {}

  int resolve_host_called() { return resolve_host_called_; }

  void RespondTextResults(const std::vector<std::string>& text_results) {
    text_results_ = text_results;
  }

  void SetExpectedHost(const std::string& expected_host) {
    expected_host_ = expected_host;
  }

 protected:
  int resolve_host_called_ = 0;

 private:
  std::vector<std::string> text_results_;
  std::string expected_host_;
};

class FakeHostResolverFail : public FakeHostResolver {
 public:
  explicit FakeHostResolverFail(const std::string& expected_host)
      : FakeHostResolver(expected_host) {}
  ~FakeHostResolverFail() override {}

  // network::mojom::HostResolver
  void ResolveHost(const net::HostPortPair& host,
                   const net::NetworkIsolationKey& network_isolation_key,
                   network::mojom::ResolveHostParametersPtr parameters,
                   mojo::PendingRemote<network::mojom::ResolveHostClient>
                       pending_response_client) override {
    mojo::Remote<network::mojom::ResolveHostClient> response_client;
    response_client.Bind(std::move(pending_response_client));
    response_client->OnComplete(-2, net::ResolveErrorInfo(), base::nullopt);
    resolve_host_called_++;
  }
};

class FakeNetworkContext : public network::TestNetworkContext {
 public:
  FakeNetworkContext() {}
  ~FakeNetworkContext() override {}

  // network::mojom::HostResolver
  void ResolveHost(const net::HostPortPair& host,
                   const net::NetworkIsolationKey& network_isolation_key,
                   network::mojom::ResolveHostParametersPtr parameters,
                   mojo::PendingRemote<network::mojom::ResolveHostClient>
                       pending_response_client) override {
    DCHECK(host_resolver_);
    host_resolver_->ResolveHost(host, network_isolation_key,
                                std::move(parameters),
                                std::move(pending_response_client));
  }

  void SetHostResolver(
      std::unique_ptr<network::mojom::HostResolver> host_resolver) {
    host_resolver_ = std::move(host_resolver);
  }

 private:
  std::unique_ptr<network::mojom::HostResolver> host_resolver_;
};

class IPFSHostResolverTest : public testing::Test {
 public:
  IPFSHostResolverTest() {
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    network_context_.reset(new FakeNetworkContext());
  }

  void HostResolvedCallback(base::OnceClosure callback,
                            const std::string& expected_host,
                            const std::string& host) {
    EXPECT_EQ(expected_host, host);
    resolved_callback_called_++;
    if (callback)
      std::move(callback).Run();
  }

  FakeNetworkContext* GetNetworkContext() { return network_context_.get(); }

  void SetResolvedCallbackCalled(int value) {
    resolved_callback_called_ = value;
  }
  int resolved_callback_called() const { return resolved_callback_called_; }

  content::BrowserTaskEnvironment task_environment_;
  int resolved_callback_called_ = 0;
  std::unique_ptr<FakeNetworkContext> network_context_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  std::unique_ptr<StubResolverConfigReader> stub_resolver_config_reader_;
  base::WeakPtrFactory<IPFSHostResolverTest> weak_ptr_factory_{this};
};

TEST_F(IPFSHostResolverTest, PrefixRunSuccess) {
  std::string prefix = "__dnslink.";
  std::vector<std::string> success = {"dnslink=abc", "a", "a=b", ""};
  std::string host = "example.com";

  std::unique_ptr<FakeHostResolver> fake_host_resolver(
      new FakeHostResolver(prefix + host));
  fake_host_resolver->RespondTextResults(success);
  auto* network_context = GetNetworkContext();
  auto* fake_host_resolver_raw = fake_host_resolver.get();
  network_context->SetHostResolver(std::move(fake_host_resolver));
  base::RunLoop run_loop;
  ipfs::IPFSHostResolver ipfs_resolver(network_context, prefix);

  SetResolvedCallbackCalled(0);
  ipfs_resolver.Resolve(
      net::HostPortPair(host, 11), net::NetworkIsolationKey(),
      net::DnsQueryType::TXT,
      base::BindOnce(&IPFSHostResolverTest::HostResolvedCallback,
                     weak_ptr_factory_.GetWeakPtr(), run_loop.QuitClosure(),
                     host));

  run_loop.Run();
  EXPECT_EQ(ipfs_resolver.host(), host);
  EXPECT_EQ(fake_host_resolver_raw->resolve_host_called(), 1);
  EXPECT_EQ(resolved_callback_called(), 1);
}

TEST_F(IPFSHostResolverTest, SuccessOnReuse) {
  std::string prefix = "__dnslink.";
  std::vector<std::string> success = {"dnslink=abc", "a", "a=b", ""};
  std::string host = "example.com";

  std::unique_ptr<FakeHostResolver> fake_host_resolver(
      new FakeHostResolver(prefix + host));
  fake_host_resolver->RespondTextResults(success);
  auto* network_context = GetNetworkContext();
  auto* fake_host_resolver_raw = fake_host_resolver.get();
  network_context->SetHostResolver(std::move(fake_host_resolver));
  base::RunLoop run_loop;
  ipfs::IPFSHostResolver ipfs_resolver(network_context, prefix);

  SetResolvedCallbackCalled(0);
  ipfs_resolver.Resolve(
      net::HostPortPair(host, 11), net::NetworkIsolationKey(),
      net::DnsQueryType::TXT,
      base::BindOnce(&IPFSHostResolverTest::HostResolvedCallback,
                     weak_ptr_factory_.GetWeakPtr(), run_loop.QuitClosure(),
                     host));

  run_loop.Run();
  EXPECT_EQ(ipfs_resolver.host(), host);
  EXPECT_EQ(fake_host_resolver_raw->resolve_host_called(), 1);
  EXPECT_EQ(resolved_callback_called(), 1);

  ipfs_resolver.Resolve(
      net::HostPortPair(host, 11), net::NetworkIsolationKey(),
      net::DnsQueryType::TXT,
      base::BindOnce(
          [](const std::string& expected_host, const std::string& host) {
            EXPECT_EQ(expected_host, host);
          },
          host));
  EXPECT_EQ(fake_host_resolver_raw->resolve_host_called(), 1);
  EXPECT_EQ(resolved_callback_called(), 1);
}

TEST_F(IPFSHostResolverTest, ResolutionFailed) {
  std::string host = "example.com";
  std::unique_ptr<FakeHostResolverFail> fake_host_resolver(
      new FakeHostResolverFail(host));
  auto* network_context = GetNetworkContext();
  auto* fake_host_resolver_raw = fake_host_resolver.get();
  network_context->SetHostResolver(std::move(fake_host_resolver));
  base::RunLoop run_loop;
  ipfs::IPFSHostResolver ipfs_resolver(network_context);
  ipfs_resolver.SetCompleteCallbackForTesting(run_loop.QuitClosure());
  SetResolvedCallbackCalled(0);
  ipfs_resolver.Resolve(
      net::HostPortPair(host, 11), net::NetworkIsolationKey(),
      net::DnsQueryType::TXT,
      base::BindOnce([](const std::string& host) { NOTREACHED(); }));
  run_loop.Run();
  EXPECT_EQ(ipfs_resolver.host(), host);
  EXPECT_EQ(fake_host_resolver_raw->resolve_host_called(), 1);
  EXPECT_EQ(resolved_callback_called(), 0);
}
