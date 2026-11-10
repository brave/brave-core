// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import UIKit

/// Implemented by the scene delegate so a control intent can open its shortcut in that scene's
/// browser. The widget extension links this module but cannot link the browser, so the intent
/// reaches it through this protocol rather than depending on it directly.
@available(iOS 26.0, *)
@MainActor
public protocol ControlWidgetShortcutOpening: AnyObject {
  func openControlWidgetShortcut(_ shortcut: WidgetShortcut)
}

@available(iOS 26.0, *)
public struct OpenControlWidgetShortcutIntent: UISceneAppIntent {
  public static let title: LocalizedStringResource = "Open Brave"
  public static let openAppWhenRun: Bool = true

  @Parameter(title: "Shortcut")
  public var shortcutOption: ShortcutOption?

  public init() {}

  public init(shortcut: ShortcutOption?) {
    self.shortcutOption = shortcut
  }

  /// `.unset` is a parameter that has not resolved, which is distinct from one resolved to `nil`.
  public var shortcut: WidgetShortcut? {
    guard case .set(let option) = $shortcutOption.valueState else { return nil }
    return option?.widgetShortcut
  }

  /// The system delivers the intent to a scene and resolves parameters before running `perform()`,
  /// so both `shortcut` and `uiScene` are available here.
  @MainActor
  public func perform() async throws -> some IntentResult {
    if let shortcut, let opener = uiScene?.delegate as? ControlWidgetShortcutOpening {
      opener.openControlWidgetShortcut(shortcut)
    }
    return .result()
  }
}

/// Static Search control. Control Center restores an intent with `init()`, so the intent type is the shortcut.
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
