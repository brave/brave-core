/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_SHIELDS_DATA_CONTROLLER_H_
#define BRAVE_BROWSER_UI_BRAVE_SHIELDS_DATA_CONTROLLER_H_

#include <set>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_shields/common/brave_shields_panel.mojom.h"
#include "components/favicon/core/favicon_driver_observer.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

using brave_shields::mojom::AdBlockMode;
using brave_shields::mojom::CookieBlockMode;
using brave_shields::mojom::FingerprintMode;
using content::NavigationEntry;

namespace brave_shields {

// Per-tab class to manage Shields panel data
class BraveShieldsDataController
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BraveShieldsDataController>,
      public favicon::FaviconDriverObserver {
 public:
  BraveShieldsDataController(const BraveShieldsDataController&) = delete;
  BraveShieldsDataController& operator=(const BraveShieldsDataController&) =
      delete;
  ~BraveShieldsDataController() override;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnResourcesChanged() = 0;
    virtual void OnFaviconUpdated() {}
  };

  void HandleItemBlocked(const std::string& block_type,
                         const std::string& subresource);
  void ClearAllResourcesList();
  int GetTotalBlockedCount();
  std::vector<GURL> GetBlockedAdsList();
  std::vector<GURL> GetHttpRedirectsList();
  std::vector<GURL> GetJsList();
  std::vector<GURL> GetFingerprintsList();
  bool GetBraveShieldsEnabled();
  void SetBraveShieldsEnabled(bool is_enabled);
  GURL GetCurrentSiteURL();
  GURL GetFaviconURL(bool refresh);

  AdBlockMode GetAdBlockMode();
  FingerprintMode GetFingerprintMode();
  CookieBlockMode GetCookieBlockMode();
  bool GetHTTPSEverywhereEnabled();
  bool GetNoScriptEnabled();
  void SetAdBlockMode(AdBlockMode mode);
  void SetFingerprintMode(FingerprintMode mode);
  void SetCookieBlockMode(CookieBlockMode mode);
  void SetIsNoScriptEnabled(bool is_enabled);
  void SetIsHTTPSEverywhereEnabled(bool is_enabled);

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);
  bool HasObserver(Observer* observer);

 private:
  friend class content::WebContentsUserData<BraveShieldsDataController>;

  explicit BraveShieldsDataController(content::WebContents* web_contents);

  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;

  // favicon::FaviconDriverObserver
  void OnFaviconUpdated(favicon::FaviconDriver* favicon_driver,
                        NotificationIconType notification_icon_type,
                        const GURL& icon_url,
                        bool icon_url_changed,
                        const gfx::Image& image) override;

  void ReloadWebContents();

  base::ObserverList<Observer> observer_list_;
  std::set<GURL> resource_list_blocked_ads_;
  std::set<GURL> resource_list_http_redirects_;
  std::set<GURL> resource_list_blocked_js_;
  std::set<GURL> resource_list_blocked_fingerprints_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_UI_BRAVE_SHIELDS_DATA_CONTROLLER_H_
