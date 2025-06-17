/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/vector_icons/vector_icons.h"

// Pass the correct icon size to ui::ImageModel::FromVectorIcon since our icons
// have a larger default size but the upstream code assumes a size of 16x16 for
// this icon
#define kSubmenuArrowChromeRefreshIcon                                  \
  kSubmenuArrowChromeRefreshIcon, colors.icon_color, 16));              \
  if (false)                                                            \
    submenu_arrow_image_view_->SetImage(ui::ImageModel::FromVectorIcon( \
        vector_icons::kSubmenuArrowChromeRefreshIcon

#include <ui/views/controls/menu/menu_item_view.cc>

#undef kSubmenuArrowChromeRefreshIcon
