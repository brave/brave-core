/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/whats_new/whats_new_fetcher.h"

#include "brave/browser/ui/whats_new/whats_new_util.h"

#define StartWhatsNewFetch StartWhatsNewFetch_UnUsed
#include "src/chrome/browser/ui/webui/whats_new/whats_new_fetcher.cc"
#undef StartWhatsNewFetch

namespace whats_new {

void StartWhatsNewFetch(Browser* browser) {
  StartBraveWhatsNew(browser);
}

}  // namespace whats_new
