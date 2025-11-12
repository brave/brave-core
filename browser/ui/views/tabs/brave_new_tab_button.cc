/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"

#include <utility>

#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/tabs/features.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/paint_vector_icon.h"

using tabs::HorizontalTabsUpdateEnabled;

// static
gfx::Size BraveNewTabButton::GetButtonSize() {
  if (!HorizontalTabsUpdateEnabled()) {
    return {24, 24};
  }
  return {28, 28};
}

gfx::Size BraveNewTabButton::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Overridden so that we use Brave's custom button size
  gfx::Size size = GetButtonSize();
  const auto insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

BraveNewTabButton::BraveNewTabButton(TabStripController* tab_strip_controller,
                                     PressedCallback callback,
                                     const gfx::VectorIcon& icon,
                                     Edge fixed_flat_edge,
                                     Edge animated_flat_edge,
                                     BrowserWindowInterface* browser)
    : NewTabButton(tab_strip_controller,
                   std::move(callback),
                   kLeoPlusAddIcon,
                   fixed_flat_edge,
                   animated_flat_edge,
                   browser) {}

BraveNewTabButton::~BraveNewTabButton() = default;

BEGIN_METADATA(BraveNewTabButton)
END_METADATA
