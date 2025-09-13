/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_clear_browsing_data.h"
#include "chrome/browser/browsing_data/browsing_data_important_sites_util.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/browser/shell_integration_win.h"
#include "chrome/browser/win/jumplist.h"
#include "chrome/browser/win/jumplist_updater.h"

void BraveRemoveJumplist(Profile* profile) {
  if (!JumpList::Enabled() || !profile) {
    return;
  }
  auto app_id =
      shell_integration::win::GetAppUserModelIdForBrowser(profile->GetPath());
  JumpListUpdater::DeleteJumpList(app_id);
}

#define browsing_data_important_sites_util                             \
  if (remove_mask & chrome_browsing_data_remover::DATA_TYPE_HISTORY) { \
    BraveRemoveJumplist(profile_);                                     \
  }                                                                    \
  browsing_data_important_sites_util
#endif

#define HOSTED_APPS_DATA                                                    \
  BRAVE_AI_CHAT:                                                            \
  remove_mask |= chrome_browsing_data_remover::DATA_TYPE_BRAVE_LEO_HISTORY; \
  break;                                                                    \
  case BrowsingDataType::HOSTED_APPS_DATA

#define DATA_TYPE_CACHE                                                        \
  DATA_TYPE_CACHE;                                                             \
  content::BraveClearBrowsingData::UpdateMasksToClearCacheStorage(remove_mask, \
                                                                  origin_mask)

#include <chrome/browser/ui/webui/settings/settings_clear_browsing_data_handler.cc>

#undef HOSTED_APPS_DATA
#undef DATA_TYPE_CACHE

#if BUILDFLAG(IS_WIN)
#undef browsing_data_important_sites_util
#endif
