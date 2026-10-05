// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

/// A location bar (and tabs bar) at the top of the screen, and the action buttons in a toolbar at
/// the bottom of the screen
struct SplitToolbarLayout: BrowserToolbarLayout {
  func makeController(context: BrowserToolbarViewContext) -> any BrowserToolbarModeController {
    SplitToolbarController(context: context)
  }
}

extension BrowserToolbarLayout where Self == SplitToolbarLayout {
  static var split: Self { .init() }
}
