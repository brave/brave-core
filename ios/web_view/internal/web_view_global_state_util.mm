// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web_view/internal/web_view_global_state_util.h"

namespace ios_web_view {

void InitializeGlobalState() {
  // We already provide global initialization with -[BraveCoreMain init] so this
  // is a stub of a required function.
  //
  // This function when implemented by the main `//ios/web_view` target would
  // typically setup the required Chromium web infrastructure required to
  // support CWVWebView: `WebMain`, `WebMainDelegate` and `WebClient`
  //
  // We already set this up in a similar fashion to Chrome iOS however so there
  // is no need to do it here.
  //
  // Note: This method is removed in M135 and replaced with a new type that
  // handles things similarily: `CWVGlobalState`. This new type will need a
  // chromium_src override to remove said initialization
}

}  // namespace ios_web_view
