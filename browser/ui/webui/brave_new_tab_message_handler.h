// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_

#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/web_ui_message_handler.h"

class Profile;
namespace content {
class WebUIDataSource;
}

// Handles messages to and from the New Tab Page javascript
class BraveNewTabMessageHandler : public content::WebUIMessageHandler {
 public:
  explicit BraveNewTabMessageHandler(Profile* profile);
  ~BraveNewTabMessageHandler() override;

  static BraveNewTabMessageHandler* Create(
      content::WebUIDataSource* html_source, Profile* profile);

 private:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  void HandleGetPreferences(const base::ListValue* args);
  void HandleGetStats(const base::ListValue* args);
  void HandleGetPrivateProperties(const base::ListValue* args);
  void HandleSaveNewTabPagePref(const base::ListValue* args);
  void HandleToggleAlternativeSearchEngineProvider(
      const base::ListValue* args);
  void HandleRegisterNewTabPageView(const base::ListValue* args);
  void HandleGetBrandedWallpaperData(const base::ListValue* args);
  void HandleGetDefaultTopSitesData(const base::ListValue* args);

  void OnStatsChanged();
  void OnPreferencesChanged();
  void OnPrivatePropertiesChanged();

  PrefChangeRegistrar pref_change_registrar_;
  // Weak pointer.
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(BraveNewTabMessageHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_
