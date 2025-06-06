/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_H_

#include "content/browser/renderer_host/render_widget_host_delegate.h"

#define ShouldDoLearning(...)                    \
  ShouldDoLearning_ChromiumImpl(__VA_ARGS__);    \
  bool GetShouldDoLearningForTesting() override; \
  bool ShouldDoLearning(__VA_ARGS__)

#include "src/content/browser/web_contents/web_contents_impl.h"  // IWYU pragma: export

#undef ShouldDoLearning

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_WEB_CONTENTS_WEB_CONTENTS_IMPL_H_
