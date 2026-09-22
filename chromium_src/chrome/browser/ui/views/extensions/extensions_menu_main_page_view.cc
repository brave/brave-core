/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/extensions/extensions_menu_main_page_view.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/browser/ui/views/extensions/brave_extensions_menu_entry_view.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/extensions/extension_action_view_model.h"
#include "chrome/browser/ui/extensions/extensions_menu_handler.h"

namespace {

void CreateAndInsertBraveMenuEntry(
    BrowserWindowInterface* browser,
    ExtensionsMenuHandler* menu_handler,
    ExtensionActionViewModel* action_model,
    ExtensionsMenuViewModel::MenuEntryState entry_state,
    int index,
    views::View* menu_entries) {
  auto item = std::make_unique<BraveExtensionsMenuEntryView>(
      browser, entry_state.is_enterprise, action_model,
      base::BindRepeating(&ExtensionsMenuHandler::OnActionButtonClicked,
                          base::Unretained(menu_handler),
                          action_model->GetId()),
      base::BindRepeating(&ExtensionsMenuHandler::OnExtensionToggleSelected,
                          base::Unretained(menu_handler),
                          action_model->GetId()),
      base::BindRepeating(&ExtensionsMenuHandler::OpenSitePermissionsPage,
                          base::Unretained(menu_handler),
                          action_model->GetId()));
  item->Update(entry_state);
  menu_entries->AddChildViewAt(std::move(item), index);
}

}  // namespace

#include <chrome/browser/ui/views/extensions/extensions_menu_main_page_view.cc>
