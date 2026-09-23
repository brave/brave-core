/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/content/public/browser/navigation_entry_fullscreen.h"

#include <memory>

#include "base/supports_user_data.h"
#include "content/public/browser/navigation_entry.h"

namespace content {

namespace {

// SupportsUserData identifies keys by pointer address, not value. Keeping the
// key a single file-local definition guarantees one stable address that the
// browser-layer writer and the content-layer reader both resolve to, without
// exposing the key across the component boundary. Presence of the entry marks a
// pending request; its value is never read.
const int kFullscreenRequestedKey = 0;

}  // namespace

void SetNavigationEntryFullscreenRequested(NavigationEntry* entry,
                                           bool requested) {
  if (!entry) {
    return;
  }
  if (requested) {
    entry->SetUserData(&kFullscreenRequestedKey,
                       std::make_unique<base::SupportsUserData::Data>());
  } else {
    entry->RemoveUserData(&kFullscreenRequestedKey);
  }
}

bool IsNavigationEntryFullscreenRequested(const NavigationEntry* entry) {
  return entry && entry->GetUserData(&kFullscreenRequestedKey);
}

}  // namespace content
