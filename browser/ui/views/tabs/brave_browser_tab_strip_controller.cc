/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"

#include <utility>

#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/brave_tab_context_menu_contents.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/menus/simple_menu_model.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/gfx/geometry/rect.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/base/emoji/emoji_panel_helper.h"

BraveBrowserTabStripController::BraveBrowserTabStripController(
    TabStripModel* model,
    BrowserView* browser_view,
    std::unique_ptr<TabMenuModelFactory> menu_model_factory_override)
    : BrowserTabStripController(model,
                                browser_view,
                                std::move(menu_model_factory_override)) {}

BraveBrowserTabStripController::~BraveBrowserTabStripController() {
  if (context_menu_contents_) {
    context_menu_contents_->Cancel();
  }
}

const std::optional<int> BraveBrowserTabStripController::GetModelIndexOf(
    Tab* tab) {
  return tabstrip_->GetModelIndexOf(tab);
}

void BraveBrowserTabStripController::EnterTabRenameModeAt(int index) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveRenamingTabs));
  return static_cast<BraveTabStrip*>(tabstrip_)->EnterTabRenameModeAt(index);
}

void BraveBrowserTabStripController::SetCustomTitleForTab(
    int index,
    const std::optional<std::u16string>& title) {
  static_cast<BraveTabStripModel*>(model_.get())
      ->SetCustomTitleForTab(index, title);
}

void BraveBrowserTabStripController::OpenEmojiPickerForTab(int index) {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveEmojiTabFavicon)) {
    return;
  }
  const std::optional<int> model_index = tabstrip_->GetModelIndexOf(tabstrip_->tab_at(index));
  if (!model_index) {
    return;
  }
  ShowNativeEmojiPickerWithCapture(*model_index);
}

void BraveBrowserTabStripController::SetCustomEmojiFaviconForTab(
    int index,
    const std::optional<std::u16string>& emoji) {
  static_cast<BraveTabStripModel*>(model_.get())
      ->SetCustomEmojiFaviconForTab(index, emoji);
}

void BraveBrowserTabStripController::ShowContextMenuForTab(
    Tab* tab,
    const gfx::Point& p,
    ui::mojom::MenuSourceType source_type) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  const auto tab_index = browser_view->tabstrip()->GetModelIndexOf(tab);
  if (!tab_index) {
    return;
  }
  context_menu_contents_ =
      std::make_unique<BraveTabContextMenuContents>(tab, this, *tab_index);
  context_menu_contents_->RunMenuAt(p, source_type);
}

void BraveBrowserTabStripController::ExecuteCommandForTab(
    TabStripModel::ContextMenuCommand command_id,
    const Tab* tab) {
  const std::optional<int> model_index = tabstrip_->GetModelIndexOf(tab);
  if (!model_index.has_value()) {
    return;
  }

  if (command_id == TabStripModel::CommandCloseTab) {
    model_->CloseSelectedTabsWithSplitView();
    return;
  }

  model_->ExecuteContextMenuCommand(model_index.value(), command_id);
}

void BraveBrowserTabStripController::ShowEmojiPickerMenu(int model_index) {
  emoji_target_model_index_ = model_index;
  emoji_items_ = {u"😀", u"😁", u"😂", u"😍", u"😎", u"🤔", u"👍", u"🔥"};
  emoji_menu_model_ = std::make_unique<ui::SimpleMenuModel>(this);
  int command_base = 10000;  // Arbitrary high range to avoid conflicts
  for (size_t i = 0; i < emoji_items_.size(); ++i) {
    emoji_menu_model_->AddItem(command_base + static_cast<int>(i), emoji_items_[i]);
  }

  views::View* anchor = tabstrip_->tab_at(model_index);
  emoji_menu_runner_ = std::make_unique<views::MenuRunner>(
      emoji_menu_model_.get(), views::MenuRunner::CONTEXT_MENU);
  emoji_menu_runner_->RunMenuAt(anchor->GetWidget(), nullptr,
                                anchor->GetBoundsInScreen(),
                                views::MenuAnchorPosition::kTopLeft,
                                ui::mojom::MenuSourceType::kMouse);
}

void BraveBrowserTabStripController::ShowNativeEmojiPickerWithCapture(
    int model_index) {
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  // Create a tiny visible textfield to capture the chosen emoji; it must be
  // focused and attached to the widget to receive input from OS emoji panel.
  views::View* anchor = tabstrip_->tab_at(model_index);
  emoji_capture_model_index_ = model_index;
  views::Widget* widget = anchor->GetWidget();
  if (!widget) {
    return;
  }
  // Use a transient frameless widget so we don't disturb existing view tree and
  // avoid blocking UI. It will self-destruct after capture.
  if (!emoji_capture_widget_) {
    views::Widget::InitParams params(
        views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET,
        views::Widget::InitParams::TYPE_POPUP);
    params.child = true;
    params.accept_events = true;
    params.activatable = views::Widget::InitParams::Activatable::kYes;
    params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
    params.parent = widget->GetNativeView();
    emoji_capture_widget_ = new views::Widget;
    emoji_capture_widget_->Init(std::move(params));
    auto textfield = std::make_unique<views::Textfield>();
    textfield->set_controller(this);
    textfield->SetBoundsRect(gfx::Rect(0, 0, 1, 1));
    emoji_capture_field_ =
        emoji_capture_widget_->SetContentsView(std::move(textfield));
  }
  const gfx::Rect tab_screen_bounds = anchor->GetBoundsInScreen();
  emoji_capture_widget_->SetBounds(gfx::Rect(tab_screen_bounds.origin(), gfx::Size(1, 1)));
  emoji_capture_widget_->Show();
  emoji_capture_widget_->Activate();
  emoji_capture_field_->SetText(std::u16string());
  emoji_capture_field_->RequestFocus();

  // Show native OS emoji panel targeting the capture widget
  emoji_capture_widget_->ShowEmojiPanel();
#else
  ShowEmojiPickerMenu(model_index);
#endif
}

bool BraveBrowserTabStripController::IsCommandIdEnabled(int command_id) const {
  // All emojis are selectable
  return true;
}

void BraveBrowserTabStripController::ExecuteCommand(int command_id, int) {
  if (emoji_target_model_index_ < 0) {
    return;
  }
  int command_base = 10000;
  int index = command_id - command_base;
  if (index < 0 || index >= static_cast<int>(emoji_items_.size())) {
    return;
  }
  SetCustomEmojiFaviconForTab(emoji_target_model_index_, emoji_items_[index]);
  emoji_target_model_index_ = -1;
}

void BraveBrowserTabStripController::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  if (sender != emoji_capture_field_) {
    return;
  }
  if (emoji_capture_model_index_ < 0) {
    return;
  }
  if (!new_contents.empty()) {
    SetCustomEmojiFaviconForTab(emoji_capture_model_index_,
                                new_contents);
    emoji_capture_model_index_ = -1;
    // Close OS emoji panel if possible.
    ui::HideEmojiPanel();
    // Destroy the transient capture widget.
    if (emoji_capture_widget_) {
      emoji_capture_widget_->CloseNow();
      emoji_capture_widget_ = nullptr;
      emoji_capture_field_ = nullptr;
    }
  }
}
