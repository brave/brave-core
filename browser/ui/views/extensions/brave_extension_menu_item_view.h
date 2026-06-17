/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSION_MENU_ITEM_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSION_MENU_ITEM_VIEW_H_

#include <utility>

#include "chrome/browser/ui/views/extensions/extensions_menu_item_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveExtensionMenuItemView : public ExtensionMenuItemView {
 public:
  METADATA_HEADER(BraveExtensionMenuItemView, ExtensionMenuItemView)

 public:
  template <typename... Args>
  explicit BraveExtensionMenuItemView(Args&&... args)
      : ExtensionMenuItemView(std::forward<Args>(args)...) {
    Init();
  }

  ~BraveExtensionMenuItemView() override;

  // ExtensionMenuItemView:
  void UpdatePinButton(bool is_force_pinned, bool is_pinned) override;

 private:
  // Applies Brave-specific styling and refreshes the pin button after the
  // upstream constructor has finished building the view.
  void Init();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSION_MENU_ITEM_VIEW_H_
