// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <chrome/browser/ui/views/page_action/page_action_view.cc>

namespace page_actions {

bool PageActionView::ShouldShowLabel() const {
  if (ShouldAlwaysShowLabel()) {
    return true;
  }
  return IconLabelBubbleView::ShouldShowLabel();
}

bool PageActionView::ShouldAlwaysShowLabel() const {
  const PageActionModelInterface* source = observation_.GetSource();
  if (source && source->GetAlwaysShowLabel()) {
    return true;
  }

  return IconLabelBubbleView::ShouldAlwaysShowLabel();
}

SkColor PageActionView::GetBackgroundColor() const {
  const PageActionModelInterface* source = observation_.GetSource();
  if (source && source->GetOverrideBackgroundColor()) {
    return *source->GetOverrideBackgroundColor();
  }

  return IconLabelBubbleView::GetBackgroundColor();
}

SkColor PageActionView::GetForegroundColor() const {
  const PageActionModelInterface* source = observation_.GetSource();
  if (source && source->GetOverrideForegroundColor()) {
    return *source->GetOverrideForegroundColor();
  }

  return IconLabelBubbleView::GetForegroundColor();
}

}  // namespace page_actions
