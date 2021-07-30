/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"

#include <algorithm>

#include "ui/gfx/geometry/size.h"

BraveTabSearchButton::~BraveTabSearchButton() = default;

int BraveTabSearchButton::GetCornerRadius() const {
  // Copied from LayoutProvider::GetCornerRadiusMetric() for kMaximum.
  // We override GetCornerRadiusMetric() by BraveLayoutProvider. However,
  // TabSearchButton needs original radius value.
  auto size = GetContentsBounds().size();
  return std::min(size.width(), size.height()) / 2;
}
