/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/ipfs/ipfs_host_resolver.h"
#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/controllable_http_response.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

using content::NavigationHandle;
using content::WebContents;

namespace {
constexpr char kCid1[] =
    "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqabf3oclgtqy55fbzdi";
constexpr char kClientSideRedirectResponsePath[] = "/client_side_redirect.html";
}  // namespace

namespace ipfs {
class IPFSTabHelperTest : public IPFSTabHelper {
 public:
  explicit IPFSTabHelperTest(content::WebContents* web_contents)
      : IPFSTabHelper(web_contents) {}

  bool IsResolveMethod(
      const ipfs::IPFSResolveMethodTypes& resolution_method) override {
    return resolution_method == ipfs::IPFSResolveMethodTypes::IPFS_LOCAL;
  }
};
}  // namespace ipfs

class IpfsTabHelperBrowserTest : public InProcessBrowserTest {
 public:
  IpfsTabHelperBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    test_data_dir_ = base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    embedded_test_server()->ServeFilesFromSourceDirectory("content/test/data");
    https_server_.ServeFilesFromSourceDirectory("content/test/data");
    embedded_test_server()->RegisterRequestHandler(base::BindRepeating(
        &IpfsTabHelperBrowserTest::ResponseHandler, base::Unretained(this)));

    https_server_.RegisterRequestHandler(base::BindRepeating(
        &IpfsTabHelperBrowserTest::ResponseHandler, base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());
    ASSERT_TRUE(embedded_test_server()->Start());
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  GURL ReplaceScheme(const GURL& current, const std::string& new_scheme) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(new_scheme);
    return current.ReplaceComponents(replacements);
  }

  std::unique_ptr<net::test_server::HttpResponse> ResponseHandler(
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse);

    http_response->set_code(code_);
    if (base::StartsWith(request.relative_url,
                         kClientSideRedirectResponsePath)) {
      http_response->set_content(GetFileContent("client_side_redirect.html"));
    }
    if (code_ == net::HTTP_OK && !x_ipfs_path_.empty()) {
      http_response->AddCustomHeader("x-ipfs-path", x_ipfs_path_);
    }
    return std::move(http_response);
  }

  void SetXIpfsPathHeader(const std::string& value) { x_ipfs_path_ = value; }

  void SetHttpStatusCode(net::HttpStatusCode code) { code_ = code; }

  ipfs::IPFSTabHelper* SetIPFSTabHelperTest() {
    active_contents()->SetUserData(
        content::WebContentsUserData<ipfs::IPFSTabHelper>::UserDataKey(),
        std::make_unique<ipfs::IPFSTabHelperTest>(active_contents()));
    auto* helper = ipfs::IPFSTabHelper::FromWebContents(active_contents());
    EXPECT_TRUE(
        helper->IsResolveMethod(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
    return helper;
  }

  const base::FilePath& test_data_dir() const { return test_data_dir_; }
  std::string GetFileContent(const std::string& filename) const {
    std::string result;

    const auto full_path = test_data_dir_.AppendASCII(filename);
    [&]() { ASSERT_TRUE(base::ReadFileToString(full_path, &result)); }();
    return result;
  }

  net::HttpStatusCode code_ = net::HTTP_OK;
  std::string x_ipfs_path_;
  net::EmbeddedTestServer https_server_;

 private:
  base::FilePath test_data_dir_;
};

class FakeIPFSHostResolver : public ipfs::IPFSHostResolver {
 public:
  FakeIPFSHostResolver() : ipfs::IPFSHostResolver(nullptr) {}
  ~FakeIPFSHostResolver() override = default;
  void Resolve(const net::HostPortPair& host,
               const net::NetworkAnonymizationKey& anonymization_key,
               net::DnsQueryType dns_query_type,
               HostTextResultsCallback callback) override {
    resolve_called_ = true;
    if (callback)
      std::move(callback).Run(host.host(), dnslink_);
  }

  void ResetResolveCalled() { resolve_called_ = false; }
  bool resolve_called() const { return resolve_called_; }

  void SetDNSLinkToRespond(const std::string& dnslink) { dnslink_ = dnslink; }

 private:
  bool resolve_called_ = false;
  std::string dnslink_;
};

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolvedIPFSLinkLocal) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
  GURL gateway = ipfs::GetConfiguredBaseGateway(prefs, chrome::GetChannel());

  // X-IPFS-Path header and no DNSLink
  SetXIpfsPathHeader("/ipfs/bafybeiemx/empty.html");
  GURL test_url = https_server_.GetURL("/empty.html?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());
  auto resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url, GURL("ipfs://bafybeiemx/empty.html?query#ref"));

  resolver_raw->ResetResolveCalled();
  test_url = https_server_.GetURL("/another.html?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());
  resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url, GURL("ipfs://bafybeiemx/empty.html?query#ref"));

  resolver_raw->ResetResolveCalled();
  SetXIpfsPathHeader("/ipns/brave.eth/empty.html");
  test_url = https_server_.GetURL("/?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());
  resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url, GURL("ipns://brave.eth/empty.html?query#ref"));

  resolver_raw->ResetResolveCalled();
  SetXIpfsPathHeader("/ipfs/bafy");
  test_url =
      embedded_test_server()->GetURL("a.com", "/wiki/empty.html?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());
  resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url, GURL("ipfs://bafy?query#ref"));

  resolver_raw->ResetResolveCalled();
  SetXIpfsPathHeader("/ipns/bafyb");
  test_url = embedded_test_server()->GetURL(
      "a.com", "/ipns/bafyb/wiki/empty.html?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());
  resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url, GURL("ipns://bafyb?query#ref"));

  resolver_raw->ResetResolveCalled();
  SetXIpfsPathHeader("/ipfs/bafy");
  test_url = embedded_test_server()->GetURL(
      "a.com", base::StringPrintf("/ipfs/%s/wiki/"
                                  "empty.html?query#ref",
                                  kCid1));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url, GURL(base::StringPrintf(
                              "ipfs://%s/wiki/empty.html?query#ref", kCid1)));
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolvedIPFSLinkGateway) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  SetXIpfsPathHeader("/ipfs/bafybeiemx/empty.html");
  const GURL test_url = https_server_.GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(),
            "ipfs://bafybeiemx/empty.html");
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, NoResolveIPFSLinkCalledMode) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED));
  SetXIpfsPathHeader("/ipfs/bafybeiemx/empty.html");
  GURL test_url = https_server_.GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());

  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest,
                       NoResolveIPFSLinkCalledHeader) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));

  GURL test_url = embedded_test_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest,
                       GatewayRedirectToIPFS_DontRedirectConfiguredGateway) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());

