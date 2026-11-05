// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// WidgetShortcut is generated as an `@objc` enum, so a checked `Sendable` conformance
// cannot be added outside that file. The cases are a trivial `Int`.
extension WidgetShortcut: @unchecked Sendable {}

/// Carries a shortcut from a performed control intent to whichever browser window can open it.
///
/// Parameters are only decoded while the intent is performed, so the value is read inside
/// `performNavigation(forScene:)` and handed off. Reading it from the intent delivered to
/// `scene(_:willPerformAppIntent:)` or `ConnectionOptions.appIntent` happens before the decode.
@available(iOS 26.0, *)
@MainActor
public final class ControlWidgetShortcutHandoff {
  public static let shared = ControlWidgetShortcutHandoff()

  /// Set once a window can navigate. Returns whether the shortcut was opened.
  public var handler: (@MainActor (WidgetShortcut) -> Bool)?
  private var pendingShortcut: WidgetShortcut?

  private init() {}

  /// Hands off from `performNavigation(forScene:)`, which is not main actor isolated.
  nonisolated public static func handOff(_ shortcut: WidgetShortcut) {
    Task { @MainActor in
      shared.deliver(shortcut)
    }
  }

  public func deliver(_ shortcut: WidgetShortcut) {
    if handler?(shortcut) == true { return }
    pendingShortcut = shortcut
  }

  /// Drains the shortcut held while no window was able to open it.
  public func takePendingShortcut() -> WidgetShortcut? {
    defer { pendingShortcut = nil }
    return pendingShortcut
  }
}
