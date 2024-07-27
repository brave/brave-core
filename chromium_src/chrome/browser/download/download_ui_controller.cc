/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/download/bubble/download_bubble_prefs.h"
#include "chrome/browser/download/download_stats.h"
#include "chrome/common/pref_names.h"

// Prevent DownloadBubbleUIControllerDelegate from overriding
// prefs::kPromptForDownload value for OffTheRecord profiles
#define kPromptForDownload kPromptForDownload,                  \
  profile_->GetPrefs()->GetBoolean(prefs::kPromptForDownload)); \
  DCHECK_EQ(true

#include "src/chrome/browser/download/download_ui_controller.cc"
#undef kPromptForDownload
