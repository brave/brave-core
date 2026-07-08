/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_FULLSCREEN_H_
#define BRAVE_CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_FULLSCREEN_H_

#include "content/common/content_export.h"

namespace content {

class NavigationEntry;

// Tracks, per NavigationEntry, whether a Brave-managed fullscreen request is in
// flight for the page the user acted on. It is set in the browser layer (the
// YouTube PiP tab helper) and read in the content layer
// (BraveScreenOrientationDelegateAndroid) to suppress the screen-orientation
// lock/unlock that would otherwise flash the phone into landscape while entering
// or leaving Picture-in-Picture.
//
// The state is exposed as free functions rather than a shared UserData key on
// purpose: the key stays a single file-local definition, so it has one address
// and cannot get a distinct per-component address that silently breaks the
// cross-layer lookup (the failure mode documented at crbug.com/589840). This
// mirrors how content/public exposes other cross-layer predicates (e.g.
// IsIsolatedContext) and how internal DocumentUserData accessors hide their key.
CONTENT_EXPORT void SetNavigationEntryFullscreenRequested(NavigationEntry* entry,
                                                          bool requested);
CONTENT_EXPORT bool IsNavigationEntryFullscreenRequested(
    const NavigationEntry* entry);

}  // namespace content

#endif  // BRAVE_CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_FULLSCREEN_H_
