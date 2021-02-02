/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

// Replaced string here instead of by running 'npm run chromium_rebase_l10n'
// because string replacement failed with string that includes place holder like
// <ph name="PRODUCT_NAME">$1<ex>Brave</ex></ph>. I assume this includes some
// special characters.
#undef IDS_FIRSTRUN_DLG_MAC_OPTIONS_SEND_USAGE_STATS_LABEL
#define IDS_FIRSTRUN_DLG_MAC_OPTIONS_SEND_USAGE_STATS_LABEL \
  IDS_FIRSTRUN_DLG_MAC_OPTIONS_SEND_USAGE_STATS_LABEL_BRAVE
#include "../../../../../../chrome/browser/ui/cocoa/first_run_dialog_controller.mm"
#undef IDS_FIRSTRUN_DLG_MAC_OPTIONS_SEND_USAGE_STATS_LABEL
