/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TOP_OVERLAY_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TOP_OVERLAY_H_

#include "brave/browser/ui/views/view_shadow.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

class FocusModeTopOverlay : public views::View {
  METADATA_HEADER(FocusModeTopOverlay, views::View)

 public:
  FocusModeTopOverlay();
  FocusModeTopOverlay(const FocusModeTopOverlay&) = delete;
  FocusModeTopOverlay& operator=(const FocusModeTopOverlay&) = delete;
  ~FocusModeTopOverlay() override;

 private:
  ViewShadow shadow_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TOP_OVERLAY_H_
