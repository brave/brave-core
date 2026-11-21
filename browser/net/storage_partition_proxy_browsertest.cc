/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/test/test_future.h"
#include "brave/browser/net/storage_partition_proxy_service.h"
#include "brave/browser/net/storage_partition_proxy_service_factory.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/net/profile_network_context_service.h"
#include "chrome/browser/net/profile_network_context_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/load_flags.h"
#include "net/dns/mock_host_resolver.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/tor/tor_profile_manager.h"
#endif

namespace brave {
namespace {

constexpr char kPartitionDomain[] = "proxytest";

std::unique_ptr<net::test_server::HttpResponse> Response(
    const std::string& label,
    const net::test_server::HttpRequest& request) {
  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->AddCustomHeader("Cache-Control", "no-store");
  if (request.GetURL().path() == "/worker.js") {
    response->set_content_type("application/javascript");
    response->set_content(
        "onmessage = async () => {"
        "  postMessage(await (await fetch('/label')).text());"
        "};");
  } else {
    response->set_content_type("text/html");
    response->set_content(label);
  }
  return response;
}

}  // namespace

class StoragePartitionProxyBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("proxy-target.test", "127.0.0.1");
    origin_.RegisterRequestHandler(base::BindRepeating(&Response, "direct"));
    proxy_a_.RegisterRequestHandler(base::BindRepeating(&Response, "proxy-a"));
    proxy_b_.RegisterRequestHandler(base::BindRepeating(&Response, "proxy-b"));
    proxy_c_.RegisterRequestHandler(base::BindRepeating(&Response, "proxy-c"));
    ASSERT_TRUE(origin_.Start());
    ASSERT_TRUE(proxy_a_.Start());
    ASSERT_TRUE(proxy_b_.Start());
    ASSERT_TRUE(proxy_c_.Start());
    SetGlobalProxy(proxy_a_);
  }

  void SetGlobalProxy(const net::EmbeddedTestServer& proxy) {
    browser()->GetProfile()->GetPrefs()->SetDict(
        proxy_config::prefs::kProxy,
        ProxyConfigDictionary::CreateFixedServers(
            "http://" + proxy.host_port_pair().ToString(), ""));
    Flush();
  }

  void SetConfig(const content::StoragePartitionConfig& partition_config,
                 const net::ProxyConfigWithAnnotation& config,
                 Profile* profile = nullptr) {
    profile = profile ? profile : browser()->GetProfile();
    StoragePartitionProxyServiceFactory::GetForProfile(profile)->SetProxyConfig(
        partition_config, config,
        StoragePartitionProxyService::Precedence::
            kOverrideExtensionAndCommandLine);
    Flush(profile);
  }

  void SetConfig(content::StoragePartition* partition,
                 const net::ProxyConfigWithAnnotation& config,
                 Profile* profile = nullptr) {
    SetConfig(partition->GetConfig(), config, profile);
  }

  void SetProxy(const content::StoragePartitionConfig& partition_config,
                const net::EmbeddedTestServer& proxy,
                Profile* profile = nullptr) {
    net::ProxyConfig config;
    config.proxy_rules().ParseFromString("http://" +
                                         proxy.host_port_pair().ToString());
    SetConfig(
        partition_config,
        net::ProxyConfigWithAnnotation(config, TRAFFIC_ANNOTATION_FOR_TESTS),
        profile);
  }

  void SetProxy(content::StoragePartition* partition,
                const net::EmbeddedTestServer& proxy,
                Profile* profile = nullptr) {
    SetProxy(partition->GetConfig(), proxy, profile);
  }

  void ClearProxy(content::StoragePartition* partition) {
    auto* profile = browser()->GetProfile();
    StoragePartitionProxyServiceFactory::GetForProfile(profile)
        ->ClearProxyConfig(partition->GetConfig());
    Flush(profile);
  }

  void Flush(Profile* profile = nullptr) {
    profile = profile ? profile : browser()->GetProfile();
    ProfileNetworkContextServiceFactory::GetForContext(profile)
        ->FlushProxyConfigMonitorForTesting();
    StoragePartitionProxyServiceFactory::GetForProfile(profile)
        ->FlushForTesting();
  }

  content::StoragePartitionConfig Config(
      const char* id,
      Profile* profile = nullptr,
      bool in_memory = false,
      const char* domain = kPartitionDomain) {
    profile = profile ? profile : browser()->GetProfile();
    return content::StoragePartitionConfig::Create(profile, domain, id,
                                                   in_memory);
  }

  content::StoragePartition* Partition(const char* id,
                                       Profile* profile = nullptr,
                                       bool in_memory = false,
                                       const char* domain = kPartitionDomain) {
    profile = profile ? profile : browser()->GetProfile();
    return profile->GetStoragePartition(Config(id, profile, in_memory, domain));
  }

  std::optional<std::string> Fetch(content::StoragePartition* partition) {
    auto request = std::make_unique<network::ResourceRequest>();
    request->url = origin_.GetURL("proxy-target.test", "/label");
    request->load_flags = net::LOAD_DISABLE_CACHE;
    auto loader = network::SimpleURLLoader::Create(
        std::move(request), TRAFFIC_ANNOTATION_FOR_TESTS);
    base::test::TestFuture<std::optional<std::string>> body;
    loader->DownloadToString(
        partition->GetURLLoaderFactoryForBrowserProcess().get(),
        body.GetCallback(), 1024);
    return body.Take();
  }

  net::EmbeddedTestServer origin_, proxy_a_, proxy_b_, proxy_c_;
};

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       RoutesAndUpdatesPerPartition) {
  auto* custom = Partition("custom");
  SetProxy(custom, proxy_b_);
  auto* inherited = Partition("inherit");
  auto* ordinary = browser()->GetProfile()->GetDefaultStoragePartition();
  EXPECT_EQ(Fetch(custom), "proxy-b");
  EXPECT_EQ(Fetch(inherited), "proxy-a");
  EXPECT_EQ(Fetch(ordinary), "proxy-a");

  SetGlobalProxy(proxy_b_);
  EXPECT_EQ(Fetch(inherited), "proxy-b");
  SetConfig(custom, net::ProxyConfigWithAnnotation::CreateDirect());
  EXPECT_EQ(Fetch(custom), "direct");
  EXPECT_EQ(Fetch(ordinary), "proxy-b");
  ClearProxy(custom);
  EXPECT_EQ(Fetch(custom), "proxy-b");
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       MultiplePartitionsUseDifferentProxies) {
  auto* first = Partition("first");
  auto* second = Partition("second");
  auto* first_in_memory = Partition("first", nullptr, /*in_memory=*/true);
  auto* inherited = Partition("inherit");
  auto* ordinary = browser()->GetProfile()->GetDefaultStoragePartition();
  SetProxy(first, proxy_b_);
  SetProxy(second, proxy_c_);
  SetConfig(first_in_memory, net::ProxyConfigWithAnnotation::CreateDirect());
  EXPECT_EQ(Fetch(first), "proxy-b");
  EXPECT_EQ(Fetch(second), "proxy-c");
  EXPECT_EQ(Fetch(first_in_memory), "direct");
  EXPECT_EQ(Fetch(inherited), "proxy-a");
  EXPECT_EQ(Fetch(ordinary), "proxy-a");

  // Swapping proxies updates each partition independently.
  SetProxy(first, proxy_c_);
  SetProxy(second, proxy_b_);
  EXPECT_EQ(Fetch(first), "proxy-c");
  EXPECT_EQ(Fetch(second), "proxy-b");
  EXPECT_EQ(Fetch(first_in_memory), "direct");

  // Profile updates only reach inheriting partitions.
  SetGlobalProxy(proxy_b_);
  EXPECT_EQ(Fetch(first), "proxy-c");
  EXPECT_EQ(Fetch(inherited), "proxy-b");
  EXPECT_EQ(Fetch(ordinary), "proxy-b");

  ClearProxy(first);
  EXPECT_EQ(Fetch(first), "proxy-b");
  EXPECT_EQ(Fetch(second), "proxy-b");
  EXPECT_EQ(Fetch(first_in_memory), "direct");
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       OverrideAppliesToPartitionCreatedLater) {
  const auto config = Config("later");
  ASSERT_FALSE(browser()->GetProfile()->GetStoragePartition(
      config, /*can_create=*/false));
  SetProxy(config, proxy_b_);
  EXPECT_EQ(Fetch(Partition("later")), "proxy-b");
  EXPECT_EQ(Fetch(Partition("other")), "proxy-a");
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       BlobUrlFallbackVariantsShareOverride) {
  // Disk-backed variants would contend for the same on-disk databases.
  auto fallback_config = Config("blob", nullptr, /*in_memory=*/true);
  fallback_config.set_fallback_to_partition_domain_for_blob_urls(
      content::StoragePartitionConfig::FallbackMode::
          kFallbackPartitionInMemory);
  auto* partition = Partition("blob", nullptr, /*in_memory=*/true);
  auto* fallback_partition =
      browser()->GetProfile()->GetStoragePartition(fallback_config);
  // Content keeps a separate partition object for each fallback mode.
  ASSERT_NE(partition, fallback_partition);
  SetProxy(partition, proxy_b_);
  EXPECT_EQ(Fetch(partition), "proxy-b");
  EXPECT_EQ(Fetch(fallback_partition), "proxy-b");
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       SurvivesNetworkServiceCrash) {
  auto* partition = Partition("custom");
  SetProxy(partition, proxy_b_);
  ASSERT_EQ(Fetch(partition), "proxy-b");
  SimulateNetworkServiceCrash();
  EXPECT_EQ(Fetch(partition), "proxy-b");
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       PrivateSettingsStaySeparate) {
  auto* regular = Partition("custom");
  SetProxy(regular, proxy_a_);
  auto* private_profile =
      browser()->GetProfile()->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  auto* private_partition = Partition("custom", private_profile);
  SetProxy(private_partition, proxy_b_, private_profile);
  EXPECT_EQ(Fetch(regular), "proxy-a");
  EXPECT_EQ(Fetch(private_partition), "proxy-b");
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       InMemoryAndDiskPartitionsStaySeparate) {
  auto* disk = Partition("same");
  auto* memory = Partition("same", nullptr, true);
  ASSERT_EQ(disk->GetPath(), memory->GetPath());
  SetProxy(disk, proxy_a_);
  SetProxy(memory, proxy_b_);
  EXPECT_EQ(Fetch(disk), "proxy-a");
  EXPECT_EQ(Fetch(memory), "proxy-b");
  ClearProxy(memory);
  EXPECT_EQ(Fetch(memory), "proxy-a");
  EXPECT_EQ(Fetch(disk), "proxy-a");
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       PartitionDomainsStaySeparate) {
  auto* a = Partition("same", nullptr, false, "domaina");
  auto* b = Partition("same", nullptr, false, "domainb");
  SetProxy(a, proxy_b_);
  EXPECT_EQ(Fetch(a), "proxy-b");
  EXPECT_EQ(Fetch(b), "proxy-a");
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       DefaultPartitionSupportsOverrides) {
  auto* partition = browser()->GetProfile()->GetDefaultStoragePartition();
  SetProxy(partition, proxy_b_);
  EXPECT_EQ(Fetch(partition), "proxy-b");
  ClearProxy(partition);
  EXPECT_EQ(Fetch(partition), "proxy-a");
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       WorkerRequestsUseProxy) {
  SetProxy(Partition("worker"), proxy_b_);
  NavigateParams params(browser(), origin_.GetURL("proxy-target.test", "/page"),
                        ui::PAGE_TRANSITION_TYPED);
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->GetProfile(), kPartitionDomain, "worker", false);
  params.disposition = WindowOpenDisposition::kNewForegroundTab;
  Navigate(&params);
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(content::WaitForLoadStop(contents));
  EXPECT_EQ("proxy-b", content::EvalJs(contents, R"(
    new Promise((resolve, reject) => {
      const worker = new Worker('/worker.js');
      worker.onmessage = event => {
        worker.terminate();
        resolve(event.data);
      };
      worker.onerror = reject;
      worker.postMessage('fetch');
    })
  )"));
}

IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       EmptyProxyListBlocksRequests) {
  net::ProxyConfig blocked;
  blocked.proxy_rules().type = net::ProxyConfig::ProxyRules::Type::PROXY_LIST;
  blocked.proxy_rules().bypass_rules.AddRulesToSubtractImplicit();
  auto* partition = Partition("blocked");
  SetConfig(partition, net::ProxyConfigWithAnnotation(
                           blocked, TRAFFIC_ANNOTATION_FOR_TESTS));
  EXPECT_FALSE(Fetch(partition));
}

#if BUILDFLAG(ENABLE_TOR)
IN_PROC_BROWSER_TEST_F(StoragePartitionProxyBrowserTest,
                       TorProfilesHaveNoService) {
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(browser()->GetProfile());
  ASSERT_TRUE(tor_profile->IsTor());
  // Creating a partition configures its network context through the factory.
  Partition("custom", tor_profile);
  EXPECT_FALSE(StoragePartitionProxyServiceFactory::GetForProfile(tor_profile));
}
#endif  // BUILDFLAG(ENABLE_TOR)

}  // namespace brave
