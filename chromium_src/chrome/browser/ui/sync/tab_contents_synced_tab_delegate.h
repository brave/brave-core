// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SYNC_TAB_CONTENTS_SYNCED_TAB_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SYNC_TAB_CONTENTS_SYNCED_TAB_DELEGATE_H_

#include "components/sync_sessions/synced_tab_delegate.h"

#define ShouldSync(...)                 \
  ShouldSync_ChromiumImpl(__VA_ARGS__); \
  bool ShouldSync(__VA_ARGS__)

#include <chrome/browser/ui/sync/tab_contents_synced_tab_delegate.h>  // IWYU pragma: export

#undef ShouldSync

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SYNC_TAB_CONTENTS_SYNCED_TAB_DELEGATE_H_
