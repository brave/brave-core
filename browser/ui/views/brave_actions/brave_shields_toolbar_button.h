// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_TOOLBAR_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_TOOLBAR_BUTTON_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_controller.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BrowserWindowInterface;

namespace gfx {
class Point;
}  // namespace gfx

namespace views {
class Widget;
}  // namespace views

// PWA / standalone web app Shields control. Uses ToolbarButton for Chrome's
// toolbar-button visuals and behavior, but is parented under the web app frame
// title bar (e.g. WebAppToolbarButtonContainer), not the tabbed window's
// ToolbarView / omnibox row.
class BraveShieldsToolbarButton : public ToolbarButton {
  METADATA_HEADER(BraveShieldsToolbarButton, ToolbarButton)
 public:
  using CreateWebUIBubbleManagerCallback =
      BraveShieldsActionController::CreateWebUIBubbleManagerCallback;

  BraveShieldsToolbarButton(
      BrowserWindowInterface* browser_window_interface,
      CreateWebUIBubbleManagerCallback create_bubble_manager_callback);
  ~BraveShieldsToolbarButton() override;

  void Update();

  views::Widget* GetBubbleWidget();

  // ToolbarButton
  void OnThemeChanged() override;
  std::u16string GetRenderedTooltipText(const gfx::Point& p) const override;

 private:
  void OnControllerStateChanged();
  void ButtonPressed();

  std::unique_ptr<BraveShieldsActionController> controller_;
  base::WeakPtrFactory<BraveShieldsToolbarButton> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_TOOLBAR_BUTTON_H_
