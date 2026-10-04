// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/download/download_ui_context_menu.h"

#include "chrome/browser/download/download_commands.h"
#include "chrome/browser/download/download_ui_model.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

bool AlreadyHasCommand(int command_id, ui::SimpleMenuModel* model) {
  return model && model->GetIndexOfCommandId(command_id).has_value();
}

void MaybeAddRemoveFromListCommand(DownloadUIModel& download,
                                   ui::SimpleMenuModel* model) {
  // Early return if |model| has the item. |model| is cached by base class.
  if (AlreadyHasCommand(DownloadCommands::REMOVE_FROM_LIST, model)) {
    return;
  }

  // Don't add "Remove item fro list" entry to in-progress state.
  if (download.GetState() != download::DownloadItem::COMPLETE &&
      download.GetState() != download::DownloadItem::CANCELLED) {
    return;
  }

  if (auto index =
          model->GetIndexOfCommandId(DownloadCommands::SHOW_IN_FOLDER)) {
    model->InsertItemAt(
        *index + 1, DownloadCommands::REMOVE_FROM_LIST,
        l10n_util::GetStringUTF16(IDS_DOWNLOAD_DELETE_FROM_HISTORY));
  }
}

void MaybeAddCopyDownloadLinkMenuItem(DownloadUIModel& download,
                                      ui::SimpleMenuModel* model) {
  // Early return if |model| has the item. |model| is cached by base class.
  if (AlreadyHasCommand(DownloadCommands::COPY_DOWNLOAD_LINK, model)) {
    return;
  }

  if (auto index =
          model->GetIndexOfCommandId(DownloadCommands::SHOW_IN_FOLDER)) {
    model->InsertItemAt(
        *index + 1, DownloadCommands::COPY_DOWNLOAD_LINK,
        l10n_util::GetStringUTF16(IDS_DOWNLOAD_COPY_DOWNLOAD_LINK));
  }
}

void InsertBraveSpecificCommandsToModel(
    const base::WeakPtr<DownloadUIModel>& download,
    ui::SimpleMenuModel* model) {
  if (!download) {
    return;
  }

  MaybeAddCopyDownloadLinkMenuItem(*download, model);
  MaybeAddRemoveFromListCommand(*download, model);
}

}  // namespace

#define RecordCommandsEnabled RecordCommandsEnabled_Unused

// Add switch-case handling for Brave-specific commands.
// These cases are not used by the DownloadUIContextMenu, so just fall
// through.
#define EDIT_WITH_MEDIA_APP                 \
  EDIT_WITH_MEDIA_APP:                      \
  case DownloadCommands::REMOVE_FROM_LIST:  \
  case DownloadCommands::DELETE_LOCAL_FILE: \
  case DownloadCommands::COPY_DOWNLOAD_LINK

#define GetMenuModel GetMenuModel_Chromium

#include <chrome/browser/download/download_ui_context_menu.cc>

#undef GetMenuModel
#undef EDIT_WITH_MEDIA_APP
#undef RecordCommandsEnabled

ui::SimpleMenuModel* DownloadUiContextMenu::GetMenuModel() {
  auto* model = GetMenuModel_Chromium();
  InsertBraveSpecificCommandsToModel(download_, model);
  return model;
}