//   prefs->SetBoolean(kIPFSAutoRedirectToConfiguredGateway, true);
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  GURL gateway_url = embedded_test_server()->GetURL("a.com", "/");
  prefs->SetString(kIPFSPublicGatewayAddress, gateway_url.spec());

  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());

  const GURL test_url = embedded_test_server()->GetURL(
      "a.com", base::StringPrintf("/ipfs/%s/wiki/"
                                  "empty.html?query#ref",
                                  kCid1));

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());

  EXPECT_EQ(active_contents()->GetURL(), test_url);
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, GatewayRedirectToIPFS) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  resolver_raw->SetDNSLinkToRespond("/ipfs/QmXoypiz");
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());

//   prefs->SetBoolean(kIPFSAutoRedirectToConfiguredGateway, true);
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
  ASSERT_FALSE(resolver_raw->resolve_called());
  SetXIpfsPathHeader("/ipns/other");

  const GURL test_url = embedded_test_server()->GetURL(
      "navigate_to.com", base::StringPrintf("/ipfs/%s/wiki/"
                                            "empty.html?query#ref",
                                            kCid1));

  GURL gateway_url = embedded_test_server()->GetURL("a.com", "/");
  prefs->SetString(kIPFSPublicGatewayAddress, gateway_url.spec());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());

  // gateway url.
  GURL expected_final_url;
  ipfs::TranslateIPFSURI(GURL(base::StringPrintf("ipfs://%s/"
                                                 "wiki/empty.html?query#ref",
                                                 kCid1)),
                         &expected_final_url, gateway_url, false);

  EXPECT_EQ(active_contents()->GetVisibleURL(), expected_final_url);
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest,
                       GatewayRedirectToIPND_LibP2PKey) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  resolver_raw->SetDNSLinkToRespond("/ipfs/QmXoypiz");
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());

//   prefs->SetBoolean(kIPFSAutoRedirectToConfiguredGateway, true);
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
  ASSERT_FALSE(resolver_raw->resolve_called());
  SetXIpfsPathHeader("/ipns/other");

  const GURL test_url = embedded_test_server()->GetURL(
      "navigate_to.com",
      "/ipns/k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v8/"
      "wiki/empty.html?query#ref");

  GURL gateway_url = embedded_test_server()->GetURL("a.com", "/");
  prefs->SetString(kIPFSPublicGatewayAddress, gateway_url.spec());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());

  // gateway url.
  GURL expected_final_url;
  ipfs::TranslateIPFSURI(GURL("ipns://"
                              "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyq"
                              "j2xjonzllnv0v8/wiki/empty.html?query#ref"),
                         &expected_final_url, gateway_url, false);

  EXPECT_EQ(active_contents()->GetVisibleURL(), expected_final_url);
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, GatewayRedirectToIPNS) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  resolver_raw->SetDNSLinkToRespond("/ipns/QmXoypiz");
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());

