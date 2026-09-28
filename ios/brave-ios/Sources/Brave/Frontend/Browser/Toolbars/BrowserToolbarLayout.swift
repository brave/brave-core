// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

/// A toolbar layout that can be displayed by the browser, such as a single toolbar at the top of
/// the screen or toolbars split between the top and bottom of the screen.
///
/// Each layout builds its own independent view hierarchy and is responsible for any variations it
/// may have, such as OS version specific designs. Layouts are exposed as static members so that
/// they can be referenced in the same way as SwiftUI styles:
///
///     struct UnifiedTopToolbarLayout: BrowserToolbarLayout {
///       func makeController(
///         context: BrowserToolbarViewContext
///       ) -> any BrowserToolbarModeController {
///         if #available(iOS 26, *) {
///           return UnifiedTopToolbarControllerIOS26(context: context)
///         }
///         return UnifiedTopToolbarController(context: context)
///       }
///     }
///
///     extension BrowserToolbarLayout where Self == UnifiedTopToolbarLayout {
///       static var unifiedTop: Self { .init() }
///     }
///
/// Deciding which layout is displayed is the responsibility of `BrowserToolbarsViewController`.
/// Layouts and their controllers should not be concerned with the conditions they are displayed
/// in, and should read any traits they need to adapt to from their own view hierarchy.
///
/// Code outside of the toolbar layer should never branch on the active layout, instead layouts
/// publish any information needed by the browser through `BrowserToolbarModeController`.
@MainActor
protocol BrowserToolbarLayout {
  /// Creates the controller that displays this layout's toolbars
  func makeController(context: BrowserToolbarViewContext) -> any BrowserToolbarModeController
}
