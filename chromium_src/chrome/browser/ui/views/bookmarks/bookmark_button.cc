/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/bookmarks/bookmark_button.h"

#include "ui/compositor/layer.h"
#include "ui/views/controls/highlight_path_generator.h"

namespace views {
void InstallNoHighlightPathGenerator(View* view) {
  // Do nothing: the default highlight path is what we want.
}
}  // namespace views

#define InstallPillHighlightPathGenerator InstallNoHighlightPathGenerator
#define BookmarkButtonBase BookmarkButtonBase_ChromiumImpl
#define BookmarkButton BookmarkButton_ChromiumImpl
#include "src/chrome/browser/ui/views/bookmarks/bookmark_button.cc"
#undef BookmarkButton
#undef BookmarkButtonBase
#undef InstallPillHighlightPathGenerator

BookmarkButtonBase::BookmarkButtonBase(PressedCallback callback,
                                       const std::u16string& title)
    : BookmarkButtonBase_ChromiumImpl(std::move(callback), title) {
  // To render label clearly over the ink drop, it should have its own layer.
  // Otherwise, the ink drop will be rendered over the label.
  label()->SetPaintToLayer();
  label()->SetSubpixelRenderingEnabled(false);
  label()->layer()->SetFillsBoundsOpaquely(false);
}

BookmarkButtonBase::~BookmarkButtonBase() = default;

BEGIN_METADATA(BookmarkButtonBase)
END_METADATA

BookmarkButton::BookmarkButton(PressedCallback callback,
                               const GURL& url,
                               const std::u16string& title,
                               const raw_ptr<Browser> browser)
    : BookmarkButton_ChromiumImpl(std::move(callback), url, title, browser) {
  // To render label clearly over the ink drop, it should have its own layer.
  // Otherwise, the ink drop will be rendered over the label.
  label()->SetPaintToLayer();
  label()->SetSubpixelRenderingEnabled(false);
  label()->layer()->SetFillsBoundsOpaquely(false);
}

BookmarkButton::~BookmarkButton() = default;

BEGIN_METADATA(BookmarkButton)
END_METADATA
