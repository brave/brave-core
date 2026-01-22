/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TAB_HELPER_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TAB_HELPER_H_

#include <optional>
#include <string>

#include "base/containers/flat_set.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "brave/browser/ephemeral_storage/tld_ephemeral_lifetime.h"
#include "build/build_config.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/memory/raw_ptr.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model_observer.h"

class TabModel;
#endif

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace ephemeral_storage {

// The EphemeralStorageTabHelper manages ephemeral storage for a WebContents.
// Ephemeral storage is a partitioned storage area only used by third-party
// iframes. This storage is partitioned based on the origin of the TLD
// of the main frame. When no more tabs are open with a particular origin,
// this storage is cleared.
class EphemeralStorageTabHelper
    : public content::WebContentsObserver,
#if BUILDFLAG(IS_ANDROID)
      public TabModelObserver,
#endif
      public content::WebContentsUserData<EphemeralStorageTabHelper> {
 public:
  explicit EphemeralStorageTabHelper(content::WebContents* web_contents);
  ~EphemeralStorageTabHelper() override;

  std::optional<base::UnguessableToken> GetEphemeralStorageToken(
      const url::Origin& origin);

  void EnforceFirstPartyStorageCleanup(StorageCleanupSource source);

 private:
  friend class content::WebContentsUserData<EphemeralStorageTabHelper>;

  // WebContentsObserver
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;

  void CreateProvisionalTLDEphemeralLifetime(
      content::NavigationHandle* navigation_handle);
  void CreateEphemeralStorageAreasForDomainAndURL(const std::string& new_domain,
                                                  const GURL& new_url);

  void UpdateShieldsState(const GURL& url);

#if BUILDFLAG(IS_ANDROID)
  // TabModelObserver
  void WillCloseTab(TabAndroid* tab) override;

  // Store the TabModel we registered with, so we can remove ourselves in
  // destructor even after WebContents is destroyed.
  raw_ptr<TabModel> registered_tab_model_ = nullptr;
#endif

  const base::raw_ptr<HostContentSettingsMap> host_content_settings_map_;
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
  scoped_refptr<content::SessionStorageNamespace> session_storage_namespace_;
  base::flat_set<scoped_refptr<TLDEphemeralLifetime>>
      provisional_tld_ephemeral_lifetimes_;
  scoped_refptr<TLDEphemeralLifetime> tld_ephemeral_lifetime_;

  base::WeakPtrFactory<EphemeralStorageTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TAB_HELPER_H_
