// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <chrome/browser/ui/views/frame/layout/browser_view_layout_impl.cc>

BrowserViewLayoutImpl::ProposedLayout*
BrowserViewLayoutImpl::ProposedLayout::GetLayoutFor(
    const views::View* descendant) {
  for (auto& child : children) {
    if (child.first == descendant) {
      return &child.second;
    }
    if (auto* result = child.second.GetLayoutFor(descendant)) {
      return result;
    }
  }
  return nullptr;
}
