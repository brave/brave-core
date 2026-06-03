// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/widget/widget.h"

class BraveShieldsActionView : public views::LabelButton {
  METADATA_HEADER(BraveShieldsActionView, views::LabelButton)
 public:
  DECLARE_CLASS_ELEMENT_IDENTIFIER_VALUE(kShieldsActionIcon);

  using CreateWebUIBubbleManagerCallback =
      BraveShieldsActionController::CreateWebUIBubbleManagerCallback;

  BraveShieldsActionView(
      BrowserWindowInterface* browser_window_interface,
      views::LayoutProvider& layout_provider,
      CreateWebUIBubbleManagerCallback create_bubble_manager_callback);
  BraveShieldsActionView(const BraveShieldsActionView&) = delete;
  BraveShieldsActionView& operator=(const BraveShieldsActionView&) = delete;
  ~BraveShieldsActionView() override;

  void Init();
  void Update();

  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override;
  std::u16string GetRenderedTooltipText(const gfx::Point& p) const override;
  void OnThemeChanged() override;

  SkPath GetHighlightPath() const;
  views::Widget* GetBubbleWidget();

 private:
  void OnControllerStateChanged();
  void ButtonPressed();

  const raw_ptr<BrowserWindowInterface> browser_window_interface_ = nullptr;
  std::unique_ptr<BraveShieldsActionController> controller_;
  raw_ref<views::LayoutProvider> layout_provider_;
  base::WeakPtrFactory<BraveShieldsActionView> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_SHIELDS_ACTION_VIEW_H_
