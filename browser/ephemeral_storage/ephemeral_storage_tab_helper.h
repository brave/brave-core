/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TAB_HELPER_H_
#define BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TAB_HELPER_H_

#include <string>
#include <utility>

#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class BrowserContext;
class EphemeralStoragePartition;
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

 protected:
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;

 private:
  void CreateEphemeralStorageAreasForDomainAndURL(std::string new_domain,
                                                  const GURL& new_url);

  friend class content::WebContentsUserData<EphemeralStorageTabHelper>;
  scoped_refptr<content::SessionStorageNamespace> local_storage_namespace_;
  scoped_refptr<content::SessionStorageNamespace> session_storage_namespace_;
  scoped_refptr<content::EphemeralStoragePartition>
      ephemeral_storage_partition_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ephemeral_storage

#endif  // BRAVE_BROWSER_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TAB_HELPER_H_
