/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_LEO_ASSISTANT_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_LEO_ASSISTANT_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;

namespace settings {

class BraveLeoAssistantHandler : public settings::SettingsPageUIHandler,
                                 public sidebar::SidebarService::Observer {
 public:
  BraveLeoAssistantHandler();
  ~BraveLeoAssistantHandler() override;

  BraveLeoAssistantHandler(const BraveLeoAssistantHandler&) = delete;
  BraveLeoAssistantHandler& operator=(const BraveLeoAssistantHandler&) = delete;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  // sidebar::SidebarService::Observer overrides
  void OnItemAdded(const sidebar::SidebarItem& item, size_t index) override;
  void OnItemRemoved(const sidebar::SidebarItem& item, size_t index) override;

  void NotifyChatUiChanged(const bool& isLeoVisible);

  void HandleToggleLeoIcon(const base::Value::List& args);
  void HandleGetLeoIconVisibility(const base::Value::List& args);
  void HandleResetLeoData(const base::Value::List& args);
  void HandleGetModels(const base::Value::List& args);

  raw_ptr<Profile> profile_ = nullptr;
  base::ScopedObservation<sidebar::SidebarService,
                          sidebar::SidebarService::Observer>
      sidebar_service_observer_{this};
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_LEO_ASSISTANT_HANDLER_H_
