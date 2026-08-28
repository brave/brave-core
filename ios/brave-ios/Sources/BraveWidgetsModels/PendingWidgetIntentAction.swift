// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

/// An action requested from a Control Center / Action Button `AppIntent`.
///
/// Although Control Widget `AppIntent`s can foreground the app (`openAppWhenRun`),  there is no supported way to also deliver a custom-scheme URL alongside it
/// because`OpenURLIntent` and `URLRepresentableEnum`/`URLRepresentableIntent` all require a universal link.
/// Instead, the intent writes the requested shortcut to a mailbox (preferences), and the app consumes it once it becomes active.
public enum PendingWidgetIntentAction {
  private static let option = Preferences.Option<Int?>(
    key: "widgets.pendingIntentAction",
    default: nil
  )

  public static func set(_ shortcut: WidgetShortcut) {
    option.value = shortcut.rawValue
  }

  /// Returns and clears the pending shortcut, if any.
  public static func consume() -> WidgetShortcut? {
    defer { option.value = nil }
    return option.value.flatMap(WidgetShortcut.init(rawValue:))
  }
}
