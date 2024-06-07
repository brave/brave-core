/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSION_MENU_ITEM_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSION_MENU_ITEM_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/extensions/extensions_menu_item_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveExtensionMenuItemView : public ExtensionMenuItemView {
 public:
  METADATA_HEADER(BraveExtensionMenuItemView, ExtensionMenuItemView)

 public:
  BraveExtensionMenuItemView(
      Browser* browser,
      bool is_enterprise,
      std::unique_ptr<ToolbarActionViewController> controller,
      base::RepeatingCallback<void(bool)> site_access_toggle_callback,
      views::Button::PressedCallback site_permissions_button_callback);

  ~BraveExtensionMenuItemView() override;

  // ExtensionMenuItemView:
  void UpdateContextMenuButton(bool is_action_pinned) override;
  void UpdatePinButton(bool is_force_pinned, bool is_pinned) override;
  void OnThemeChanged() override;

 private:
  void UpdatePinButtonIcon();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSION_MENU_ITEM_VIEW_H_
