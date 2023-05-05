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
#include "brave/components/speedreader/speedreader_util.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class PrefService;

namespace speedreader {

class SpeedreaderService : public KeyedService {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnSiteSettingsChanged(
        const mojom::SiteSettings& site_settings) {}
    virtual void OnTtsSettingsChanged(const mojom::TtsSettings& tts_settings) {}

   protected:
    ~Observer() override = default;
  };

  explicit SpeedreaderService(PrefService* prefs);
  ~SpeedreaderService() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void ToggleSpeedreader();
  void DisableSpeedreaderForTest();
  bool IsEnabled();
  bool ShouldPromptUserToEnable() const;
  void IncrementPromptCount();

  void SetSiteSettings(const mojom::SiteSettings& site_settings);
  mojom::SiteSettings GetSiteSettings() const;

  void SetTtsSettings(const mojom::TtsSettings& tts_settings);
  mojom::TtsSettings GetTtsSettings() const;

  std::string GetThemeName() const;
  std::string GetFontSizeName() const;
  std::string GetFontFamilyName() const;

  SpeedreaderService(const SpeedreaderService&) = delete;
  SpeedreaderService& operator=(const SpeedreaderService&) = delete;

 private:
  raw_ptr<PrefService> prefs_ = nullptr;
  base::ObserverList<Observer> observers_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_
