/* Copyright (c) 2025 The Brave Authors. */
/* This Source Code Form is subject to the terms of the Mozilla Public */
/* License, v. 2.0. If a copy of the MPL was not distributed with this file, */
/* You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/emoji_favicon_picker_bubble.h"

#include <optional>
#include <string>

#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/brave_browser_tab_strip_controller.h"
#include "ui/base/ime/text_input_flags.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"

// static
void EmojiFaviconPickerBubble::Show(views::View* anchor_view,
                                    BraveBrowserTabStripController* controller,
                                    int tab_index) {
  if (!anchor_view) {
    return;
  }
  auto* bubble = new EmojiFaviconPickerBubble(anchor_view, controller, tab_index);
  views::BubbleDialogDelegateView::CreateBubble(bubble)->Show();
}

EmojiFaviconPickerBubble::EmojiFaviconPickerBubble(
    views::View* anchor_view,
    BraveBrowserTabStripController* controller,
    int tab_index)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_LEFT),
      controller_(controller),
      tab_index_(tab_index) {
  SetButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
  SetAcceptCallback(base::BindOnce(&EmojiFaviconPickerBubble::Accept,
                                   weak_factory_.GetWeakPtr()));

  auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));
  layout->set_between_child_spacing(8);

  input_ = AddChildView(std::make_unique<views::Textfield>());
  input_->SetPlaceholderText(u"Enter an emoji");
  input_->SetTextInputType(ui::TextInputType::TEXT);
}

EmojiFaviconPickerBubble::~EmojiFaviconPickerBubble() = default;

bool EmojiFaviconPickerBubble::Accept() {
  if (!controller_) {
    return true;
  }
  std::u16string value = input_->GetText();
  if (value.empty()) {
    controller_->SetCustomEmojiFaviconForTab(tab_index_, std::nullopt);
  } else {
    controller_->SetCustomEmojiFaviconForTab(tab_index_, value);
  }
  return true;
}


