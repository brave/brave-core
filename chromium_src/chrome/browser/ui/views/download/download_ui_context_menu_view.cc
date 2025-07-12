/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/download_ui_context_menu_view.h"

#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/download/bubble/download_bubble_prefs.h"
#include "chrome/browser/download/download_ui_model.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/menus/simple_menu_model.h"

#define DownloadUiContextMenuView DownloadUiContextMenuViewChromium

#include "src/chrome/browser/ui/views/download/download_ui_context_menu_view.cc"

#undef DownloadUiContextMenuView

DownloadUiContextMenuView::~DownloadUiContextMenuView() = default;

ui::SimpleMenuModel* DownloadUiContextMenuView::GetMenuModel() {
  auto* model = DownloadUiContextMenuViewChromium::GetMenuModel();
  if (!model) {
    return nullptr;
  }

  // Only add "Remove item from list" entry to download bubble.
  if (!download::IsDownloadBubbleEnabled()) {
    return model;
  }

  auto* download = GetDownload();
  if (download->GetDownloadItem() == nullptr) {
    return model;
  }

  // Early return if |model| has the item. |model| is cached by base class.
  if (auto index = model->GetIndexOfCommandId(
          static_cast<int>(BraveDownloadCommands::REMOVE_FROM_LIST))) {
    return model;
  }

  // Don't add "Remove item fro list" entry to in-progress state.
  if (download->GetState() != download::DownloadItem::COMPLETE &&
      download->GetState() != download::DownloadItem::CANCELLED) {
    return model;
  }

  if (auto index =
          model->GetIndexOfCommandId(DownloadCommands::SHOW_IN_FOLDER)) {
    model->InsertItemAt(
        *index + 1, static_cast<int>(BraveDownloadCommands::REMOVE_FROM_LIST),
        l10n_util::GetStringUTF16(
            IDS_DOWNLOAD_BUBBLE_ITEM_CTX_MENU_REMOVE_ITEM));
  }

  return model;
}

bool DownloadUiContextMenuView::IsCommandIdEnabled(int command_id) const {
  if (static_cast<BraveDownloadCommands>(command_id) ==
      BraveDownloadCommands::REMOVE_FROM_LIST) {
    return true;
  }

  return DownloadUiContextMenuViewChromium::IsCommandIdEnabled(command_id);
}

bool DownloadUiContextMenuView::IsCommandIdChecked(int command_id) const {
  if (static_cast<BraveDownloadCommands>(command_id) ==
      BraveDownloadCommands::REMOVE_FROM_LIST) {
    return false;
  }

  return DownloadUiContextMenuViewChromium::IsCommandIdChecked(command_id);
}

bool DownloadUiContextMenuView::IsCommandIdVisible(int command_id) const {
  if (static_cast<BraveDownloadCommands>(command_id) ==
      BraveDownloadCommands::REMOVE_FROM_LIST) {
    return true;
  }

  return DownloadUiContextMenuViewChromium::IsCommandIdVisible(command_id);
}

void DownloadUiContextMenuView::ExecuteCommand(int command_id,
                                               int event_flags) {
  if (static_cast<BraveDownloadCommands>(command_id) ==
      BraveDownloadCommands::REMOVE_FROM_LIST) {
    GetDownload()->GetDownloadItem()->Remove();
    return;
  }

  DownloadUiContextMenuViewChromium::ExecuteCommand(command_id, event_flags);
}
