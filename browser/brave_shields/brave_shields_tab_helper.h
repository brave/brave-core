/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_TAB_HELPER_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_TAB_HELPER_H_

#include <set>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/favicon/core/favicon_driver_observer.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

using brave_shields::mojom::AdBlockMode;
using brave_shields::mojom::CookieBlockMode;
using brave_shields::mojom::FingerprintMode;
using brave_shields::mojom::HttpsUpgradeMode;
using content::NavigationEntry;

namespace brave_shields {

// Per-tab class to manage Shields panel data
class BraveShieldsTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BraveShieldsTabHelper>,
      public content_settings::Observer,
      public favicon::FaviconDriverObserver {
 public:
  BraveShieldsTabHelper(const BraveShieldsTabHelper&) = delete;
  BraveShieldsTabHelper& operator=(const BraveShieldsTabHelper&) = delete;
  ~BraveShieldsTabHelper() override;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnResourcesChanged() = 0;
    virtual void OnFaviconUpdated() {}
    virtual void OnShieldsEnabledChanged() {}
  };

  void HandleItemBlocked(const std::string& block_type,
                         const std::string& subresource);
  void HandleItemAllowedOnce(const std::string& allowed_once_type,
                             const std::string& subresource);
  void HandleWebcompatFeatureInvoked(
      ContentSettingsType webcompat_content_settings);
  void ClearAllResourcesList();
  int GetTotalBlockedCount();
  std::vector<GURL> GetBlockedAdsList();
  std::vector<GURL> GetHttpRedirectsList();
  std::vector<GURL> GetBlockedJsList();
  std::vector<GURL> GetAllowedJsList();
  std::vector<GURL> GetFingerprintsList();
  bool GetBraveShieldsEnabled();
  void SetBraveShieldsEnabled(bool is_enabled);
  GURL GetCurrentSiteURL();
  GURL GetFaviconURL(bool refresh);
  const base::flat_set<ContentSettingsType>& GetInvokedWebcompatFeatures();

  AdBlockMode GetAdBlockMode();
  FingerprintMode GetFingerprintMode();
  CookieBlockMode GetCookieBlockMode();
  bool IsBraveShieldsManaged();
  bool IsForgetFirstPartyStorageFeatureEnabled() const;
  HttpsUpgradeMode GetHttpsUpgradeMode();
  bool GetNoScriptEnabled();
  bool GetForgetFirstPartyStorageEnabled();
  void SetAdBlockMode(AdBlockMode mode);
  void SetFingerprintMode(FingerprintMode mode);
  void SetCookieBlockMode(CookieBlockMode mode);
  void SetHttpsUpgradeMode(HttpsUpgradeMode mode);
  void SetIsNoScriptEnabled(bool is_enabled);
  void SetForgetFirstPartyStorageEnabled(bool is_enabled);
  void AllowScriptsOnce(const std::vector<std::string>& origins);
  void BlockAllowedScripts(const std::vector<std::string>& origins);
  void SetWebcompatEnabled(ContentSettingsType webcompat_settings_type,
                           bool enabled);
  base::flat_map<ContentSettingsType, bool> GetWebcompatSettings();

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);
  bool HasObserver(Observer* observer);

 private:
  friend class content::WebContentsUserData<BraveShieldsTabHelper>;

  explicit BraveShieldsTabHelper(content::WebContents* web_contents);

  // content::WebContentsObserver
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;

  // content_settings::Observer
  void OnContentSettingChanged(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      ContentSettingsTypeSet content_type_set) override;

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
  std::set<GURL> resource_list_allowed_once_js_;
  std::set<GURL> resource_list_blocked_fingerprints_;
  base::flat_set<ContentSettingsType> webcompat_features_invoked_;
  base::ScopedObservation<HostContentSettingsMap, content_settings::Observer>
      observation_{this};

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_TAB_HELPER_H_
