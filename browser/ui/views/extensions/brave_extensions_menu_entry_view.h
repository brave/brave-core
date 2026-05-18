// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSIONS_MENU_ENTRY_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSIONS_MENU_ENTRY_VIEW_H_

#include <utility>

#include "chrome/browser/ui/views/extensions/extensions_menu_entry_view.h"

class BraveExtensionsMenuEntryView : public ExtensionsMenuEntryView {
 public:
  METADATA_HEADER(BraveExtensionsMenuEntryView, ExtensionsMenuEntryView)

 public:
  template <typename... Args>
  explicit BraveExtensionsMenuEntryView(Args&&... args)
      : ExtensionsMenuEntryView(std::forward<Args>(args)...) {
    Init();
  }
  ~BraveExtensionsMenuEntryView() override;

  // Overrides ExtensionsMenuEntryView:
  void UpdateContextMenuButton(
      ExtensionsMenuViewModel::ControlState button_state) override;

 private:
  // Applies Brave-specific styling to this entry's child views.
  void Init();
};

BEGIN_VIEW_BUILDER(/* no export */,
                   BraveExtensionsMenuEntryView,
                   ExtensionsMenuEntryView)
END_VIEW_BUILDER

DEFINE_VIEW_BUILDER(/* no export */, BraveExtensionsMenuEntryView)

#endif  // BRAVE_BROWSER_UI_VIEWS_EXTENSIONS_BRAVE_EXTENSIONS_MENU_ENTRY_VIEW_H_
