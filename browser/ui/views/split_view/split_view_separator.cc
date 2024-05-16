/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/split_view/split_view_separator.h"

#include <memory>

#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"

SplitViewSeparator::SplitViewSeparator() : ResizeArea(this) {}

SplitViewSeparator::~SplitViewSeparator() = default;

bool SplitViewSeparator::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton() && event.GetClickCount() == 2) {
    if (resize_area_delegate_) {
      resize_area_delegate_->OnDoubleClicked();
    }
    return true;
  }

  return ResizeArea::OnMousePressed(event);
}

void SplitViewSeparator::OnResize(int resize_amount, bool done_resizing) {
  // When mouse goes toward web contents area, the cursor could have been
  // changed to the normal cursor. Reset it resize cursor.
  GetWidget()->SetCursor(ui::Cursor(ui::mojom::CursorType::kEastWestResize));
  if (resize_area_delegate_) {
    resize_area_delegate_->OnResize(resize_amount, done_resizing);
  }
}

BEGIN_METADATA(SplitViewSeparator)
END_METADATA
