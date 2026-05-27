// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/download/download_bubble_info_utils.h"

#include <vector>

#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/download/download_commands.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// Appends a "Delete local file" quick action to `actions`. Invoked from
// QuickActionsForDownload via a plaster substitution, since DELETE_LOCAL_FILE
// is a Brave-only extension of DownloadCommands::Command and is shown only
// alongside the upstream OPEN_WHEN_COMPLETE quick action.
void AddDeleteLocalFileQuickAction(
    std::vector<DownloadBubbleQuickAction>& actions) {
  actions.emplace_back(DownloadCommands::DELETE_LOCAL_FILE,
                       l10n_util::GetStringUTF16(IDS_DOWNLOAD_BUBBLE_DELETE),
                       &kLeoTrashIcon);
}

}  // namespace

#include <chrome/browser/ui/download/download_bubble_info_utils.cc>