//   prefs->SetBoolean(kIPFSAutoRedirectToConfiguredGateway, true);
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
  ASSERT_FALSE(resolver_raw->resolve_called());
  SetXIpfsPathHeader("/ipns/other");

  const GURL test_url = embedded_test_server()->GetURL(
      "navigate_to.com",
      "/ipns/en-wikiepdia--on--ipfs-org/wiki/empty.html?query#ref");

  GURL gateway_url = embedded_test_server()->GetURL("a.com", "/");
  prefs->SetString(kIPFSPublicGatewayAddress, gateway_url.spec());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());

  // gateway url.
  GURL expected_final_url;
  ipfs::TranslateIPFSURI(
      GURL("ipns://en.wikiepdia-on-ipfs.org/wiki/empty.html?query#ref"),
      &expected_final_url, gateway_url, false);

  EXPECT_EQ(active_contents()->GetVisibleURL(), expected_final_url);
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolveIPFSLinkCalled5xx) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  resolver_raw->SetDNSLinkToRespond("/ipfs/QmXoypiz");
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
  ASSERT_FALSE(resolver_raw->resolve_called());
  SetHttpStatusCode(net::HTTP_INTERNAL_SERVER_ERROR);
  const GURL test_url = https_server_.GetURL("/5xx.html?query#fragment");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_FALSE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());
  GURL ipns = ReplaceScheme(test_url, ipfs::kIPNSScheme);
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), ipns);
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolveNotCalled5xx) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  SetHttpStatusCode(net::HTTP_INTERNAL_SERVER_ERROR);
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED));
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
  ASSERT_FALSE(resolver_raw->resolve_called());
  const GURL test_url = https_server_.GetURL("/5xx.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_FALSE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolvedIPFSLinkBad) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());

  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  helper->SetResolverForTesting(std::move(resolver));

  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));

  SetXIpfsPathHeader("");
  resolver_raw->SetDNSLinkToRespond("");

  const GURL test_url = https_server_.GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  std::string result = "";
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), result);
}

// Some tests are failing for Windows x86 CI,
// See https://github.com/brave/brave-browser/issues/22767
#if BUILDFLAG(IS_WIN) && defined(ARCH_CPU_X86)
#define MAYBE_ResolvedIPFSLinkBackward DISABLED_ResolvedIPFSLinkBackward
#else
#define MAYBE_ResolvedIPFSLinkBackward ResolvedIPFSLinkBackward
#endif
IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest,
                       MAYBE_ResolvedIPFSLinkBackward) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  ASSERT_TRUE(helper);
  auto* network_context = active_contents()
                              ->GetBrowserContext()
                              ->GetDefaultStoragePartition()
                              ->GetNetworkContext();
  ASSERT_TRUE(network_context);
  std::unique_ptr<FakeIPFSHostResolver> resolver(new FakeIPFSHostResolver());
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetNetworkContextForTesting(network_context);
  helper->SetResolverForTesting(std::move(resolver));
  std::string ipfs_path = "/ipfs/bafybeiemx/";
  SetXIpfsPathHeader(ipfs_path);
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());

//   prefs->SetBoolean(kIPFSAutoRedirectToConfiguredGateway, true);
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

  // Navigation with x-ipfs-path header and valid _dnslink record redirects to
  // ipns:// url
  GURL test_url = embedded_test_server()->GetURL("navigate_to.com",
                                                 "/empty.html?query#ref");
  GURL gateway_url = embedded_test_server()->GetURL("a.com", "/");
  resolver_raw->SetDNSLinkToRespond("/ipns/a.com/");

  prefs->SetString(kIPFSPublicGatewayAddress, gateway_url.spec());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());

  GURL::Replacements scheme_replacements;
  scheme_replacements.SetSchemeStr(ipfs::kIPNSScheme);

  // Url will be translated to ipns:// scheme which will be translated to
  // gateway url.
  GURL expected_final_url;
  ipfs::TranslateIPFSURI(test_url.ReplaceComponents(scheme_replacements),
                         &expected_final_url, gateway_url, false);

  EXPECT_EQ(active_contents()->GetVisibleURL(), expected_final_url);

  // Second one navigation also succeed
  GURL another_test_url =
      embedded_test_server()->GetURL("/another.html?query#ref");

  resolver_raw->ResetResolveCalled();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), another_test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());

  // Url will be translated to ipns:// scheme which will be translated to
  // gateway url.
  GURL expected_second_final_url;
  ipfs::TranslateIPFSURI(
      another_test_url.ReplaceComponents(scheme_replacements),
      &expected_second_final_url, gateway_url, false);

  EXPECT_EQ(active_contents()->GetVisibleURL().spec(),
            expected_second_final_url);

  // Backward navigation also succeed
  active_contents()->GetController().GoBack();
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(active_contents()->GetVisibleURL(), expected_final_url);
}

