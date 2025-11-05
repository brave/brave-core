// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/download/download_item_model.h"

#include "chrome/browser/download/download_commands.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

#define DownloadItemModel DownloadItemModel_Chromium

// Add switch-case handling for Brave-specific commands.
// These cases are not used by the DownloadItemModel_Chromium, so just fall
// through.
#define EDIT_WITH_MEDIA_APP                 \
  EDIT_WITH_MEDIA_APP:                      \
  case DownloadCommands::REMOVE_FROM_LIST:  \
  case DownloadCommands::DELETE_LOCAL_FILE: \
  case DownloadCommands::COPY_DOWNLOAD_LINK

#include <chrome/browser/download/download_item_model.cc>

#undef EDIT_WITH_MEDIA_APP
#undef DownloadItemModel

// static
DownloadUIModel::DownloadUIModelPtr DownloadItemModel::Wrap(
    download::DownloadItem* download) {
  return std::make_unique<DownloadItemModel>(download);
}

// static
DownloadUIModel::DownloadUIModelPtr DownloadItemModel::Wrap(
    download::DownloadItem* download,
    std::unique_ptr<DownloadUIModel::StatusTextBuilderBase>
        status_text_builder) {
  return std::make_unique<DownloadItemModel>(download,
                                             std::move(status_text_builder));
}

void DownloadItemModel::DeleteLocalFile() {
  // Passing base::DoNothing() as a callback because we don't have follow-up
  // actions to take after the deletion.
  // In case of success, DownloadItemModel will be updated by itself.
  // On the other hand, if the deletion fails, we don't have to do anything.
  // Note that we're calling non-const version of GetDownloadItem() here.
  DownloadUIModel::GetDownloadItem()->DeleteFile(base::DoNothing());
}

void DownloadItemModel::CopyDownloadLinkToClipboard() {
  auto url = GetURL();
  CHECK(url.is_valid())
      << "This call must be reached only when the URL is valid.";
  ui::ScopedClipboardWriter clipboard_writer(ui::ClipboardBuffer::kCopyPaste);
  clipboard_writer.WriteText(base::UTF8ToUTF16(url.spec()));
}

#if !BUILDFLAG(IS_ANDROID)
bool DownloadItemModel::IsCommandEnabled(
    const DownloadCommands* download_commands,
    DownloadCommands::Command command) const {
  if (command == DownloadCommands::DELETE_LOCAL_FILE) {
    return GetState() == download::DownloadItem::COMPLETE &&
           !GetFileExternallyRemoved() && !GetFullPath().empty();
  }

  if (command == DownloadCommands::REMOVE_FROM_LIST) {
    return true;
  }

  if (command == DownloadCommands::COPY_DOWNLOAD_LINK) {
    return GetURL().is_valid();
  }

  return DownloadItemModel_Chromium::IsCommandEnabled(download_commands,
                                                      command);
}

void DownloadItemModel::ExecuteCommand(DownloadCommands* download_commands,
                                       DownloadCommands::Command command) {
  if (command == DownloadCommands::DELETE_LOCAL_FILE) {
    DeleteLocalFile();
    return;
  }

  if (command == DownloadCommands::REMOVE_FROM_LIST) {
    // Calls non-const version of GetDownloadItem() here.
    DownloadUIModel::GetDownloadItem()->Remove();
    return;
  }

  if (command == DownloadCommands::COPY_DOWNLOAD_LINK) {
    CopyDownloadLinkToClipboard();
    return;
  }

  DownloadItemModel_Chromium::ExecuteCommand(download_commands, command);
}
#endif  // !BUILDFLAG(IS_ANDROID)
