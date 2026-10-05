// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

/// The semantic identity of a toolbar arrangement, such as a single toolbar at the top of the
/// screen or toolbars split between the top and bottom of the screen.
///
/// A mode only describes which arrangement is displayed. The views that display it are provided
/// by a `BrowserToolbarLayout` registered for the mode in `BrowserToolbarLayoutRegistry`.
///
/// Code outside of the toolbar layer should never switch on the mode.
struct BrowserToolbarMode: Hashable, Sendable {
  var id: String
}

extension BrowserToolbarMode {
  /// A single toolbar at the top of the screen
  static let unifiedTop = Self(id: "UnifiedTop")
  /// A location bar at the top of the screen and action buttons at the bottom
  static let split = Self(id: "Split")
  /// A single toolbar at the bottom of the screen
  static let unifiedBottom = Self(id: "UnifiedBottom")
}

/// The inputs used to select the `BrowserToolbarMode` to display
struct BrowserToolbarModeContext {
  var traitCollection: UITraitCollection
  var userInterfaceIdiom: UIUserInterfaceIdiom
  var isUsingBottomBarPreference: Bool

  /// Whether the horizontal and vertical size classes are compact and regular respectively, such
  /// as an iPhone in portrait or an iPad in a compact width split view
  var isCompactWidthRegularHeight: Bool {
    traitCollection.horizontalSizeClass == .compact
      && traitCollection.verticalSizeClass == .regular
  }
}

/// A rule that selects a mode when the context matches
struct BrowserToolbarModeRule {
  var mode: BrowserToolbarMode
  /// Rules with a higher priority are evaluated first
  var priority: Int
  var matches: (BrowserToolbarModeContext) -> Bool
}

extension BrowserToolbarModeRule {
  /// The default rules.
  ///
  /// - `.unifiedBottom`: iPhone with the bottom bar preference enabled, compact width & regular
  ///   height
  /// - `.split`: compact width & regular height
  /// - `.unifiedTop`: everything else
  static let defaultRules: [BrowserToolbarModeRule] = [
    .init(
      mode: .unifiedBottom,
      priority: 200,
      matches: {
        $0.userInterfaceIdiom == .phone && $0.isUsingBottomBarPreference
          && $0.isCompactWidthRegularHeight
      }
    ),
    .init(mode: .split, priority: 100, matches: { $0.isCompactWidthRegularHeight }),
  ]

  /// Resolves the mode for a context. The first matching rule by priority wins, falling back to
  /// `.unifiedTop`.
  static func resolveMode(
    for context: BrowserToolbarModeContext,
    rules: [BrowserToolbarModeRule] = defaultRules
  ) -> BrowserToolbarMode {
    rules.sorted(by: { $0.priority > $1.priority }).first(where: { $0.matches(context) })?.mode
      ?? .unifiedTop
  }
}

/// Maps modes to the layouts that display them
///
/// A mode without a registered layout displays no toolbars.
@MainActor
struct BrowserToolbarLayoutRegistry {
  private var layouts: [BrowserToolbarMode: any BrowserToolbarLayout]

  init(layouts: [BrowserToolbarMode: any BrowserToolbarLayout]) {
    self.layouts = layouts
  }

  func layout(for mode: BrowserToolbarMode) -> (any BrowserToolbarLayout)? {
    layouts[mode]
  }

  /// The layouts currently ported to the toolbar layer
  static let `default` = BrowserToolbarLayoutRegistry(layouts: [
    .split: SplitToolbarLayout()
  ])
}
