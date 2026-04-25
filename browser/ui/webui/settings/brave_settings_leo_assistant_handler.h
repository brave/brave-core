/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_LEO_ASSISTANT_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_LEO_ASSISTANT_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

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

  void HandleValidateModelEndpoint(const base::ListValue& args);
  void HandleToggleLeoIcon(const base::ListValue& args);
  void HandleGetLeoIconVisibility(const base::ListValue& args);
  void HandleResetLeoData(const base::ListValue& args);

  raw_ptr<Profile> profile_ = nullptr;
  base::ScopedObservation<sidebar::SidebarService,
                          sidebar::SidebarService::Observer>
      sidebar_service_observer_{this};
};

}  // namespace settings

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_SETTINGS_LEO_ASSISTANT_HANDLER_H_
