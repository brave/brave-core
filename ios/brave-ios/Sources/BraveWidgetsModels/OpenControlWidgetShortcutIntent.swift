// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import UIKit
import os.log

@available(iOS 26.0, *)
public struct OpenControlWidgetShortcutIntent: UISceneAppIntent {
  public static let title: LocalizedStringResource = "Open Brave"
  public static let openAppWhenRun: Bool = true

  @Parameter(title: "Shortcut")
  public var shortcutOption: ShortcutControlOption?

  public init() {}

  public init(shortcut: ShortcutControlOption?) {
    self.shortcutOption = shortcut
  }

  public var shortcut: WidgetShortcut? {
    guard let shortcutOption else { return nil }
    return shortcutOption.widgetShortcut
  }

  public func performNavigation(forScene scene: UIScene) {
    Logger.module.debug(
      "[CONTROL PROBE] performNavigation valueState=\(String(describing: $shortcutOption.valueState))"
    )
    guard let shortcut else { return }
    ControlWidgetShortcutHandoff.handOff(shortcut)
  }
}

/// Static Search control. Control Center restores an intent with `init()`, so the intent type is the shortcut.
@available(iOS 26.0, *)
public struct OpenBraveSearchControlIntent: UISceneAppIntent {
  public static let title: LocalizedStringResource = "Brave Search"
  public static let openAppWhenRun: Bool = true
  public static let isDiscoverable: Bool = false

  public init() {}

  public func performNavigation(forScene scene: UIScene) {
    ControlWidgetShortcutHandoff.handOff(.search)
  }
}

/// Static Ask Brave control. Separate from Search so a restored empty intent cannot open Search.
@available(iOS 26.0, *)
public struct OpenAskBraveControlIntent: UISceneAppIntent {
  public static let title: LocalizedStringResource = "Ask Brave"
  public static let openAppWhenRun: Bool = true
  public static let isDiscoverable: Bool = false

  public init() {}

  public func performNavigation(forScene scene: UIScene) {
    ControlWidgetShortcutHandoff.handOff(.askBrave)
  }
}
