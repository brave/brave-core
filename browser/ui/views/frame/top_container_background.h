/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_TOP_CONTAINER_BACKGROUND_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_TOP_CONTAINER_BACKGROUND_H_

#include "brave/browser/ui/views/view_shadow.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

// Backs the horizontal tab strip and top container while they slide in/out
// during focus mode reveal, so the contents layer is not visible behind them.
// Also renders a drop shadow beneath those views along the lower edge.
class TopContainerBackground : public views::View {
  METADATA_HEADER(TopContainerBackground, views::View)

 public:
  TopContainerBackground();
  TopContainerBackground(const TopContainerBackground&) = delete;
  TopContainerBackground& operator=(const TopContainerBackground&) = delete;
  ~TopContainerBackground() override;

 private:
  ViewShadow shadow_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_TOP_CONTAINER_BACKGROUND_H_
