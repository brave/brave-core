/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/download/bubble/download_display_controller.h"

#define DownloadDisplayController DownloadDisplayControllerChromium

#include "src/chrome/browser/download/bubble/download_display_controller.cc"

#undef DownloadDisplayController

void DownloadDisplayController::UpdateToolbarButtonState(
    std::vector<std::unique_ptr<DownloadUIModel>>& all_models) {
  DownloadDisplayControllerChromium::UpdateToolbarButtonState(all_models);

  if (all_models.empty()) {
    return;
  }

  if (display_->IsShowing()) {
    return;
  }

  // Show toolbar if there's at least one in-progress download item.
  // Upstream doesn't show toolbar button when only dangerous files are
  // in-progress.
  for (const auto& model : all_models) {
    if (model->GetState() == download::DownloadItem::IN_PROGRESS) {
      ShowToolbarButton();
      return;
    }
  }
}
