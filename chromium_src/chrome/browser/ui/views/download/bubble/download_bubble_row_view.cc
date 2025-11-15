// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/grit/generated_resources.h"
#include "ui/views/view.h"

// Insert logic after SetNotifyEnterOnChild() call in the constructor to add
// quick action for deleting local files.
#define SetNotifyEnterExitOnChild(...)    \
  SetNotifyEnterExitOnChild(__VA_ARGS__); \
  AddQuickAction(DownloadCommands::DELETE_LOCAL_FILE)

// Insert switch case handling for kDeleteLocalFile command in
// DownloadBubbleRowView::GetAccessibleNameForQuickAction().
// We reuse the Chromium's a11y resource id for deleting local files.
constexpr auto kQuickActionAccessibilityResourceId =
    IDS_DOWNLOAD_BUBBLE_SHOW_IN_FOLDER_QUICK_ACTION_ACCESSIBILITY;
#undef IDS_DOWNLOAD_BUBBLE_SHOW_IN_FOLDER_QUICK_ACTION_ACCESSIBILITY
#define IDS_DOWNLOAD_BUBBLE_SHOW_IN_FOLDER_QUICK_ACTION_ACCESSIBILITY \
    kQuickActionAccessibilityResourceId,                              \
    info_->model()->GetFileNameToReportUser().LossyDisplayName());    \
  case DownloadCommands::DELETE_LOCAL_FILE:                           \
      return l10n_util::GetStringFUTF16(                              \
         IDS_DOWNLOAD_BUBBLE_DELETE_MAIN_BUTTON_ACCESSIBILITY

#include <chrome/browser/ui/views/download/bubble/download_bubble_row_view.cc>

#undef IDS_DOWNLOAD_BUBBLE_SHOW_IN_FOLDER_QUICK_ACTION_ACCESSIBILITY
#undef SetNotifyEnterExitOnChild
