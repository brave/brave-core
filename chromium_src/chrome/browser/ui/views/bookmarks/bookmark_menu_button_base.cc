/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/bookmarks/bookmark_menu_button_base.h"

#include "ui/compositor/layer.h"
#include "ui/views/controls/highlight_path_generator.h"

namespace views {

class View;

void DontInstallHighlightPathGenerator(View* view) {
  // Do nothing: the default highlight path is what we want.
}

}  // namespace views

#define InstallPillHighlightPathGenerator DontInstallHighlightPathGenerator
#define BookmarkMenuButtonBase BookmarkMenuButtonBase_ChromiumImpl
#include "src/chrome/browser/ui/views/bookmarks/bookmark_menu_button_base.cc"
#undef BookmarkMenuButtonBase
#undef InstallPillHighlightPathGenerator

BookmarkMenuButtonBase::BookmarkMenuButtonBase(PressedCallback callback,
                                               const std::u16string& title)
    : BookmarkMenuButtonBase_ChromiumImpl(std::move(callback), title) {
  // To render label clearly over the ink drop, it should have its own layer.
  // Otherwise, the ink drop will be rendered over the label.
  label()->SetPaintToLayer();
  label()->SetSubpixelRenderingEnabled(false);
  label()->layer()->SetFillsBoundsOpaquely(false);
}

BookmarkMenuButtonBase::~BookmarkMenuButtonBase() = default;

BEGIN_METADATA(BookmarkMenuButtonBase)
END_METADATA
