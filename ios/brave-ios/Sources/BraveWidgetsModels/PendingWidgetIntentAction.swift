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
/// Instead, the intent writes the requested shortcut to a mailbox, and the app consumes it once it becomes active.
///
/// perform()` runs in the app rocess after `openAppWhenRun` foregrounds it, then hops here to `set`.
/// A leftover from an interrupted consume must not run the shortcut on a later launch.

@MainActor
public enum PendingWidgetIntentAction {
  private static var pending: WidgetShortcut?

  /// Drop any value persisted by an earlier build so it cannot run on this launch.
  private static let forgetPersistedMailbox: Void = {
    Preferences.Option<Int?>(
      key: "appIntent.control.pending.shortcut",
      default: nil
    ).reset()
  }()

  public static func set(_ shortcut: WidgetShortcut) {
    _ = forgetPersistedMailbox
    pending = shortcut
  }

  /// Returns and clears the pending shortcut for this process, if any.
  public static func consume() -> WidgetShortcut? {
    _ = forgetPersistedMailbox
    defer { pending = nil }
    return pending
  }
}
