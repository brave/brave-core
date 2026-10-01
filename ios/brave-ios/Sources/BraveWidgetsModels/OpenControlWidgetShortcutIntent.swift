// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import UIKit
import os

public enum ShortcutControlTrace {
  public static let log = Logger(subsystem: "com.brave.ios.shortcut-control", category: "trace")

  public static func describe(_ shortcut: WidgetShortcut) -> String {
    "\(String(describing: shortcut)) raw=\(shortcut.rawValue)"
  }
}

// WidgetShortcut is generated as an `@objc` enum, so a checked `Sendable` conformance
// cannot be added outside that file. The cases are a trivial `Int`.
extension WidgetShortcut: @unchecked Sendable {}

@available(iOS 26.0, *)
public struct OpenControlWidgetShortcutIntent: UISceneAppIntent {
  public static let title: LocalizedStringResource = "Open Brave"
  public static let openAppWhenRun: Bool = true

  /// Static controls deliver this stored value.
  public var shortcut: WidgetShortcut

  /// Configurable controls only keep `@Parameter` values. `WidgetShortcut` cannot be an `AppEnum`,
  /// so the raw value is stored beside `shortcut`.
  @Parameter(title: "Shortcut")
  public var shortcutRawValue: Int

  public init() {
    shortcut = .unknown
    shortcutRawValue = WidgetShortcut.unknown.rawValue
    ShortcutControlTrace.log.info("OpenControlWidgetShortcutIntent.init()")
  }

  public init(shortcut: WidgetShortcut) {
    self.shortcut = shortcut
    self.shortcutRawValue = shortcut.rawValue
    let message = "OpenControlWidgetShortcutIntent.init \(ShortcutControlTrace.describe(shortcut))"
    ShortcutControlTrace.log.info("\(message, privacy: .public)")
  }

  /// Prefer the stored shortcut. Configurable controls will leave it `.unknown`.
  public var resolvedShortcut: WidgetShortcut {
    if shortcut != .unknown {
      return shortcut
    }
    return WidgetShortcut(rawValue: shortcutRawValue) ?? .unknown
  }
}
