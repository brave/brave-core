/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/download/bubble/download_display_controller.h"

namespace {

bool ShouldShowToolbarButtonForInProgressDownload(
    const DownloadBubbleDisplayInfo& info,
    DownloadDisplay* display,
    DownloadBubbleUIController* bubble_controller,
    const BrowserWindowInterface* browser);

}  // namespace

#include <chrome/browser/download/bubble/download_display_controller.cc>

namespace {

bool ShouldShowToolbarButtonForInProgressDownload(
    const DownloadBubbleDisplayInfo& info,
    DownloadDisplay* display,
    DownloadBubbleUIController* bubble_controller,
    const BrowserWindowInterface* browser) {
  if (info.all_models_size == 0) {
    return false;
  }

  if (display->IsShowing()) {
    return false;
  }

  // Show the toolbar button if there's at least one in-progress download item.
  // Upstream doesn't show it when only dangerous files are in-progress, and we
  // can't use DownloadBubbleDisplayInfo::in_progress_count because it excludes
  // dangerous files.
  std::vector<DownloadUIModel::DownloadUIModelPtr> all_models;
  bubble_controller->update_service()->GetAllModelsToDisplay(
      all_models, GetWebAppIdForBrowser(browser),
      /*force_backfill_download_items=*/true);
  for (const auto& model : all_models) {
    if (model->GetState() == download::DownloadItem::IN_PROGRESS) {
      return true;
    }
  }
  return false;
}

}  // namespace
