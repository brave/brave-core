// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

extension UIView {
  /// The insets to apply to the contents of a bar that is laid out against the top edge of this view
  /// with its background extending to the top edge of the screen.
  ///
  /// The returned bottom inset is always 0.
  ///
  /// These insets are derived from this view's current position on screen, so they should not be
  /// computed while the view is moved away from its resting position (such as while collapsing a
  /// toolbar), as that would alter its safe area and layout region insets.
  var topBarContentInsets: NSDirectionalEdgeInsets {
    if #available(iOS 27.1, *) {
      // The bar region places bars beside corner intrusions that the safe area spans the entire top
      // edge for (such as the status bar on an unfolded iPhone Duo), but also always reserves space
      // above the bar even when there is no top safe area (such as on iPhone in landscape), so only
      // use it when it places the bar closer to the top edge.
      let bar = directionalEdgeInsets(
        for: .bar(onEdge: UIRectEdge.top, extent: UIConstants.toolbarHeight)
      )
      if bar.top < safeAreaInsets.top {
        return .init(top: bar.top, leading: bar.leading, bottom: 0, trailing: bar.trailing)
      }
    }
    let safeArea: NSDirectionalEdgeInsets
    if #available(iOS 26.0, *), UIDevice.current.userInterfaceIdiom == .pad {
      safeArea = directionalEdgeInsets(for: .safeArea(cornerAdaptation: .horizontal))
    } else {
      let insets = safeAreaInsets
      let isRTL = effectiveUserInterfaceLayoutDirection == .rightToLeft
      safeArea = .init(
        top: insets.top,
        leading: isRTL ? insets.right : insets.left,
        bottom: insets.bottom,
        trailing: isRTL ? insets.left : insets.right
      )
    }
    guard safeArea.top.isZero else {
      // The contents are laid out below the top safe area, and therefore below the display's
      // corners, so corner adaptation doesn't apply
      return .init(
        top: safeArea.top,
        leading: safeArea.leading,
        bottom: 0,
        trailing: safeArea.trailing
      )
    }
    // The contents are laid out against the top edge so they must avoid the display's corners and
    // any intrusions within them
    return .init(top: 8, leading: safeArea.leading, bottom: 0, trailing: safeArea.trailing)
  }
}
