/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/color/color_provider.h"
#include "ui/views/cascading_property.h"

namespace views {

SkColor GetTabSpinningColor(views::View* view) {
  CHECK(view && view->GetColorProvider());
  return view->GetColorProvider()->GetColor(kColorToolbarButtonIcon);
}

}  // namespace views

#define GetCascadingAccentColor GetTabSpinningColor

#include "src/chrome/browser/ui/views/tabs/tab_icon.cc"

#undef GetCascadingAccentColor
