// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_

#include "chrome/browser/download/download_ui_model.h"

// Replaces DownloadItemModel imeplementation from Upstream with Brave's one.
#define DownloadItemModel DownloadItemModel_Chromium

#include <chrome/browser/download/download_item_model.h>  // IWYU pragma: export

#undef DownloadItemModel

class DownloadItemModel : public DownloadItemModel_Chromium {
 public:
  using DownloadItemModel_Chromium::DownloadItemModel_Chromium;
  ~DownloadItemModel() override = default;

  // Provides factory methods to create DownloadItemModel,
  // not DownloadItemModel_Chromium.
  static DownloadUIModelPtr Wrap(download::DownloadItem* download);
  static DownloadUIModelPtr Wrap(
      download::DownloadItem* download,
      std::unique_ptr<DownloadUIModel::StatusTextBuilderBase>
          status_text_builder);

  // Deletes the local file associated with this download.
  void DeleteLocalFile() override;
#if !BUILDFLAG(IS_ANDROID)
  bool IsCommandEnabled(const DownloadCommands* download_commands,
                        DownloadCommands::Command command) const override;
  void ExecuteCommand(DownloadCommands* download_commands,
                      DownloadCommands::Command command) override;
#endif  // !BUILDFLAG(IS_ANDROID)
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_
