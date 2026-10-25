/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_SHIELDS_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_SHIELDS_BUTTON_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class BrowserWindowInterface;
class PeekShieldsPanelProvider;
class WebUIBubbleManager;

namespace content {
class WebContents;
}  // namespace content

// Shows the Shields icon and blocked count for the Peek contents, and opens
// the Shields panel for it.
class PeekShieldsButton : public views::LabelButton,
                          public brave_shields::BraveShieldsTabHelper::Observer,
                          public views::WidgetObserver {
  METADATA_HEADER(PeekShieldsButton, views::LabelButton)

 public:
  PeekShieldsButton(BrowserWindowInterface& browser,
                    PeekShieldsPanelProvider& provider);
  PeekShieldsButton(const PeekShieldsButton&) = delete;
  PeekShieldsButton& operator=(const PeekShieldsButton&) = delete;
  ~PeekShieldsButton() override;

  void SetWebContents(content::WebContents* contents);
  views::Widget* GetBubbleWidget();

  // views::LabelButton:
  std::u16string GetRenderedTooltipText(const gfx::Point& p) const override;
  void OnThemeChanged() override;

 private:
  void ButtonPressed();
  void UpdateImage();

  // brave_shields::BraveShieldsTabHelper::Observer:
  void OnResourcesChanged() override;
  void OnShieldsEnabledChanged() override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  const raw_ref<BrowserWindowInterface> browser_;
  const raw_ref<PeekShieldsPanelProvider> provider_;
  raw_ptr<views::MenuButtonController> menu_button_controller_ = nullptr;
  base::WeakPtr<content::WebContents> web_contents_;
  std::unique_ptr<WebUIBubbleManager> bubble_manager_;
  // Keeps the button pressed while the panel is loading or open.
  std::unique_ptr<views::MenuButtonController::PressedLock> pressed_lock_;
  base::ScopedObservation<brave_shields::BraveShieldsTabHelper,
                          brave_shields::BraveShieldsTabHelper::Observer>
      shields_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      bubble_widget_observation_{this};
  base::WeakPtrFactory<PeekShieldsButton> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_SHIELDS_BUTTON_H_
