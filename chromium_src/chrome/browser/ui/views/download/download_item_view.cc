/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/download_item_view.h"

namespace views {
namespace {
class DownloadItemViewButton : public Button {
 public:
  using Button::Button;

  void OnMouseEntered(const ui::MouseEvent& event) override {
    parent()->OnMouseEntered(event);
  }
  void OnMouseExited(const ui::MouseEvent& event) override {
    parent()->OnMouseExited(event);
  }
  void OnViewFocused(View* observed_view) override {
    auto* item = static_cast<DownloadItemView*>(parent());
    item->OnViewFocused(observed_view);
  }
  void OnViewBlurred(View* observed_view) override {
    auto* item = static_cast<DownloadItemView*>(parent());
    item->OnViewBlurred(observed_view);
  }
};

}  // namespace

}  // namespace views

#define Button DownloadItemViewButton
#include "../../../../../../../chrome/browser/ui/views/download/download_item_view.cc"
#undef Button

bool DownloadItemView::IsShowingWarningDialog() const {
  return has_warning_label(mode_);
}
