/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/download/bubble/download_display_controller.h"

#define DownloadDisplayController DownloadDisplayControllerChromium

#include "chrome/browser/download/bubble/download_bubble_display_info.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
#include "src/chrome/browser/download/bubble/download_display_controller.cc"

#undef DownloadDisplayController

void DownloadDisplayController::UpdateToolbarButtonState(
    const DownloadBubbleDisplayInfo& info,
    const DownloadDisplay::ProgressInfo& progress_info) {
  DownloadDisplayControllerChromium::UpdateToolbarButtonState(info,
                                                              progress_info);

  if (info.all_models_size == 0) {
    return;
  }

  if (display_->IsShowing()) {
    return;
  }

  // Show toolbar if there's at least one in-progress download item.
  // Upstream doesn't show toolbar button when only dangerous files are
  // in-progress. We cannot use DownloadBubbleDisplayInfo's in_progress_count
  // here because it doesn't include dangerous files.
  std::vector<DownloadUIModel::DownloadUIModelPtr> all_models;
  bubble_controller_->update_service()->GetAllModelsToDisplay(
      all_models, GetWebAppIdForBrowser(browser_),
      /*force_backfill_download_items=*/true);
  DCHECK(!all_models.empty());
  for (const auto& model : all_models) {
    if (model->GetState() == download::DownloadItem::IN_PROGRESS) {
      ShowToolbarButton();
      return;
    }
  }
}
