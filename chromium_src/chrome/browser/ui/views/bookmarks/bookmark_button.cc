/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/views/controls/highlight_path_generator.h"

namespace views {
void InstallNoHighlightPathGenerator(View* view) {
  // Do nothing: the default highlight path is what we want.
}
}  // namespace views

#define InstallPillHighlightPathGenerator InstallNoHighlightPathGenerator
#include "src/chrome/browser/ui/views/bookmarks/bookmark_button.cc"
#undef InstallPillHighlightPathGenerator
