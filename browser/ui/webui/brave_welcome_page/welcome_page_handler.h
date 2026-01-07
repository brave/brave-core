// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_WELCOME_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_WELCOME_PAGE_HANDLER_H_

#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.h"
#include "chrome/browser/themes/theme_service_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;
class ThemeService;

namespace brave_welcome_page {

// Handler for communication with the welcome page front end application.
class WelcomePageHandler : public mojom::WelcomePageHandler,
                           public ThemeServiceObserver {
 public:
  WelcomePageHandler(mojo::PendingReceiver<mojom::WelcomePageHandler> receiver,
                     const base::flat_set<mojom::Feature>& available_features,
                     ThemeService* theme_service,
                     PrefService* prefs,
                     PrefService* local_state);

  WelcomePageHandler(const WelcomePageHandler&) = delete;
  WelcomePageHandler& operator=(const WelcomePageHandler&) = delete;

  ~WelcomePageHandler() override;

  // mojom::WelcomePageHandler:
  void SetWelcomePage(mojo::PendingRemote<mojom::WelcomePage> page) override;
  void GetColorScheme(GetColorSchemeCallback callback) override;
  void SetColorScheme(mojom::ColorScheme color_scheme,
                      SetColorSchemeCallback callback) override;
  void GetVerticalTabsEnabled(GetVerticalTabsEnabledCallback callback) override;
  void SetVerticalTabsEnabled(bool enabled,
                              SetVerticalTabsEnabledCallback callback) override;
  void GetFeatureVisibility(GetFeatureVisibilityCallback callback) override;
  void SetFeatureVisible(mojom::Feature feature,
                         bool visible,
                         SetFeatureVisibleCallback callback) override;
  void SetWebDiscoveryEnabled(bool enabled,
                              SetWebDiscoveryEnabledCallback callback) override;
  void SetP3AEnabled(bool enabled, SetP3AEnabledCallback callback) override;
  void SetCrashReportsEnabled(bool enabled,
                              SetCrashReportsEnabledCallback callback) override;

  // ThemeServiceObserver:
  void OnThemeChanged() override;

 private:
  void OnVerticalTabsEnabledChanged();
  void OnFeatureVisibilityChanged();

  bool IsFeatureVisible(mojom::Feature feature) const;

  mojo::Receiver<mojom::WelcomePageHandler> receiver_;
  mojo::Remote<mojom::WelcomePage> page_;

  base::ScopedObservation<ThemeService, ThemeServiceObserver>
      theme_service_observation_{this};

  base::flat_map<mojom::Feature, std::vector<std::string_view>>
      feature_visibility_prefs_;
  PrefChangeRegistrar pref_change_registrar_;
  const raw_ref<PrefService> local_state_;
};

}  // namespace brave_welcome_page

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_WELCOME_PAGE_HANDLER_H_
