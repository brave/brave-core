/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_H_

#define ClosePage(...)                         \
  GetShouldDoLearningForTesting(bool& should); \
  virtual void ClosePage(__VA_ARGS__)

#include "src/content/public/browser/web_contents.h"  // IWYU pragma: export

#undef ClosePage

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_H_
