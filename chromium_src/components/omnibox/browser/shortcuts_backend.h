/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_SHORTCUTS_BACKEND_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_SHORTCUTS_BACKEND_H_

#define InitInternal                       \
  InitInternalUnused();                    \
  friend class BraveShortcutsProviderTest; \
  void InitInternal

#include "src/components/omnibox/browser/shortcuts_backend.h"  // IWYU pragma: export

#undef InitInternal

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_SHORTCUTS_BACKEND_H_
