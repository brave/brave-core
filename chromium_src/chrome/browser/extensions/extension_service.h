/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_SERVICE_H_

#define BlocklistStatesInteractionUnitTest \
  BlocklistStatesInteractionUnitTest;      \
  friend class BraveExtensionService

#include "src/chrome/browser/extensions/extension_service.h"
#undef BlocklistStatesInteractionUnitTest

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_EXTENSION_SERVICE_H_
