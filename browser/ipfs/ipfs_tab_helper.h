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

#include "brave/browser/ipfs/import/ipfs_import_controller.h"
#include "brave/browser/ipfs/ipfs_host_resolver.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

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

 private:
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest, CanResolveURLTest);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest, URLResolvingTest);
  FRIEND_TEST_ALL_PREFIXES(IpfsTabHelperUnitTest, GatewayResolving);

  friend class content::WebContentsUserData<IPFSTabHelper>;
  explicit IPFSTabHelper(content::WebContents* web_contents);

  GURL GetCurrentPageURL() const;
  bool CanResolveURL(const GURL& url) const;
  bool IsDNSLinkCheckEnabled() const;
  void IPFSLinkResolved(const GURL& ipfs);
  void MaybeShowDNSLinkButton(const net::HttpResponseHeaders* headers);
  void UpdateDnsLinkButtonState();

  void MaybeSetupIpfsProtocolHandlers(const GURL& url);

  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void UpdateLocationBar();

  void ResolveIPFSLink();
  void HostResolvedCallback(const std::string& host,
                            const std::string& dnslink);

  PrefService* pref_service_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  GURL ipfs_resolved_url_;
  GURL current_page_url_for_testing_;
  std::unique_ptr<IPFSHostResolver> resolver_;
  base::WeakPtrFactory<IPFSTabHelper> weak_ptr_factory_{this};
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_TAB_HELPER_H_
