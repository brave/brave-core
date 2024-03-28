/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/bookmarks/bookmark_button.h"

#include "ui/compositor/layer.h"
#include "ui/views/controls/highlight_path_generator.h"

// Skips highlightPath. The default highlight path is what we want.
// And in order to render label clearly over the ink drop, it should have its
// own layer. Otherwise, the ink drop will be rendered over the label.
#define InstallPillHighlightPathGenerator(view)       \
  Label* bookmark_label = label();                    \
  bookmark_label->SetPaintToLayer();                  \
  bookmark_label->SetSubpixelRenderingEnabled(false); \
  bookmark_label->layer()->SetFillsBoundsOpaquely(false);

#include "src/chrome/browser/ui/views/bookmarks/bookmark_button.cc"

#undef InstallPillHighlightPathGenerator
