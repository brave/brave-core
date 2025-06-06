/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/web_contents/web_contents_impl.h"

namespace content {

bool WebContentsImpl::ShouldDoLearning() {
  if (!ShouldDoLearning_ChromiumImpl()) {
    return false;
  }
  return !GetContentClient()->browser()->IsWindowsRecallDisabled(
      GetBrowserContext());
}

bool WebContentsImpl::GetShouldDoLearningForTesting() {
  return ShouldDoLearning();
}

}  // namespace content

#define ShouldDoLearning(...) ShouldDoLearning_ChromiumImpl(__VA_ARGS__)

#include "src/content/browser/web_contents/web_contents_impl.cc"

#undef ShouldDoLearning
