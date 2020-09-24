/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_TAB_HELPER_H_
#define BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_TAB_HELPER_H_

#include <memory>

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

class PrefService;

namespace ipfs {

class IpfsTabHelperDelegate;

// Determines if IPFS should be active for a given top-level navigation.
class IPFSTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<IPFSTabHelper> {
 public:
  ~IPFSTabHelper() override;

  IPFSTabHelper(const IPFSTabHelper&) = delete;
  IPFSTabHelper& operator=(IPFSTabHelper&) = delete;

  bool IsActiveForMainFrame() const { return active_; }

  static void MaybeCreateForWebContents(content::WebContents* web_contents,
      bool regular_profile, IpfsTabHelperDelegate*);

 private:
  friend class content::WebContentsUserData<IPFSTabHelper>;
  explicit IPFSTabHelper(content::WebContents* web_contents);

  void UpdateActiveState(content::NavigationHandle* handle);

  // content::WebContentsObserver
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;

  void set_delegate(IpfsTabHelperDelegate* delegate);

  PrefService* pref_service_ = nullptr;
  bool active_ = false;

  std::unique_ptr<IpfsTabHelperDelegate> delegate_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_BROWSER_IPFS_TAB_HELPER_H_
