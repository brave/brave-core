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
struct ImportedData;
class IPFSHostResolver;
class IpfsService;

// Determines if IPFS should be active for a given top-level navigation.
class IPFSTabHelper : public content::WebContentsObserver,
                      public content::WebContentsUserData<IPFSTabHelper> {
 public:
  ~IPFSTabHelper() override;

  IPFSTabHelper(const IPFSTabHelper&) = delete;
  IPFSTabHelper& operator=(IPFSTabHelper&) = delete;

  static bool MaybeCreateForWebContents(content::WebContents* web_contents);
  GURL GetIPFSResolvedURL() const;

  void SetResolverForTesting(std::unique_ptr<IPFSHostResolver> resolver) {
    resolver_ = std::move(resolver);
  }

  void SetIpfsServiceForTesting(ipfs::IpfsService* service) {
    ipfs_service_ = service;
  }

  void ImportLinkToIpfs(const GURL& url);
  void ImportTextToIpfs(const std::string& text);

 private:
  friend class content::WebContentsUserData<IPFSTabHelper>;
  explicit IPFSTabHelper(content::WebContents* web_contents);

  void PushNotification(const base::string16& title,
                        const base::string16& body);
  GURL CreateAndCopyShareableLink(const ipfs::ImportedData& data);
  bool IsDNSLinkCheckEnabled() const;
  void IPFSLinkResolved(const GURL& ipfs);
  void MaybeShowDNSLinkButton(content::NavigationHandle* handle);
  void UpdateDnsLinkButtonState();

  void MaybeSetupIpfsProtocolHandlers(const GURL& url);
  void OnImportCompleted(const ipfs::ImportedData& data);
  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void UpdateLocationBar();

  void ResolveIPFSLink();
  void HostResolvedCallback(const std::string& host,
                            const std::string& dnslink);

  PrefService* pref_service_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
  ipfs::IpfsService* ipfs_service_ = nullptr;
  GURL ipfs_resolved_url_;
  std::unique_ptr<IPFSHostResolver> resolver_;
  base::WeakPtrFactory<IPFSTabHelper> weak_ptr_factory_{this};
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ipfs

#endif  // BRAVE_BROWSER_IPFS_IPFS_TAB_HELPER_H_
