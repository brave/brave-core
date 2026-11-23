// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/sync/tab_contents_synced_tab_delegate.h"

#include <optional>

namespace content {
class WebContents;
}  // namespace content

// Defined in //brave/browser/ui/sync:chromium_impl.
std::optional<bool> BraveShouldSyncTab(content::WebContents* web_contents);

#include <chrome/browser/ui/sync/tab_contents_synced_tab_delegate.cc>
