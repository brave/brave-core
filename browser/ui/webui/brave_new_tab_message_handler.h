// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_

#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/web_ui_message_handler.h"

class Profile;
class BraveNewTabUI;

// Handles messages to and from the New Tab Page javascript
class BraveNewTabMessageHandler : public content::WebUIMessageHandler {
 public:
  explicit BraveNewTabMessageHandler(BraveNewTabUI* web_ui);
  ~BraveNewTabMessageHandler() override;

 private:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  void HandleInitialized(const base::ListValue* args);
  void HandleSaveNewTabPagePref(const base::ListValue* args);
  void HandleToggleAlternativeSearchEngineProvider(
      const base::ListValue* args);

  void OnStatsChanged();
  void OnPreferencesChanged();
  void OnPrivatePropertiesChanged();

  PrefChangeRegistrar pref_change_registrar_;
  BraveNewTabUI* new_tab_web_ui_;

  DISALLOW_COPY_AND_ASSIGN(BraveNewTabMessageHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_MESSAGE_HANDLER_H_
