/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/browsing_data/core/browsing_data_utils.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/browser/shell_integration_win.h"
#include "chrome/browser/win/jumplist.h"
#include "chrome/browser/win/jumplist_updater.h"

namespace {
void BraveRemoveJumplist(uint64_t remove_mask, Profile* profile) {
  if (!(remove_mask & chrome_browsing_data_remover::DATA_TYPE_HISTORY) ||
      !profile || !JumpList::Enabled()) {
    return;
  }
  auto app_id =
      shell_integration::win::GetAppUserModelIdForBrowser(profile->GetPath());
  JumpListUpdater::DeleteJumpList(app_id);
}
}  // namespace
#endif  // #if BUILDFLAG(IS_WIN)

#include <chrome/browser/ui/webui/settings/settings_clear_browsing_data_handler.cc>
