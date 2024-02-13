/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/download_item_view.h"

// Include these headers from the original download_item_view.cc to prevent
// unintentional redefinitions of Button into BraveDownloadItemViewButton.
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/button/md_text_button.h"

namespace views {

namespace {

// This class intercepts mouse/focus events on button and its associated view
// and relays them to BraveDownloadItemView to make decisions on when to hide
// the download item's URL (i.e. default) and when to show it (i.e. on hover).
class BraveDownloadItemViewButton : public Button {
 public:
  METADATA_HEADER(BraveDownloadItemViewButton, Button)
  using Button::Button;

  // Button overrides.
  void OnMouseEntered(const ui::MouseEvent& event) override {
    parent()->OnMouseEntered(event);
    Button::OnMouseEntered(event);
  }

  void OnMouseExited(const ui::MouseEvent& event) override {
    parent()->OnMouseExited(event);
    Button::OnMouseExited(event);
  }

  // ViewObserver overrides.
  void OnViewFocused(View* observed_view) override {
    auto* item = static_cast<DownloadItemView*>(parent());
    item->OnViewFocused(observed_view);
  }

  void OnViewBlurred(View* observed_view) override {
    auto* item = static_cast<DownloadItemView*>(parent());
    item->OnViewBlurred(observed_view);
  }
};

BEGIN_METADATA(BraveDownloadItemViewButton)
END_METADATA

}  // namespace

}  // namespace views

#define Button BraveDownloadItemViewButton
#include "src/chrome/browser/ui/views/download/download_item_view.cc"
#undef Button

bool DownloadItemView::IsShowingWarningDialog() const {
  return has_warning_label(mode_);
}
