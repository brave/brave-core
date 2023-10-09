/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_IPFS_IPFS_TAB_HELPER_H_
#define BRAVE_BROWSER_IPFS_IPFS_TAB_HELPER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "brave/browser/ipfs/import/ipfs_import_controller.h"
#include "brave/browser/ipfs/ipfs_host_resolver.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

class PrefService;

namespace ipfs {
class IPFSHostResolver;
class IpfsImportController;

// Determines if IPFS should be active for a given top-level navigation.
class IPFSTabHelper : public content::WebContentsObserver,
                      public IpfsImportController,
                      public content::WebContentsUserData<IPFSTabHelper> {
 public:
  IPFSTabHelper(const IPFSTabHelper&) = delete;
  IPFSTabHelper& operator=(IPFSTabHelper&) = delete;
  ~IPFSTabHelper() override;

  static bool MaybeCreateForWebContents(content::WebContents* web_contents);
  GURL GetIPFSResolvedURL() const;

  void SetResolverForTesting(std::unique_ptr<IPFSHostResolver> resolver) {
    resolver_ = std::move(resolver);
  }

  IpfsImportController* GetImportController() {
    return static_cast<IpfsImportController*>(this);
  }

  void SetPageURLForTesting(const GURL& url) {
    current_page_url_for_testing_ = url;
  }
  void SetSetShowFallbackInfobarCallbackForTesting(
      base::RepeatingCallback<void(const GURL&)> callback) {
    show_fallback_infobar_callback_for_testing_ = callback;
  }

  void SetRediretCallbackForTesting(
      base::RepeatingCallback<void(const GURL&)> callback) {
    redirect_callback_for_testing_ = callback;
  }

 private:
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest, CanResolveURLTest);
  FRIEND_TEST_ALL_PREFIXES(
      IpfsTabHelperUnitTest,
      TranslateUrlToIpns_When_HasDNSLinkRecord_AndXIPFSPathHeader);
  FRIEND_TEST_ALL_PREFIXES(
      IpfsTabHelperUnitTest,
      TranslateUrlToIpns_When_HasDNSLinkRecord_AndOriginalPageFails_500);
  FRIEND_TEST_ALL_PREFIXES(
      IpfsTabHelperUnitTest,
      DoNotTranslateUrlToIpns_When_HasDNSLinkRecord_AndOriginalPageFails_400);
  FRIEND_TEST_ALL_PREFIXES(
      IpfsTabHelperUnitTest,
      TranslateUrlToIpns_When_HasDNSLinkRecord_AndOriginalPageFails_505);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           DoNotTranslateUrlToIpns_When_NoDNSLinkRecord);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           DoNotTranslateUrlToIpns_When_NoHeader_And_NoError);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           DNSLinkRecordResolved_AutoRedirectDNSLink);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           XIpfsPathHeaderUsed_IfNoDnsLinkRecord_IPFS);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           XIpfsPathHeaderUsed_IfNoDnsLinkRecord_IPNS);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest, ResolveXIPFSPathUrl);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest, GatewayResolving);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           GatewayLikeUrlParsed_AutoRedirectEnabled);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           GatewayLikeUrlParsed_AutoRedirectDisabled);
  FRIEND_TEST_ALL_PREFIXES(
      IpfsTabHelperUnitTest,
      GatewayLikeUrlParsed_AutoRedirectDisabled_WithXIPFSPathHeader);
  FRIEND_TEST_ALL_PREFIXES(
      IpfsTabHelperUnitTest,
      GatewayLikeUrlParsed_AutoRedirectDisabled_WithDnsLink);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest, GatewayIPNS_Redirect);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           GatewayIPNS_NoRedirect_WhenNoDnsLinkRecord);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest, GatewayIPNS_ResolveUrl);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           GatewayIPNS_Redirect_LibP2PKey);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           GatewayIPNS_Redirect_LibP2PKey_NoAutoRedirect);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           GatewayIPNS_No_Redirect_WhenNoDnsLink);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           DetectPageLoadingError_ShowInfobar);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           DetectPageLoadingError_HeadersOk_ShowInfobar);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           DetectPageLoadingError_IPFSCompanion_Enabled);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest,
                           DetectPageLoadingError_NoRedirectAsNonIPFSLink);
  friend class content::WebContentsUserData<IPFSTabHelper>;
  friend class BraveIPFSInfoBarDelegateObserverImpl;
  friend class BraveIPFSFallbackInfoBarDelegateObserverImpl;
  explicit IPFSTabHelper(content::WebContents* web_contents);

  GURL GetCurrentPageURL() const;
  bool CanResolveURL(const GURL& url) const;
  bool IsDNSLinkCheckEnabled() const;
  bool IsAutoRedirectIPFSResourcesEnabled() const;
  void IPFSResourceLinkResolved(const GURL& ipfs);
  void DNSLinkResolved(const GURL& ipfs, bool is_gateway_url);
  void MaybeCheckDNSLinkRecord(const net::HttpResponseHeaders* headers);
  void UpdateDnsLinkButtonState();
  absl::optional<GURL> ResolveIPFSUrlFromGatewayLikeUrl(const GURL& gurl);

  GURL ResolveDNSLinkUrl(const GURL& url);
  GURL ResolveXIPFSPathUrl(const std::string& x_ipfs_path_header_value);

  void MaybeSetupIpfsProtocolHandlers(const GURL& url);

  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  void UpdateLocationBar();

  void CheckDNSLinkRecord(const GURL& gurl,
                          bool is_gateway_url,
                          absl::optional<std::string> x_ipfs_path_header);
  void HostResolvedCallback(const GURL& current,
                            const GURL& url,
                            bool is_gateway_url,
                            absl::optional<std::string> x_ipfs_path_header,
                            const std::string& host,
                            const absl::optional<std::string>& dnslink);

  void LoadUrl(const GURL& gurl);

  void SetFallbackAddress(const GURL& original_url);

  void ShowBraveIPFSFallbackInfoBar(const GURL& initial_navigation_url);

  const raw_ptr<PrefService> pref_service_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  GURL ipfs_resolved_url_;
  GURL current_page_url_for_testing_;
  base::RepeatingCallback<void(const GURL&)> redirect_callback_for_testing_;
  base::RepeatingCallback<void(const GURL&)>
      show_fallback_infobar_callback_for_testing_;
  std::unique_ptr<IPFSHostResolver> resolver_;
  absl::optional<GURL> initial_navigation_url_;
  bool auto_redirect_blocked_{false};
  base::WeakPtrFactory<IPFSTabHelper> weak_ptr_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_TAB_HELPER_H_
