// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "mojo/public/cpp/bindings/receiver.h"

class Profile;

namespace base {
class Time;
class Value;
}  //  namespace base

namespace content {
class WebUIDataSource;
}

class PrefRegistrySimple;
class PrefService;

// TODO(simonhong): Migrate to brave_new_tab_page.mojom.
// Handles messages to and from the New Tab Page javascript
class BraveNewTabMessageHandler : public content::WebUIMessageHandler,
                                  public bat_ads::mojom::BatAdsObserver {
 public:
  BraveNewTabMessageHandler(Profile* profile, bool was_restored);
  BraveNewTabMessageHandler(const BraveNewTabMessageHandler&) = delete;
  BraveNewTabMessageHandler& operator=(const BraveNewTabMessageHandler&) =
      delete;
  ~BraveNewTabMessageHandler() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* local_state);
  static void RecordInitialP3AValues(PrefService* local_state);
  static bool CanPromptBraveTalk();
  static bool CanPromptBraveTalk(base::Time now);
  static BraveNewTabMessageHandler* Create(
      content::WebUIDataSource* html_source,
      Profile* profile,
      bool was_restored);

 private:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  void HandleGetPreferences(const base::Value::List& args);
  void HandleGetStats(const base::Value::List& args);
  void HandleGetNewTabAdsData(const base::Value::List& args);
  void HandleSaveNewTabPagePref(const base::Value::List& args);
  void HandleRegisterNewTabPageView(const base::Value::List& args);
  void HandleBrandedWallpaperLogoClicked(const base::Value::List& args);
  void HandleGetWallpaperData(const base::Value::List& args);
  void HandleCustomizeClicked(const base::Value::List& args);

  void OnStatsChanged();
  void OnPreferencesChanged();

  base::Value::Dict GetAdsDataDictionary() const;

  // bat_ads::mojom::BatAdsObserver:
  void OnAdRewardsDidChange() override {}
  void OnBrowserUpgradeRequiredToServeAds() override;
  void OnIneligibleWalletToServeAds() override {}
  void OnRemindUser(brave_ads::mojom::ReminderType type) override {}

  PrefChangeRegistrar pref_change_registrar_;
  // Weak pointer.
  raw_ptr<Profile> profile_ = nullptr;

  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;
  mojo::Receiver<bat_ads::mojom::BatAdsObserver> bat_ads_observer_receiver_{
      this};

  bool was_restored_ = false;

  base::WeakPtrFactory<BraveNewTabMessageHandler> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_
