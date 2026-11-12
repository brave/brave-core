// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/net/storage_partition_proxy_service.h"

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/test/gtest_util.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/testing_pref_store.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/test/storage_partition_test_helpers.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/net_errors.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/proxy_config_with_annotation.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave {
namespace {

constexpr auto kOverride =
    StoragePartitionProxyService::Precedence::kOverrideExtensionAndCommandLine;
constexpr auto kYield =
    StoragePartitionProxyService::Precedence::kYieldToExtensionAndCommandLine;

content::StoragePartitionConfig Partition(const char* name,
                                          bool in_memory = false) {
  return content::CreateStoragePartitionConfigForTesting(in_memory, "test",
                                                         name);
}

net::ProxyConfigWithAnnotation ProxyConfig(const char* server,
                                           const char* bypass = "") {
  net::ProxyConfig config;
  config.proxy_rules().ParseFromString(server);
  config.proxy_rules().bypass_rules.ParseFromString(bypass);
  return net::ProxyConfigWithAnnotation(config, TRAFFIC_ANNOTATION_FOR_TESTS);
}

class ProxyClient : public network::mojom::ProxyConfigClient {
 public:
  void Bind(network::mojom::NetworkContextParams* params) {
    config = params->initial_proxy_config;
    receiver.Bind(std::move(params->proxy_config_client_receiver));
  }

  void OnProxyConfigUpdated(
      const net::ProxyConfigWithAnnotation& value) override {
    config = value;
    ++update_count;
  }

  void FlushProxyConfig(FlushProxyConfigCallback callback) override {
    std::move(callback).Run();
  }

  std::string ProxyFor(const GURL& url = GURL("https://example.test/")) const {
    CHECK(config);
    net::ProxyInfo info;
    config->value().proxy_rules().Apply(url, &info);
    return info.ToPacString();
  }

  // Requests fail because no proxy is available and nothing bypasses it.
  bool IsBlocked() const {
    return config && ProxyFor().empty() &&
           ProxyFor(GURL("http://localhost/")).empty();
  }

  int update_count = 0;
  std::optional<net::ProxyConfigWithAnnotation> config;
  mojo::Receiver<network::mojom::ProxyConfigClient> receiver{this};
};

class ErrorClient : public network::mojom::ProxyErrorClient {
 public:
  void OnPACScriptError(int32_t line_number,
                        const std::string& details) override {
    ++pac_errors;
  }
  void OnRequestMaybeFailedDueToProxySettings(int32_t net_error) override {
    ++request_errors;
  }

  int pac_errors = 0;
  int request_errors = 0;
  mojo::Receiver<network::mojom::ProxyErrorClient> receiver{this};
};

class ErrorObserver : public StoragePartitionProxyService::Observer {
 public:
  void OnPACScriptError(const content::StoragePartitionConfig& partition_config,
                        int32_t line_number,
                        const std::string& details) override {
    pac_errors.emplace_back(partition_config, line_number, details);
  }
  void OnRequestMaybeFailedDueToProxySettings(
      const content::StoragePartitionConfig& partition_config,
      int32_t net_error) override {
    request_errors.emplace_back(partition_config, net_error);
  }

  std::vector<std::tuple<content::StoragePartitionConfig, int32_t, std::string>>
      pac_errors;
  std::vector<std::tuple<content::StoragePartitionConfig, int32_t>>
      request_errors;
};

}  // namespace

class StoragePartitionProxyServiceTest : public testing::Test {
 protected:
  StoragePartitionProxyServiceTest() {
    prefs_.registry()->RegisterDictionaryPref(proxy_config::prefs::kProxy);
    prefs_.registry()->RegisterListPref(
        proxy_config::prefs::kProxyOverrideRules);
    local_state_.registry()->RegisterDictionaryPref(
        proxy_config::prefs::kProxy);
    local_state_.registry()->RegisterListPref(
        proxy_config::prefs::kProxyOverrideRules);
    service_ = std::make_unique<StoragePartitionProxyService>(
        prefs_, local_state_, /*is_off_the_record=*/false);
  }

  network::mojom::NetworkContextParamsPtr Params(
      mojo::Remote<network::mojom::ProxyConfigClient>& upstream) {
    auto params = network::mojom::NetworkContextParams::New();
    params->initial_proxy_config = ProxyConfig("http://global.example:8080");
    params->proxy_config_client_receiver =
        upstream.BindNewPipeAndPassReceiver();
    return params;
  }

  void SetProxy(const char* id, const char* server, const char* bypass = "") {
    service_->SetProxyConfig(Partition(id), ProxyConfig(server, bypass),
                             kOverride);
  }

  void Configure(const char* id,
                 network::mojom::NetworkContextParams* params,
                 bool in_memory = false) {
    service_->ConfigureNetworkContextParams(Partition(id, in_memory), params);
  }

  // Creates a network context for `partition` that inherits the profile proxy.
  void Connect(const content::StoragePartitionConfig& partition,
               mojo::Remote<network::mojom::ProxyConfigClient>& upstream,
               ProxyClient& client) {
    auto params = Params(upstream);
    service_->ConfigureNetworkContextParams(partition, params.get());
    client.Bind(params.get());
  }

  base::test::TaskEnvironment environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<StoragePartitionProxyService> service_;
};

TEST_F(StoragePartitionProxyServiceTest,
       IsolatesOverridesAndRetainsInheritedUpdates) {
  mojo::Remote<network::mojom::ProxyConfigClient> upstream_a, upstream_b;
  auto params_a = Params(upstream_a);
  auto params_b = Params(upstream_b);
  Configure("a", params_a.get());
  Configure("b", params_b.get());
  ProxyClient a, b;
  a.Bind(params_a.get());
  b.Bind(params_b.get());
  EXPECT_EQ(a.ProxyFor(), "PROXY global.example:8080");
  SetProxy("a", "socks5://partition.example:1080", "*.bypass.test");
  service_->FlushForTesting();
  EXPECT_EQ(a.ProxyFor(), "SOCKS5 partition.example:1080");
  EXPECT_EQ(a.ProxyFor(GURL("http://www.bypass.test/")), "DIRECT");
  EXPECT_EQ(b.ProxyFor(), "PROXY global.example:8080");

  upstream_a->OnProxyConfigUpdated(ProxyConfig("http://new.example:8081"));
  upstream_b->OnProxyConfigUpdated(ProxyConfig("http://new.example:8081"));
  service_->FlushForTesting();
  EXPECT_EQ(a.ProxyFor(), "SOCKS5 partition.example:1080");
  EXPECT_EQ(b.ProxyFor(), "PROXY new.example:8081");
  service_->ClearProxyConfig(Partition("a"));
  service_->FlushForTesting();
  EXPECT_FALSE(service_->GetProxyConfig(Partition("a")));
  EXPECT_EQ(a.ProxyFor(), "PROXY new.example:8081");
}

TEST_F(StoragePartitionProxyServiceTest,
       MultiplePartitionsUseIndependentOverrides) {
  const auto partition_a = Partition("a");
  const auto partition_b = Partition("b");
  SetProxy("a", "http://a.example:8080");
  SetProxy("b", "socks5://b.example:1080");
  mojo::Remote<network::mojom::ProxyConfigClient> upstream_a, upstream_b,
      upstream_c, upstream_default;
  ProxyClient a, b, c, default_client;
  Connect(partition_a, upstream_a, a);
  Connect(partition_b, upstream_b, b);
  Connect(Partition("c"), upstream_c, c);
  Connect(content::CreateStoragePartitionConfigForTesting(), upstream_default,
          default_client);
  EXPECT_EQ(a.ProxyFor(), "PROXY a.example:8080");
  EXPECT_EQ(b.ProxyFor(), "SOCKS5 b.example:1080");
  EXPECT_EQ(c.ProxyFor(), "PROXY global.example:8080");
  EXPECT_EQ(default_client.ProxyFor(), "PROXY global.example:8080");

  // Changing one override leaves the other partitions untouched.
  SetProxy("a", "https://a2.example:443");
  service_->FlushForTesting();
  EXPECT_EQ(a.ProxyFor(), "HTTPS a2.example:443");
  EXPECT_EQ(b.ProxyFor(), "SOCKS5 b.example:1080");
  EXPECT_EQ(a.update_count, 1);
  EXPECT_EQ(b.update_count, 0);
  EXPECT_EQ(c.update_count, 0);
  EXPECT_EQ(default_client.update_count, 0);

  // Profile updates only reach inheriting partitions.
  for (auto* upstream :
       {&upstream_a, &upstream_b, &upstream_c, &upstream_default}) {
    (*upstream)->OnProxyConfigUpdated(ProxyConfig("http://new.example:8081"));
  }
  service_->FlushForTesting();
  EXPECT_EQ(a.ProxyFor(), "HTTPS a2.example:443");
  EXPECT_EQ(b.ProxyFor(), "SOCKS5 b.example:1080");
  EXPECT_EQ(c.ProxyFor(), "PROXY new.example:8081");
  EXPECT_EQ(default_client.ProxyFor(), "PROXY new.example:8081");

  // Clearing one override only affects that partition.
  service_->ClearProxyConfig(partition_b);
  service_->FlushForTesting();
  EXPECT_EQ(a.ProxyFor(), "HTTPS a2.example:443");
  EXPECT_EQ(b.ProxyFor(), "PROXY new.example:8081");
  EXPECT_TRUE(service_->GetProxyConfig(partition_a));
  EXPECT_FALSE(service_->GetProxyConfig(partition_b));
}

TEST_F(StoragePartitionProxyServiceTest,
       PolicyAppliesToEveryOverriddenPartition) {
  SetProxy("a", "http://a.example:8080");
  SetProxy("b", "http://b.example:8080");
  mojo::Remote<network::mojom::ProxyConfigClient> upstream_a, upstream_b;
  ProxyClient a, b;
  Connect(Partition("a"), upstream_a, a);
  Connect(Partition("b"), upstream_b, b);

  prefs_.SetManagedPref(proxy_config::prefs::kProxy, base::DictValue());
  service_->FlushForTesting();
  EXPECT_EQ(a.ProxyFor(), "PROXY global.example:8080");
  EXPECT_EQ(b.ProxyFor(), "PROXY global.example:8080");

  prefs_.RemoveManagedPref(proxy_config::prefs::kProxy);
  service_->FlushForTesting();
  EXPECT_EQ(a.ProxyFor(), "PROXY a.example:8080");
  EXPECT_EQ(b.ProxyFor(), "PROXY b.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       KeyedByPartitionDomainNameAndStorageType) {
  const auto domain_a = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false, "domain-a", "same");
  const auto domain_b = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false, "domain-b", "same");
  const auto in_memory = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/true, "domain-a", "same");
  const auto unnamed = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false, "domain-a", "");
  service_->SetProxyConfig(domain_a, ProxyConfig("http://a.example:8080"),
                           kOverride);
  service_->SetProxyConfig(domain_b, ProxyConfig("http://b.example:8080"),
                           kOverride);
  service_->SetProxyConfig(in_memory, ProxyConfig("http://memory.example:8080"),
                           kOverride);
  service_->SetProxyConfig(unnamed, ProxyConfig("http://unnamed.example:8080"),
                           kOverride);
  mojo::Remote<network::mojom::ProxyConfigClient> upstream_a, upstream_b,
      upstream_memory, upstream_unnamed;
  ProxyClient a, b, memory, unnamed_client;
  Connect(domain_a, upstream_a, a);
  Connect(domain_b, upstream_b, b);
  Connect(in_memory, upstream_memory, memory);
  Connect(unnamed, upstream_unnamed, unnamed_client);
  EXPECT_EQ(a.ProxyFor(), "PROXY a.example:8080");
  EXPECT_EQ(b.ProxyFor(), "PROXY b.example:8080");
  EXPECT_EQ(memory.ProxyFor(), "PROXY memory.example:8080");
  EXPECT_EQ(unnamed_client.ProxyFor(), "PROXY unnamed.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest, BlobUrlFallbackModeIsIgnored) {
  auto with_fallback = Partition("a");
  with_fallback.set_fallback_to_partition_domain_for_blob_urls(
      content::StoragePartitionConfig::FallbackMode::kFallbackPartitionOnDisk);
  SetProxy("a", "http://partition.example:8080");
  EXPECT_TRUE(service_->GetProxyConfig(with_fallback));
  mojo::Remote<network::mojom::ProxyConfigClient> upstream, fallback_upstream;
  ProxyClient client, fallback_client;
  Connect(Partition("a"), upstream, client);
  Connect(with_fallback, fallback_upstream, fallback_client);
  EXPECT_EQ(client.ProxyFor(), "PROXY partition.example:8080");
  EXPECT_EQ(fallback_client.ProxyFor(), "PROXY partition.example:8080");

  service_->ClearProxyConfig(with_fallback);
  service_->FlushForTesting();
  EXPECT_FALSE(service_->GetProxyConfig(Partition("a")));
  EXPECT_EQ(client.ProxyFor(), "PROXY global.example:8080");
  EXPECT_EQ(fallback_client.ProxyFor(), "PROXY global.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest, ExplicitDirectAndRecreatedContexts) {
  service_->SetProxyConfig(Partition("a"),
                           net::ProxyConfigWithAnnotation::CreateDirect(),
                           kOverride);
  for (int i = 0; i < 2; ++i) {
    mojo::Remote<network::mojom::ProxyConfigClient> upstream;
    auto params = Params(upstream);
    Configure("a", params.get());
    ProxyClient client;
    client.Bind(params.get());
    EXPECT_EQ(client.ProxyFor(), "DIRECT");
    // Drop the network-side pipe, as happens during a network-service crash.
    client.receiver.reset();
    service_->FlushForTesting();
  }
}

TEST_F(StoragePartitionProxyServiceTest,
       ManagedProxyTakesPrecedenceAndCanBeRemoved) {
  SetProxy("a", "http://partition.example:8080");
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  EXPECT_EQ(client.ProxyFor(), "PROXY partition.example:8080");
  prefs_.SetManagedPref(proxy_config::prefs::kProxy, base::DictValue());
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY global.example:8080");
  EXPECT_TRUE(service_->GetProxyConfig(Partition("a")));
  prefs_.RemoveManagedPref(proxy_config::prefs::kProxy);
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY partition.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest, ManagedOverrideRulesTakePrecedence) {
  SetProxy("a", "http://partition.example:8080");
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  prefs_.SetManagedPref(proxy_config::prefs::kProxyOverrideRules,
                        base::ListValue());
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY global.example:8080");
  prefs_.RemoveManagedPref(proxy_config::prefs::kProxyOverrideRules);
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY partition.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest, LocalStatePolicyTakesPrecedence) {
  sync_preferences::TestingPrefServiceSyncable local_state;
  local_state.registry()->RegisterDictionaryPref(proxy_config::prefs::kProxy);
  StoragePartitionProxyService service(prefs_, local_state,
                                       /*is_off_the_record=*/false);
  service.SetProxyConfig(
      Partition("a"), ProxyConfig("http://partition.example:8080"), kOverride);
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  service.ConfigureNetworkContextParams(Partition("a"), params.get());
  ProxyClient client;
  client.Bind(params.get());
  local_state.SetManagedPref(proxy_config::prefs::kProxy, base::DictValue());
  service.FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY global.example:8080");
  local_state.RemoveManagedPref(proxy_config::prefs::kProxy);
  service.FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY partition.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       InheritedDynamicRoutingTakesPrecedence) {
  SetProxy("a", "http://partition.example:8080");
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  net::ProxyConfig inherited;
  inherited.proxy_rules().ParseFromString("http://managed.example:8080");
  net::ProxyConfig::DynamicRoutingConfig routing;
  routing.is_update_in_progress = true;
  inherited.set_dynamic_routing_config(std::move(routing));
  params->initial_proxy_config =
      net::ProxyConfigWithAnnotation(inherited, TRAFFIC_ANNOTATION_FOR_TESTS);
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  EXPECT_EQ(client.ProxyFor(), "PROXY managed.example:8080");
  upstream->OnProxyConfigUpdated(ProxyConfig("http://global.example:8080"));
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY partition.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       InMemoryAndDiskPartitionsStaySeparate) {
  SetProxy("a", "http://disk.example:8080");
  service_->SetProxyConfig(Partition("a", true),
                           ProxyConfig("http://memory.example:8080"),
                           kOverride);
  mojo::Remote<network::mojom::ProxyConfigClient> disk_upstream,
      memory_upstream;
  auto disk_params = Params(disk_upstream);
  auto memory_params = Params(memory_upstream);
  Configure("a", disk_params.get());
  Configure("a", memory_params.get(), true);
  ProxyClient disk, memory;
  disk.Bind(disk_params.get());
  memory.Bind(memory_params.get());
  EXPECT_EQ(disk.ProxyFor(), "PROXY disk.example:8080");
  EXPECT_EQ(memory.ProxyFor(), "PROXY memory.example:8080");
  service_->ClearProxyConfig(Partition("a", true));
  service_->FlushForTesting();
  EXPECT_EQ(disk.ProxyFor(), "PROXY disk.example:8080");
  EXPECT_EQ(memory.ProxyFor(), "PROXY global.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       DefaultPartitionSupportsRuntimeOverrides) {
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  service_->ConfigureNetworkContextParams(
      content::CreateStoragePartitionConfigForTesting(), params.get());
  ProxyClient client;
  client.Bind(params.get());
  service_->SetProxyConfig(content::CreateStoragePartitionConfigForTesting(),
                           ProxyConfig("https://default.example:443"),
                           kOverride);
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "HTTPS default.example:443");
  service_->ClearProxyConfig(content::CreateStoragePartitionConfigForTesting());
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY global.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       InheritWaitsForPendingInitialConfiguration) {
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  params->initial_proxy_config.reset();
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  EXPECT_FALSE(client.config);
  upstream->OnProxyConfigUpdated(ProxyConfig("http://global.example:8080"));
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY global.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       MultipleContextsForSamePartitionUpdate) {
  mojo::Remote<network::mojom::ProxyConfigClient> upstream_a, upstream_b;
  auto params_a = Params(upstream_a);
  auto params_b = Params(upstream_b);
  Configure("a", params_a.get());
  Configure("a", params_b.get());
  ProxyClient a, b;
  a.Bind(params_a.get());
  b.Bind(params_b.get());
  SetProxy("a", "http://partition.example:8080");
  service_->FlushForTesting();
  EXPECT_EQ(a.ProxyFor(), "PROXY partition.example:8080");
  EXPECT_EQ(b.ProxyFor(), "PROXY partition.example:8080");
  SetProxy("a", "http://partition.example:8080");
  service_->ClearProxyConfig(Partition("missing"));
  service_->FlushForTesting();
  EXPECT_EQ(a.update_count, 1);
  EXPECT_EQ(b.update_count, 1);
}

TEST_F(StoragePartitionProxyServiceTest, ConfigurationIsNotPersisted) {
  SetProxy("a", "http://partition.example:8080");
  service_->Shutdown();
  EXPECT_FALSE(service_->GetProxyConfig(Partition("a")));
  service_ = std::make_unique<StoragePartitionProxyService>(
      prefs_, local_state_, /*is_off_the_record=*/false);
  EXPECT_FALSE(service_->GetProxyConfig(Partition("a")));
}

TEST_F(StoragePartitionProxyServiceTest, SupportsPACConfiguration) {
  net::ProxyConfig config;
  config.set_pac_url(GURL("https://proxy.example/proxy.pac"));
  config.set_pac_mandatory(true);
  service_->SetProxyConfig(
      Partition("a"),
      net::ProxyConfigWithAnnotation(config, TRAFFIC_ANNOTATION_FOR_TESTS),
      kOverride);
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  ASSERT_TRUE(client.config);
  EXPECT_TRUE(client.config->value().Equals(config));
}

TEST_F(StoragePartitionProxyServiceTest, ForwardsFlushAcknowledgements) {
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  base::test::TestFuture<void> flushed;
  upstream->FlushProxyConfig(flushed.GetCallback());
  EXPECT_TRUE(flushed.Wait());
}

TEST_F(StoragePartitionProxyServiceTest, UpdatesTrafficAnnotation) {
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  SetProxy("a", "http://partition.example:8080");
  service_->FlushForTesting();
  const auto annotation =
      net::ProxyConfigWithAnnotation::CreateDirect().traffic_annotation();
  service_->SetProxyConfig(
      Partition("a"),
      net::ProxyConfigWithAnnotation(
          ProxyConfig("http://partition.example:8080").value(), annotation),
      kOverride);
  service_->FlushForTesting();
  ASSERT_TRUE(client.config);
  EXPECT_EQ(client.config->traffic_annotation(), annotation);
  EXPECT_EQ(client.update_count, 2);
}

TEST_F(StoragePartitionProxyServiceTest,
       ClearWhileInheritedConfigPendingBlocksUntilInheritedArrives) {
  SetProxy("a", "http://partition.example:8080");
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  params->initial_proxy_config.reset();
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  EXPECT_EQ(client.ProxyFor(), "PROXY partition.example:8080");

  service_->ClearProxyConfig(Partition("a"));
  service_->FlushForTesting();
  EXPECT_TRUE(client.IsBlocked());

  upstream->OnProxyConfigUpdated(ProxyConfig("http://global.example:8080"));
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY global.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       PolicyWhileInheritedConfigPendingBlocksUntilInheritedArrives) {
  SetProxy("a", "http://partition.example:8080");
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  params->initial_proxy_config.reset();
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());

  prefs_.SetManagedPref(proxy_config::prefs::kProxy, base::DictValue());
  service_->FlushForTesting();
  EXPECT_TRUE(client.IsBlocked());

  upstream->OnProxyConfigUpdated(ProxyConfig("http://managed.example:8080"));
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY managed.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest, SupervisedUserProxyTakesPrecedence) {
  SetProxy("a", "http://partition.example:8080");
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  prefs_.SetSupervisedUserPref(proxy_config::prefs::kProxy, base::DictValue());
  service_->FlushForTesting();
  EXPECT_EQ(client.ProxyFor(), "PROXY global.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       ExtensionProxyPrecedenceIsConfigurable) {
  service_->SetProxyConfig(Partition("override"),
                           ProxyConfig("http://override.example:8080"),
                           kOverride);
  service_->SetProxyConfig(Partition("yield"),
                           ProxyConfig("http://yield.example:8080"), kYield);
  mojo::Remote<network::mojom::ProxyConfigClient> upstream_o, upstream_y;
  auto params_o = Params(upstream_o);
  auto params_y = Params(upstream_y);
  Configure("override", params_o.get());
  Configure("yield", params_y.get());
  ProxyClient override_client, yield_client;
  override_client.Bind(params_o.get());
  yield_client.Bind(params_y.get());
  EXPECT_EQ(yield_client.ProxyFor(), "PROXY yield.example:8080");

  prefs_.SetExtensionPref(proxy_config::prefs::kProxy, base::DictValue());
  service_->FlushForTesting();
  EXPECT_EQ(override_client.ProxyFor(), "PROXY override.example:8080");
  EXPECT_EQ(yield_client.ProxyFor(), "PROXY global.example:8080");

  prefs_.RemoveExtensionPref(proxy_config::prefs::kProxy);
  service_->FlushForTesting();
  EXPECT_EQ(override_client.ProxyFor(), "PROXY override.example:8080");
  EXPECT_EQ(yield_client.ProxyFor(), "PROXY yield.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       CommandLineProxyPrecedenceIsConfigurable) {
  auto command_line_store = base::MakeRefCounted<TestingPrefStore>();
  PrefServiceFactory factory;
  factory.set_user_prefs(base::MakeRefCounted<TestingPrefStore>());
  factory.set_command_line_prefs(command_line_store);
  auto registry = base::MakeRefCounted<PrefRegistrySimple>();
  registry->RegisterDictionaryPref(proxy_config::prefs::kProxy);
  std::unique_ptr<PrefService> prefs = factory.Create(registry);
  StoragePartitionProxyService service(*prefs, local_state_,
                                       /*is_off_the_record=*/false);
  service.SetProxyConfig(Partition("override"),
                         ProxyConfig("http://override.example:8080"),
                         kOverride);
  service.SetProxyConfig(Partition("yield"),
                         ProxyConfig("http://yield.example:8080"), kYield);
  mojo::Remote<network::mojom::ProxyConfigClient> upstream_o, upstream_y;
  auto params_o = Params(upstream_o);
  auto params_y = Params(upstream_y);
  service.ConfigureNetworkContextParams(Partition("override"), params_o.get());
  service.ConfigureNetworkContextParams(Partition("yield"), params_y.get());
  ProxyClient override_client, yield_client;
  override_client.Bind(params_o.get());
  yield_client.Bind(params_y.get());

  command_line_store->SetValue(proxy_config::prefs::kProxy,
                               base::Value(base::DictValue()), 0);
  service.FlushForTesting();
  EXPECT_EQ(override_client.ProxyFor(), "PROXY override.example:8080");
  EXPECT_EQ(yield_client.ProxyFor(), "PROXY global.example:8080");
  service.Shutdown();
}

TEST_F(StoragePartitionProxyServiceTest,
       ExtensionOverrideRulesPrecedenceIsConfigurable) {
  service_->SetProxyConfig(Partition("override"),
                           ProxyConfig("http://override.example:8080"),
                           kOverride);
  service_->SetProxyConfig(Partition("yield"),
                           ProxyConfig("http://yield.example:8080"), kYield);
  net::ProxyConfig inherited;
  inherited.proxy_rules().ParseFromString("http://global.example:8080");
  inherited.set_proxy_override_rules({net::ProxyConfig::ProxyOverrideRule()});
  mojo::Remote<network::mojom::ProxyConfigClient> upstream_o, upstream_y;
  auto params_o = Params(upstream_o);
  auto params_y = Params(upstream_y);
  params_o->initial_proxy_config =
      net::ProxyConfigWithAnnotation(inherited, TRAFFIC_ANNOTATION_FOR_TESTS);
  params_y->initial_proxy_config = params_o->initial_proxy_config;
  Configure("override", params_o.get());
  Configure("yield", params_y.get());
  ProxyClient override_client, yield_client;
  override_client.Bind(params_o.get());
  yield_client.Bind(params_y.get());
  // Override rules from an unknown source are kept.
  EXPECT_EQ(override_client.ProxyFor(), "PROXY global.example:8080");
  EXPECT_EQ(yield_client.ProxyFor(), "PROXY global.example:8080");

  prefs_.SetExtensionPref(proxy_config::prefs::kProxyOverrideRules,
                          base::ListValue());
  service_->FlushForTesting();
  EXPECT_EQ(override_client.ProxyFor(), "PROXY override.example:8080");
  EXPECT_EQ(yield_client.ProxyFor(), "PROXY global.example:8080");
}

TEST_F(StoragePartitionProxyServiceTest,
       ObserverReceivesErrorsOnlyForActiveOverrides) {
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  ErrorClient inherited_errors;
  params->proxy_error_client =
      inherited_errors.receiver.BindNewPipeAndPassRemote();
  Configure("a", params.get());
  ProxyClient client;
  client.Bind(params.get());
  mojo::Remote<network::mojom::ProxyErrorClient> network_errors(
      std::move(params->proxy_error_client));
  ErrorObserver observer;
  service_->AddObserver(&observer);

  network_errors->OnPACScriptError(1, "inherited");
  network_errors->OnRequestMaybeFailedDueToProxySettings(
      net::ERR_PROXY_CONNECTION_FAILED);
  network_errors.FlushForTesting();
  inherited_errors.receiver.FlushForTesting();
  EXPECT_EQ(inherited_errors.pac_errors, 1);
  EXPECT_EQ(inherited_errors.request_errors, 1);
  EXPECT_TRUE(observer.pac_errors.empty());
  EXPECT_TRUE(observer.request_errors.empty());

  SetProxy("a", "http://partition.example:8080");
  service_->FlushForTesting();
  network_errors->OnPACScriptError(2, "override");
  network_errors->OnRequestMaybeFailedDueToProxySettings(
      net::ERR_TUNNEL_CONNECTION_FAILED);
  network_errors.FlushForTesting();
  inherited_errors.receiver.FlushForTesting();
  EXPECT_EQ(inherited_errors.pac_errors, 1);
  EXPECT_EQ(inherited_errors.request_errors, 1);
  ASSERT_EQ(observer.pac_errors.size(), 1u);
  EXPECT_EQ(observer.pac_errors[0],
            std::make_tuple(Partition("a"), 2, std::string("override")));
  ASSERT_EQ(observer.request_errors.size(), 1u);
  EXPECT_EQ(observer.request_errors[0],
            std::make_tuple(Partition("a"),
                            int32_t{net::ERR_TUNNEL_CONNECTION_FAILED}));
  service_->RemoveObserver(&observer);
}

TEST_F(StoragePartitionProxyServiceTest,
       ObserverReceivesErrorsWithoutInheritedErrorClient) {
  SetProxy("a", "http://partition.example:8080");
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  Configure("a", params.get(), /*in_memory=*/true);
  ProxyClient client;
  client.Bind(params.get());
  ASSERT_TRUE(params->proxy_error_client);
  mojo::Remote<network::mojom::ProxyErrorClient> network_errors(
      std::move(params->proxy_error_client));
  ErrorObserver observer;
  service_->AddObserver(&observer);

  // The in-memory partition has no override, so there is nowhere to report.
  network_errors->OnRequestMaybeFailedDueToProxySettings(
      net::ERR_PROXY_CONNECTION_FAILED);
  network_errors.FlushForTesting();
  EXPECT_TRUE(observer.request_errors.empty());

  service_->SetProxyConfig(Partition("a", true),
                           ProxyConfig("http://memory.example:8080"),
                           kOverride);
  service_->FlushForTesting();
  network_errors->OnRequestMaybeFailedDueToProxySettings(
      net::ERR_PROXY_CONNECTION_FAILED);
  network_errors.FlushForTesting();
  ASSERT_EQ(observer.request_errors.size(), 1u);
  EXPECT_EQ(observer.request_errors[0],
            std::make_tuple(Partition("a", true),
                            int32_t{net::ERR_PROXY_CONNECTION_FAILED}));
  service_->RemoveObserver(&observer);
}

TEST_F(StoragePartitionProxyServiceTest,
       OffTheRecordServiceRejectsOnDiskPartitions) {
  StoragePartitionProxyService service(prefs_, local_state_,
                                       /*is_off_the_record=*/true);
  service.SetProxyConfig(Partition("a", /*in_memory=*/true),
                         ProxyConfig("http://a.example:8080"), kOverride);
  EXPECT_TRUE(service.GetProxyConfig(Partition("a", /*in_memory=*/true)));
  mojo::Remote<network::mojom::ProxyConfigClient> upstream;
  auto params = Params(upstream);
  EXPECT_CHECK_DEATH(service.SetProxyConfig(
      Partition("a"), ProxyConfig("http://a.example:8080"), kOverride));
  EXPECT_CHECK_DEATH(service.ClearProxyConfig(Partition("a")));
  EXPECT_CHECK_DEATH(service.GetProxyConfig(Partition("a")));
  EXPECT_CHECK_DEATH(
      service.ConfigureNetworkContextParams(Partition("a"), params.get()));
  service.Shutdown();
}

}  // namespace brave
