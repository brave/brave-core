/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "brave/components/speedreader/common/speedreader_toolbar.mojom.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/keyed_service/core/keyed_service.h"

class HostContentSettingsMap;
class PrefRegistrySimple;
class PrefService;
class GURL;

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace speedreader {

namespace features {
bool IsSpeedreaderEnabled();
}

class SpeedreaderService : public KeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnSiteEnableSettingChanged(content::WebContents* site,
                                            bool enabled_on_site) {}
    virtual void OnAllSitesEnableSettingChanged(bool enabled_on_all_sites) {}
    virtual void OnAppearanceSettingsChanged(
        const mojom::AppearanceSettings& appearance_settings) {}
    virtual void OnTtsSettingsChanged(const mojom::TtsSettings& tts_settings) {}

   protected:
    ~Observer() override = default;
  };

  SpeedreaderService(content::BrowserContext* browser_context,
                     HostContentSettingsMap* content_rules);
  ~SpeedreaderService() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Returns |true| if Speedreader is turned on for all sites.
  bool IsEnabledForAllSites();

  // Returns content setting if the user has explicitally enabled/disabled
  // Speedreader on the domain.
  ContentSetting GetEnabledForSiteSetting(const GURL& url);
  ContentSetting GetEnabledForSiteSetting(content::WebContents* contents);

  // Returns |true| if IsEnabledForAllSites is true or/and
  // GetEnabledForSiteSetting is ALLOW.
  bool IsEnabledForSite(const GURL& url);
  bool IsEnabledForSite(content::WebContents* contents);

  // Allow or deny a site(all sites) from being run through speedreader.
  void EnableForAllSites(bool enabled);
  void EnableForSite(const GURL& url, bool enabled);
  void EnableForSite(content::WebContents* contents, bool enabled);

  void SetAppearanceSettings(
      const mojom::AppearanceSettings& appearance_settings);
  mojom::AppearanceSettings GetAppearanceSettings() const;

  void SetTtsSettings(const mojom::TtsSettings& tts_settings);
  mojom::TtsSettings GetTtsSettings() const;

  std::string GetThemeName() const;
  std::string GetFontSizeName() const;
  std::string GetFontFamilyName() const;
  std::string GetColumnWidth() const;

  SpeedreaderService(const SpeedreaderService&) = delete;
  SpeedreaderService& operator=(const SpeedreaderService&) = delete;

 private:
  raw_ptr<content::BrowserContext> browser_context_ = nullptr;
  raw_ptr<HostContentSettingsMap> content_rules_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  base::ObserverList<Observer> observers_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_
