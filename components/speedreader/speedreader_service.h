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
#include "brave/components/speedreader/speedreader_metrics.h"
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

// Returns true if Speedreader feature is enabled.
bool IsSpeedreaderFeatureEnabled(PrefService* prefs);

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
                     PrefService* local_state,
                     HostContentSettingsMap* content_rules);
  ~SpeedreaderService() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Returns |true| if Speedreader should be allowed for all sites that look
  // readable. This is just the setting value and additional checks are needed
  // to determine if the site should be enabled for Speedreader or not
  bool IsAllowedForAllReadableSites();
  void SetAllowedForAllReadableSites(bool enabled);

  // Returns |true| if Speedreader should be allowed for the site. This does
  // not necessarily mean Speedreader is enabled for the site. See
  // IsAllowedForAllReadableSites() above.
  bool IsAllowedForSite(const GURL& url);
  bool IsAllowedForSite(content::WebContents* contents);

  // Speedreader should be explicitly enabled/disabled. A site that is enabled
  // will always open automatically in Speedreader. A site that is disabled will
  // never open in Speedreader even if
  // IsAllowedForAllReadableSites/IsAllowedForSite is true and/or it looks
  // readable.
  void SetEnabledForSite(const GURL& url, bool enabled);
  void SetEnabledForSite(content::WebContents* contents, bool enabled);

  // Returns |true| if Speedreader should be enabled for the site
  bool IsEnabledForSite(const GURL& url);
  bool IsEnabledForSite(content::WebContents* contents);

  // Returns |true| if Speedreader should be disabled for the site
  bool IsDisabledForSite(const GURL& url);
  bool IsDisabledForSite(content::WebContents* contents);

  void SetAppearanceSettings(
      const mojom::AppearanceSettings& appearance_settings);
  mojom::AppearanceSettings GetAppearanceSettings() const;

  void SetTtsSettings(const mojom::TtsSettings& tts_settings);
  mojom::TtsSettings GetTtsSettings() const;

  std::string GetThemeName() const;
  std::string GetFontSizeName() const;
  std::string GetFontFamilyName() const;
  std::string GetColumnWidth() const;

  SpeedreaderMetrics& metrics() { return metrics_; }

  SpeedreaderService(const SpeedreaderService&) = delete;
  SpeedreaderService& operator=(const SpeedreaderService&) = delete;

 private:
  // Returns content setting if the user has explicitally enabled/disabled
  // Speedreader on the domain.
  ContentSetting GetSiteSetting(const GURL& url);
  ContentSetting GetSiteSetting(content::WebContents* contents);

  raw_ptr<content::BrowserContext> browser_context_ = nullptr;
  raw_ptr<HostContentSettingsMap> content_rules_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  base::ObserverList<Observer> observers_;
  SpeedreaderMetrics metrics_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_
