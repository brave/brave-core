/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include "brave/browser/ipfs/ipfs_host_resolver.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/channel_info.h"
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
  ~FakeIPFSHostResolver() override = default;
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
  auto* storage_partition =
      active_contents()->GetBrowserContext()->GetDefaultStoragePartition();
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
  GURL gateway = ipfs::GetConfiguredBaseGateway(prefs, chrome::GetChannel());

  SetXIpfsPathHeader("/ipfs/bafybeiemx/empty.html");
  GURL test_url = https_server_.GetURL("/empty.html?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  auto resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url.host(), gateway.host());
  EXPECT_EQ(resolved_url.path(), "/ipns/" + test_url.host() + "/empty.html");
  EXPECT_EQ(resolved_url.query(), "query");
  EXPECT_EQ(resolved_url.ref(), "ref");

  test_url = https_server_.GetURL("/another.html?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url.host(), gateway.host());
  EXPECT_EQ(resolved_url.path(), "/ipns/" + test_url.host() + "/another.html");
  EXPECT_EQ(resolved_url.query(), "query");
  EXPECT_EQ(resolved_url.ref(), "ref");

  SetXIpfsPathHeader("/ipns/brave.eth/empty.html");
  test_url = https_server_.GetURL("/?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url.host(), gateway.host());
  EXPECT_EQ(resolved_url.path(), "/ipns/" + test_url.host() + "/");
  EXPECT_EQ(resolved_url.query(), "query");
  EXPECT_EQ(resolved_url.ref(), "ref");

  SetXIpfsPathHeader("/ipfs/bafy");
  test_url = embedded_test_server()->GetURL(
      "a.com", "/ipfs/bafy/wiki/empty.html?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url.host(), gateway.host());
  EXPECT_EQ(resolved_url.path(),
            "/ipns/" + test_url.host() + "/ipfs/bafy/wiki/empty.html");
  EXPECT_EQ(resolved_url.query(), "query");
  EXPECT_EQ(resolved_url.ref(), "ref");

  SetXIpfsPathHeader("/ipns/bafyb");
  test_url = embedded_test_server()->GetURL(
      "a.com", "/ipns/bafyb/wiki/empty.html?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  resolved_url = helper->GetIPFSResolvedURL();
  EXPECT_EQ(resolved_url.host(), gateway.host());
  EXPECT_EQ(resolved_url.path(),
            "/ipns/" + test_url.host() + "/ipns/bafyb/wiki/empty.html");
  EXPECT_EQ(resolved_url.query(), "query");
  EXPECT_EQ(resolved_url.ref(), "ref");
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolvedIPFSLinkGateway) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  auto* storage_partition =
      active_contents()->GetBrowserContext()->GetDefaultStoragePartition();
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(),
            "https://dweb.link/ipns/" + test_url.host() + "/empty.html");
}

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, NoResolveIPFSLinkCalledMode) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  auto* storage_partition =
      active_contents()->GetBrowserContext()->GetDefaultStoragePartition();
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
  if (!helper)
    return;
  auto* storage_partition =
      active_contents()->GetBrowserContext()->GetDefaultStoragePartition();
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
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

IN_PROC_BROWSER_TEST_F(IpfsTabHelperBrowserTest, ResolveIPFSLinkCalled5xx) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  auto* storage_partition =
      active_contents()->GetBrowserContext()->GetDefaultStoragePartition();
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
  if (!helper)
    return;
  auto* storage_partition =
      active_contents()->GetBrowserContext()->GetDefaultStoragePartition();
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
  if (!helper)
    return;
  auto* storage_partition =
      active_contents()->GetBrowserContext()->GetDefaultStoragePartition();
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  helper->SetResolverForTesting(std::move(resolver));
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());
  prefs->SetInteger(kIPFSResolveMethod,
                    static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));

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
  if (!helper)
    return;
  auto* storage_partition =
      active_contents()->GetBrowserContext()->GetDefaultStoragePartition();
  std::unique_ptr<FakeIPFSHostResolver> resolver(
      new FakeIPFSHostResolver(storage_partition->GetNetworkContext()));
  FakeIPFSHostResolver* resolver_raw = resolver.get();
  helper->SetResolverForTesting(std::move(resolver));
  std::string ipfs_path = "/ipfs/bafybeiemx/";
  SetXIpfsPathHeader(ipfs_path);
  auto* prefs =
      user_prefs::UserPrefs::Get(active_contents()->GetBrowserContext());

  prefs->SetBoolean(kIPFSAutoRedirectDNSLink, true);
  prefs->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));

  GURL test_url = embedded_test_server()->GetURL("/empty.html?query#ref");
  GURL gateway_url = embedded_test_server()->GetURL("a.com", "/");
  prefs->SetString(kIPFSPublicGatewayAddress, gateway_url.spec());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());

  GURL expected_first("http://a.com/ipns/" + test_url.host() +
                      "/empty.html?query#ref");
  GURL::Replacements first_replacements;
  first_replacements.SetPortStr(gateway_url.port_piece());
  EXPECT_EQ(active_contents()->GetVisibleURL().spec(),
            expected_first.ReplaceComponents(first_replacements));

  GURL another_test_url =
      embedded_test_server()->GetURL("/another.html?query#ref");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), another_test_url));
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  ASSERT_FALSE(resolver_raw->resolve_called());

  GURL expected_second("http://a.com/ipns/" + test_url.host() +
                       "/another.html?query#ref");
  GURL::Replacements second_replacements;
  second_replacements.SetPortStr(gateway_url.port_piece());
  EXPECT_EQ(active_contents()->GetVisibleURL().spec(),
            expected_second.ReplaceComponents(second_replacements));

  active_contents()->GetController().GoBack();
  ASSERT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(active_contents()->GetVisibleURL(),
            expected_first.ReplaceComponents(first_replacements));
}
