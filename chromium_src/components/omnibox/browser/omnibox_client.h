/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_CLIENT_H_

#define FocusWebContents                             \
  OnInputAccepted(const AutocompleteMatch& match) {} \
  virtual void FocusWebContents
#include "src/components/omnibox/browser/omnibox_client.h"  // IWYU pragma: export
#undef FocusWebContents

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_OMNIBOX_BROWSER_OMNIBOX_CLIENT_H_
