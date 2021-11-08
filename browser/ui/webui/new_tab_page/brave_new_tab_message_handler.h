// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "brave/components/tor/tor_launcher_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/web_ui_message_handler.h"

class Profile;

namespace base {
class Time;
}  //  namespace base

namespace content {
class WebUIDataSource;
}

#if BUILDFLAG(ENABLE_TOR)
class TorLauncherFactory;
#endif

class PrefRegistrySimple;
class PrefService;

// Handles messages to and from the New Tab Page javascript
class BraveNewTabMessageHandler : public content::WebUIMessageHandler,
                                  public TorLauncherObserver {
 public:
  explicit BraveNewTabMessageHandler(Profile* profile);
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
      Profile* profile);

 private:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  void HandleGetPreferences(base::Value::ConstListView args);
  void HandleGetStats(base::Value::ConstListView args);
  void HandleGetPrivateProperties(base::Value::ConstListView args);
  void HandleGetTorProperties(base::Value::ConstListView args);
  void HandleSaveNewTabPagePref(base::Value::ConstListView args);
  void HandleToggleAlternativeSearchEngineProvider(
      base::Value::ConstListView args);
  void HandleRegisterNewTabPageView(base::Value::ConstListView args);
  void HandleBrandedWallpaperLogoClicked(base::Value::ConstListView args);
  void HandleGetWallpaperData(base::Value::ConstListView args);
  void HandleCustomizeClicked(base::Value::ConstListView args);

  void OnStatsChanged();
  void OnPreferencesChanged();
  void OnPrivatePropertiesChanged();

  // TorLauncherObserver:
  void OnTorCircuitEstablished(bool result) override;
  void OnTorInitializing(const std::string& percentage) override;

  PrefChangeRegistrar pref_change_registrar_;
  // Weak pointer.
  Profile* profile_;
#if BUILDFLAG(ENABLE_TOR)
  TorLauncherFactory* tor_launcher_factory_ = nullptr;
#endif
  base::WeakPtrFactory<BraveNewTabMessageHandler> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_NEW_TAB_PAGE_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_
