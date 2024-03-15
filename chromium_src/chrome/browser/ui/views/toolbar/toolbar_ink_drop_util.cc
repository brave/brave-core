/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"

#define ConfigureInkDropForToolbar ConfigureInkDropForToolbar_UnUsed

#include "src/chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.cc"

#undef ConfigureInkDropForToolbar

void ConfigureInkDropForToolbar(
    views::Button* host,
    std::unique_ptr<views::HighlightPathGenerator> highlight_generator) {
  if (!highlight_generator) {
    highlight_generator =
        std::make_unique<ToolbarButtonHighlightPathGenerator>();
  }

  host->SetHasInkDropActionOnClick(true);
  views::HighlightPathGenerator::Install(host, std::move(highlight_generator));
  views::InkDrop::Get(host)->SetMode(views::InkDropHost::InkDropMode::ON);
  CreateToolbarInkdropCallbacks(host, kColorToolbarInkDropHover,
                                kColorToolbarInkDropRipple);
}
