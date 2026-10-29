// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import UIKit

// WidgetShortcut is generated as an `@objc` enum, so a checked `Sendable` conformance
// cannot be added outside that file. The cases are a trivial `Int`.
extension WidgetShortcut: @unchecked Sendable {}

@available(iOS 26.0, *)
public struct OpenControlWidgetShortcutIntent: UISceneAppIntent {
  public static let title: LocalizedStringResource = "Open Brave"
  public static let openAppWhenRun: Bool = true

  /// Control Center restores this after `init()`. The stored `WidgetShortcut` does not come back.
  @Parameter(title: "Shortcut")
  public var shortcutValue: Int?

  public init() {}

  public init(shortcut: WidgetShortcut?) {
    self.shortcutValue = shortcut?.rawValue
  }

  public var shortcut: WidgetShortcut? {
    guard let shortcutValue else { return nil }
    return WidgetShortcut(rawValue: shortcutValue)
  }
}

/// Static Search control. Control Center restores an intent with `init()` and does not keep a stored shortcut, so the intent type is the shortcut.
@available(iOS 26.0, *)
public struct OpenBraveSearchControlIntent: UISceneAppIntent {
  public static let title: LocalizedStringResource = "Brave Search"
  public static let openAppWhenRun: Bool = true
  public static let isDiscoverable: Bool = false

  public init() {}
}

/// Static Ask Brave control. Separate from Search so a restored empty intent cannot open Search.
@available(iOS 26.0, *)
public struct OpenAskBraveControlIntent: UISceneAppIntent {
  public static let title: LocalizedStringResource = "Ask Brave"
  public static let openAppWhenRun: Bool = true
  public static let isDiscoverable: Bool = false

  public init() {}
}
