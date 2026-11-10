// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AppIntents
import UIKit

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

  /// Read through `valueState` rather than `shortcutOption`. The wrapped value is not populated on
  /// the instance `scene(_:willPerformAppIntent:)` receives, but the resolved parameter is.
  public var shortcut: WidgetShortcut? {
    guard case .set(let option) = $shortcutOption.valueState else { return nil }
    return option?.widgetShortcut
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
