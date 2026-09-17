// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import UIKit

@available(iOS 26.0, *)
public struct OpenControlWidgetShortcutIntent: UISceneAppIntent {
  public static var title: LocalizedStringResource = "Open Brave"
  public static var openAppWhenRun: Bool = true

  public var shortcut: WidgetShortcut

  public init() {
    self.shortcut = .unknown
  }

  public init(shortcut: WidgetShortcut) {
    self.shortcut = shortcut
  }
}
