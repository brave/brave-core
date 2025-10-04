/* Copyright (c) 2025 The Brave Authors. */
/* This Source Code Form is subject to the terms of the Mozilla Public */
/* License, v. 2.0. If a copy of the MPL was not distributed with this file, */
/* You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_EMOJI_FAVICON_PICKER_BUBBLE_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_EMOJI_FAVICON_PICKER_BUBBLE_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

namespace views {
class Textfield;
class View;
}

class BraveBrowserTabStripController;

// Simple bubble UI to input a custom emoji for the tab's favicon.
class EmojiFaviconPickerBubble : public views::BubbleDialogDelegateView {
 public:
  // Shows the picker anchored to |anchor_view|. Calls controller to set emoji.
  static void Show(views::View* anchor_view,
                   BraveBrowserTabStripController* controller,
                   int tab_index);

  EmojiFaviconPickerBubble(views::View* anchor_view,
                           BraveBrowserTabStripController* controller,
                           int tab_index);
  EmojiFaviconPickerBubble(const EmojiFaviconPickerBubble&) = delete;
  EmojiFaviconPickerBubble& operator=(const EmojiFaviconPickerBubble&) = delete;
  ~EmojiFaviconPickerBubble() override;

  // views::DialogDelegate overrides:
  bool Accept() override;

 private:
  raw_ptr<BraveBrowserTabStripController> controller_ = nullptr;
  int tab_index_ = -1;
  raw_ptr<views::Textfield> input_ = nullptr;
  base::WeakPtrFactory<EmojiFaviconPickerBubble> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_EMOJI_FAVICON_PICKER_BUBBLE_H_


