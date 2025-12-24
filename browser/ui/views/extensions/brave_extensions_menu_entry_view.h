// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSIONS_MENU_ENTRY_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSIONS_MENU_ENTRY_VIEW_H_

#include "chrome/browser/ui/views/extensions/extensions_menu_entry_view.h"

class BraveExtensionsMenuEntryView : public ExtensionsMenuEntryView {
 public:
  METADATA_HEADER(BraveExtensionsMenuEntryView, ExtensionsMenuEntryView)

 public:
  explicit BraveExtensionsMenuEntryView(
      Browser* browser,
      bool is_enterprise,
      ToolbarActionViewModel* view_model,
      base::RepeatingCallback<void(bool)> site_access_toggle_callback,
      views::Button::PressedCallback site_permissions_button_callback);
  ~BraveExtensionsMenuEntryView() override;

  // Overrides ExtensionsMenuEntryView:
  void UpdateContextMenuButton(
      ExtensionsMenuViewModel::ControlState button_state) override;
};

BEGIN_VIEW_BUILDER(/* no export */,
                   BraveExtensionsMenuEntryView,
                   ExtensionsMenuEntryView)
END_VIEW_BUILDER

DEFINE_VIEW_BUILDER(/* no export */, BraveExtensionsMenuEntryView)

#endif  // BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSIONS_MENU_ENTRY_VIEW_H_
