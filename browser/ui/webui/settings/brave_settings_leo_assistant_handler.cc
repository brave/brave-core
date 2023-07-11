/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_settings_leo_assistant_handler.h"

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "chrome/browser/profiles/profile.h"

namespace settings {

BraveLeoAssistantHandler::BraveLeoAssistantHandler() = default;
BraveLeoAssistantHandler::~BraveLeoAssistantHandler() = default;

void BraveLeoAssistantHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());

  web_ui()->RegisterMessageCallback(
      "initLeoAssistant",
      base::BindRepeating(&BraveLeoAssistantHandler::HandleInitLeoAssistant,
                          base::Unretained(this)));
}

void BraveLeoAssistantHandler::HandleInitLeoAssistant(
    const base::Value::List& args) {
  AllowJavascript();
}

void BraveLeoAssistantHandler::OnJavascriptAllowed() {
  sidebar_service_observer_.Reset();
  sidebar_service_observer_.Observe(
      sidebar::SidebarServiceFactory::GetForProfile(profile_));
}

void BraveLeoAssistantHandler::OnJavascriptDisallowed() {
  sidebar_service_observer_.Reset();
}

void BraveLeoAssistantHandler::OnItemAdded(const sidebar::SidebarItem& item,
                                           size_t index) {
  if (item.built_in_item_type ==
      sidebar::SidebarItem::BuiltInItemType::kChatUI) {
    NotifyChatUiChanged();
  }
}

void BraveLeoAssistantHandler::OnItemRemoved(const sidebar::SidebarItem& item,
                                             size_t index) {
  if (item.built_in_item_type ==
      sidebar::SidebarItem::BuiltInItemType::kChatUI) {
    NotifyChatUiChanged();
  }
}

void BraveLeoAssistantHandler::NotifyChatUiChanged() {
  if (!IsJavascriptAllowed()) {
    return;
  }
  FireWebUIListener("settings-brave-leo-assistant-changed");
}

}  // namespace settings
