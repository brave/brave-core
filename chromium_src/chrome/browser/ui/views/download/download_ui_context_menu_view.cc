/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/download_ui_context_menu_view.h"

#include "brave/browser/download/brave_download_commands.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/download/bubble/download_bubble_prefs.h"
#include "chrome/browser/download/download_ui_model.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/menus/simple_menu_model.h"

#define DownloadUiContextMenuView DownloadUiContextMenuViewChromium

#include <chrome/browser/ui/views/download/download_ui_context_menu_view.cc>

#undef DownloadUiContextMenuView

DownloadUiContextMenuView::~DownloadUiContextMenuView() = default;

ui::SimpleMenuModel* DownloadUiContextMenuView::GetMenuModel() {
  auto* model = DownloadUiContextMenuViewChromium::GetMenuModel();
  if (!model) {
    return nullptr;
  }

  auto* download = GetDownload();
  if (download->GetDownloadItem() == nullptr) {
    return model;
  }

  MaybeAddCopyDownloadLinkMenuItem(download, model);
  MaybeAddRemoveFromListMenuItem(download, model);
  return model;
}

void DownloadUiContextMenuView::MaybeAddRemoveFromListMenuItem(
    DownloadUIModel* download,
    ui::SimpleMenuModel* model) {
  // Early return if |model| has the item. |model| is cached by base class.
  if (AlreadyHasCommand(BraveDownloadCommands::REMOVE_FROM_LIST, model)) {
    return;
  }

  // Don't add "Remove item from list" entry to in-progress state.
  if (download->GetState() != download::DownloadItem::COMPLETE &&
      download->GetState() != download::DownloadItem::CANCELLED) {
    return;
  }

  if (auto index = model->GetIndexOfCommandId(
          BraveDownloadCommands::COPY_DOWNLOAD_LINK)) {
    model->InsertItemAt(*index + 1, BraveDownloadCommands::REMOVE_FROM_LIST,
                        l10n_util::GetStringUTF16(
                            IDS_DOWNLOAD_BUBBLE_ITEM_CTX_MENU_REMOVE_ITEM));
  }
}

void DownloadUiContextMenuView::MaybeAddCopyDownloadLinkMenuItem(
    DownloadUIModel* download,
    ui::SimpleMenuModel* model) {
  // Early return if |model| has the item. |model| is cached by base class.
  if (AlreadyHasCommand(BraveDownloadCommands::COPY_DOWNLOAD_LINK, model)) {
    return;
  }

  if (auto index =
          model->GetIndexOfCommandId(DownloadCommands::SHOW_IN_FOLDER)) {
    model->InsertItemAt(
        *index + 1, BraveDownloadCommands::COPY_DOWNLOAD_LINK,
        l10n_util::GetStringUTF16(
            IDS_DOWNLOAD_BUBBLE_ITEM_CTX_MENU_COPY_DOWNLOAD_LINK));
  }
}

bool DownloadUiContextMenuView::AlreadyHasCommand(
    int command_id,
    ui::SimpleMenuModel* model) const {
  return model && model->GetIndexOfCommandId(command_id).has_value();
}

bool DownloadUiContextMenuView::IsCommandIdEnabled(int command_id) const {
  if (command_id == BraveDownloadCommands::REMOVE_FROM_LIST) {
    return true;
  }

  return DownloadUiContextMenuViewChromium::IsCommandIdEnabled(command_id);
}

bool DownloadUiContextMenuView::IsCommandIdChecked(int command_id) const {
  if (command_id == BraveDownloadCommands::REMOVE_FROM_LIST) {
    return false;
  }

  return DownloadUiContextMenuViewChromium::IsCommandIdChecked(command_id);
}

bool DownloadUiContextMenuView::IsCommandIdVisible(int command_id) const {
  if (command_id == BraveDownloadCommands::REMOVE_FROM_LIST) {
    return true;
  }

  return DownloadUiContextMenuViewChromium::IsCommandIdVisible(command_id);
}

void DownloadUiContextMenuView::ExecuteCommand(int command_id,
                                               int event_flags) {
  if (command_id == BraveDownloadCommands::REMOVE_FROM_LIST) {
    GetDownload()->GetDownloadItem()->Remove();
    return;
  }

  // Bypass the DownloadUIContextMenuViewChromium implementation so that
  // we can avoid histogramming for Brave specific commands.
  DownloadUiContextMenu::ExecuteCommand(command_id, event_flags);
}
