/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_VERIFIED_CONTENTS_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_VERIFIED_CONTENTS_H_

#define Create(...)                 \
  Create_ChromiumImpl(__VA_ARGS__); \
  static std::unique_ptr<VerifiedContents> Create(__VA_ARGS__)

#include "src/extensions/browser/verified_contents.h"  // IWYU pragma: export

#undef Create

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_VERIFIED_CONTENTS_H_
