// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

struct NavigateParams;

namespace content {
class WebContents;
}  // namespace content

// Forward declared to avoid adding a compile-time dependency.
// Impl target is //brave/browser/ui/navigator:chromium_impl.
void BraveAdjustLoadURLParams(NavigateParams* params,
                              content::WebContents* contents);

#include <chrome/browser/ui/navigator/browser_navigator_params_utils.cc>
