/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include "brave/browser/ipfs/ipfs_host_resolver.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
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

class IpfsTabHelperBrowserTest : public InProcessBrowserTest {
 public:
  IpfsTabHelperBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    embedded_test_server()->ServeFilesFromSourceDirectory("content/test/data");
    https_server_.ServeFilesFromSourceDirectory("content/test/data");
    embedded_test_server()->RegisterRequestHandler(base::BindRepeating(
        &IpfsTabHelperBrowserTest::ResponseHandler, base::Unretained(this)));

    https_server_.RegisterRequestHandler(base::BindRepeating(
        &IpfsTabHelperBrowserTest::ResponseHandler, base::Unretained(this)));
    ASSERT_TRUE(https_server_.Start());
    ASSERT_TRUE(embedded_test_server()->Start());
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
    if (code_ == net::HTTP_OK)
      http_response->AddCustomHeader("x-ipfs-path", x_ipfs_path_);
    return std::move(http_response);
  }

  void SetXIpfsPathHeader(const std::string& value) { x_ipfs_path_ = value; }

  void SetHttpStatusCode(net::HttpStatusCode code) { code_ = code; }

  net::HttpStatusCode code_ = net::HTTP_OK;
  std::string x_ipfs_path_;
  net::EmbeddedTestServer https_server_;
};

class FakeIPFSHostResolver : public ipfs::IPFSHostResolver {
 public:
  explicit FakeIPFSHostResolver(network::mojom::NetworkContext* context)
      : ipfs::IPFSHostResolver(context) {}
  ~FakeIPFSHostResolver() override {}
  void Resolve(const net::HostPortPair& host,
               const net::NetworkIsolationKey& isolation_key,
               net::DnsQueryType dns_query_type,
               HostTextResultsCallback callback) override {
    resolve_called_++;
    if (callback)
      std::move(callback).Run(host.host(), dnslink_);
  }

  bool resolve_called() const { return resolve_called_ == 1; }

  void SetDNSLinkToResopnd(const std::string& dnslink) { dnslink_ = dnslink; }

 private:
  int resolve_called_ = 0;
  std::string dnslink_;
};

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolvedIPFSLinkLocal) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      active_contents()->GetBrowserContext());
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));

  SetXIpfsPathHeader("/ipfs/bafybeiemx/empty.html");
  GURL test_url = https_server_.GetURL("/empty.html?query#ref");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  std::string result = "ipfs://bafybeiemx/empty.html?query#ref";
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), result);

  test_url = https_server_.GetURL("/another.html?query#ref");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  result = "ipfs://bafybeiemx/another.html?query#ref";
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), result);

  SetXIpfsPathHeader("/ipns/brave.eth/empty.html");
  test_url = https_server_.GetURL("/?query#ref");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  result = "ipns://brave.eth/?query#ref";
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), result);

  SetXIpfsPathHeader("/ipfs/bafy");
  test_url = embedded_test_server()->GetURL(
      "a.com", "/ipfs/bafy/wiki/empty.html?query#ref");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  result = "ipfs://bafy/wiki/empty.html?query#ref";
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), result);

  SetXIpfsPathHeader("/ipns/bafyb");
  test_url = embedded_test_server()->GetURL(
      "a.com", "/ipns/bafyb/wiki/empty.html?query#ref");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  result = "ipns://bafyb/wiki/empty.html?query#ref";
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), result);
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolvedIPFSLinkGateway) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      active_contents()->GetBrowserContext());
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  SetXIpfsPathHeader("/ipfs/bafybeiemx/empty.html");
  const GURL test_url = https_server_.GetURL("/empty.html");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  GURL ipns = ReplaceScheme(test_url, ipfs::kIPNSScheme);
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(),
            "ipfs://bafybeiemx/empty.html");
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, NoResolveIPFSLinkCalledMode) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      active_contents()->GetBrowserContext());
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK));
  SetXIpfsPathHeader("/ipfs/bafybeiemx/empty.html");
  GURL test_url = https_server_.GetURL("/empty.html");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());

  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED));

  ui_test_utils::NavigateToURL(browser(), test_url);
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
  if (!helper)
    return;
  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      active_contents()->GetBrowserContext());
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));

  GURL test_url = embedded_test_server()->GetURL("/empty.html");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolveIPFSLinkCalled5xx) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      active_contents()->GetBrowserContext());
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  resolver_raw->SetDNSLinkToResopnd("/ipfs/QmXoypiz");
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
  ASSERT_FALSE(resolver_raw->resolve_called());
  SetHttpStatusCode(net::HTTP_INTERNAL_SERVER_ERROR);
  const GURL test_url =
      embedded_test_server()->GetURL("a.com", "/5xx.html?query#fragment");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_FALSE(WaitForLoadStop(active_contents()));
  ASSERT_TRUE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL(),
            GURL("ipns://a.com/5xx.html?query#fragment"));
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolveNotCalled5xx) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      active_contents()->GetBrowserContext());
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  SetHttpStatusCode(net::HTTP_INTERNAL_SERVER_ERROR);
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK));
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
  ASSERT_FALSE(resolver_raw->resolve_called());
  const GURL test_url = https_server_.GetURL("/5xx.html");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_FALSE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), std::string());
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolvedIPFSLinkBad) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  auto* storage_partition = content::BrowserContext::GetDefaultStoragePartition(
      active_contents()->GetBrowserContext());
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));

  SetXIpfsPathHeader("/http/bafybeiemx/empty.html");
  const GURL test_url = https_server_.GetURL("/empty.html");
  ui_test_utils::NavigateToURL(browser(), test_url);
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  std::string result = "";
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), result);
}
