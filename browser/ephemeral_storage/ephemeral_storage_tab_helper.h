/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TAB_HELPER_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TAB_HELPER_H_

#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

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
      public content::WebContentsUserData<EphemeralStorageTabHelper> {
 public:
  explicit EphemeralStorageTabHelper(content::WebContents* web_contents);
  ~EphemeralStorageTabHelper() override;

  static void SetKeepAliveTimeDelayForTesting(const base::TimeDelta& time);

 private:
  // WebContentsObserver
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;

  void ClearEphemeralLifetimeKeepalive(
      const content::TLDEphemeralLifetimeKey& key);
  void ClearLocalStorageKeepAlive(const std::string& id);

  void CreateEphemeralStorageAreasForDomainAndURL(const std::string& new_domain,
                                                  const GURL& new_url);

  friend class content::WebContentsUserData<EphemeralStorageTabHelper>;
  scoped_refptr<content::SessionStorageNamespace> local_storage_namespace_;
  scoped_refptr<content::SessionStorageNamespace> session_storage_namespace_;
  scoped_refptr<content::TLDEphemeralLifetime> tld_ephemeral_lifetime_;

  std::vector<scoped_refptr<content::TLDEphemeralLifetime>>
                    keep_alive_tld_ephemeral_lifetime_list_;
  std::vector<scoped_refptr<content::SessionStorageNamespace>>
                    keep_alive_local_storage_list_;

  base::WeakPtrFactory<EphemeralStorageTabHelper> weak_factory_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TAB_HELPER_H_
