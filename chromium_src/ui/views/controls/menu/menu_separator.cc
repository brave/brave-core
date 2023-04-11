// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#define BRAVE_MENU_SEPARATOR_ON_PAINT            \
  if (type_ == ui::BOTH_SIDE_PADDED_SEPARATOR) { \
    paint_rect.Inset(gfx::Insets::VH(0, 8));     \
  } else

#include "src/ui/views/controls/menu/menu_separator.cc"  // IWYU pragma: export

#undef BRAVE_MENU_SEPARATOR_ON_PAINT
