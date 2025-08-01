/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_settings_leo_assistant_handler.h"

#include <algorithm>
#include <vector>

#include "base/containers/contains.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"

namespace {

const std::vector<sidebar::SidebarItem>::const_iterator FindAiChatSidebarItem(
    const std::vector<sidebar::SidebarItem>& items) {
  return std::ranges::find_if(items, [](const auto& item) {
    return item.built_in_item_type ==
           sidebar::SidebarItem::BuiltInItemType::kChatUI;
  });
}

bool ShowLeoAssistantIconVisibleIfNot(
    sidebar::SidebarService* sidebar_service) {
  const auto hidden_items = sidebar_service->GetHiddenDefaultSidebarItems();
  const auto item_hidden_iter = FindAiChatSidebarItem(hidden_items);

  if (item_hidden_iter != hidden_items.end()) {
    sidebar_service->AddItem(*item_hidden_iter);
    return true;
  }

  return false;
}

bool HideLeoAssistantIconIfNot(sidebar::SidebarService* sidebar_service) {
  const auto visible_items = sidebar_service->items();
  const auto item_visible_iter = FindAiChatSidebarItem(visible_items);

  if (item_visible_iter != visible_items.end()) {
    sidebar_service->RemoveItemAt(item_visible_iter - visible_items.begin());
    return true;
  }

  return false;
}

}  // namespace

namespace settings {

BraveLeoAssistantHandler::BraveLeoAssistantHandler() = default;

BraveLeoAssistantHandler::~BraveLeoAssistantHandler() = default;

void BraveLeoAssistantHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());

  web_ui()->RegisterMessageCallback(
      "toggleLeoIcon",
      base::BindRepeating(&BraveLeoAssistantHandler::HandleToggleLeoIcon,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getLeoIconVisibility",
      base::BindRepeating(&BraveLeoAssistantHandler::HandleGetLeoIconVisibility,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "resetLeoData",
      base::BindRepeating(&BraveLeoAssistantHandler::HandleResetLeoData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "validateModelEndpoint",
      base::BindRepeating(
          &BraveLeoAssistantHandler::HandleValidateModelEndpoint,
          base::Unretained(this)));
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
    NotifyChatUiChanged(true);
  }
}

void BraveLeoAssistantHandler::OnItemRemoved(const sidebar::SidebarItem& item,
                                             size_t index) {
  if (item.built_in_item_type ==
      sidebar::SidebarItem::BuiltInItemType::kChatUI) {
    NotifyChatUiChanged(false);
  }
}

void BraveLeoAssistantHandler::NotifyChatUiChanged(const bool& is_leo_visible) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  FireWebUIListener("settings-brave-leo-assistant-changed", is_leo_visible);
}

void BraveLeoAssistantHandler::HandleToggleLeoIcon(
    const base::Value::List& args) {
  auto* service = sidebar::SidebarServiceFactory::GetForProfile(profile_);

  AllowJavascript();
  if (!ShowLeoAssistantIconVisibleIfNot(service)) {
    HideLeoAssistantIconIfNot(service);
  }
}

void BraveLeoAssistantHandler::HandleValidateModelEndpoint(
    const base::Value::List& args) {
  AllowJavascript();

  if (args.size() < 2 || !args[1].is_dict()) {
    // Expect the appropriate number and type of arguments, or reject
    RejectJavascriptCallback(args[0], base::Value("Invalid arguments"));
    return;
  }

  const base::Value::Dict& dict = args[1].GetDict();
  GURL endpoint(*dict.FindString("url"));

  base::Value::Dict response;

  const bool is_valid = ai_chat::ModelValidator::IsValidEndpoint(endpoint);

  response.Set("isValid", is_valid);
  response.Set("isValidAsPrivateEndpoint",
               ai_chat::ModelValidator::IsValidEndpoint(
                   endpoint, std::optional<bool>(true)));
  response.Set("isValidDueToPrivateIPsFeature",
               is_valid && ai_chat::features::IsAllowPrivateIPsEnabled() &&
                   !ai_chat::ModelValidator::IsValidEndpoint(
                       endpoint, std::optional<bool>(false)));

  ResolveJavascriptCallback(args[0], response);
}

void BraveLeoAssistantHandler::HandleGetLeoIconVisibility(
    const base::Value::List& args) {
  auto* service = sidebar::SidebarServiceFactory::GetForProfile(profile_);
  const auto hidden_items = service->GetHiddenDefaultSidebarItems();
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], !base::Contains(hidden_items,
                               sidebar::SidebarItem::BuiltInItemType::kChatUI,
                               &sidebar::SidebarItem::built_in_item_type));
}

void BraveLeoAssistantHandler::HandleResetLeoData(
    const base::Value::List& args) {
  auto* sidebar_service =
      sidebar::SidebarServiceFactory::GetForProfile(profile_);

  ShowLeoAssistantIconVisibleIfNot(sidebar_service);

  ai_chat::AIChatService* service =
      ai_chat::AIChatServiceFactory::GetForBrowserContext(profile_);
  if (!service) {
    return;
  }
  service->DeleteConversations();
  if (profile_) {
    ai_chat::SetUserOptedIn(profile_->GetPrefs(), false);
    ai_chat::prefs::DeleteAllMemoriesFromPrefs(*profile_->GetPrefs());
    ai_chat::prefs::ResetCustomizationsPref(*profile_->GetPrefs());
  }

  AllowJavascript();
}

}  // namespace settings
