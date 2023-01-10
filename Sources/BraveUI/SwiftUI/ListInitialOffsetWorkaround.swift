// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import Introspect

private struct iOS16ListInitialOffsetFixViewModifier: ViewModifier {
  @State private var isFixApplied: Bool = false
  func body(content: Content) -> some View {
    content.introspect(
      selector: TargetViewSelector.ancestorOrSiblingContaining
    ) { (collectionView: UICollectionView) in
      if isFixApplied { return }
      defer { isFixApplied = true }
      collectionView.contentOffset = .init(x: 0, y: -collectionView.adjustedContentInset.top)
    }
  }
}

extension View {
  /// Starting in iOS 16.1 `List` and `Form` inside of navigation views have the chance to have the wrong
  /// initial content offset on the containing `UICollectionView`. This seems to happen mostly when sheets
  /// are involved.
  ///
  /// This workaround resets the initial content offset once on iOS 16.1 and later. On lower versions, this
  /// modifier does nothing.
  ///
  /// This bug is fixed in iOS 16.2
  @available(iOS, introduced: 14.0, deprecated: 16.2)
  @ViewBuilder public func listInitialOffsetWorkaround() -> some View {
#if swift(>=5.7.2)
    if #available(iOS 16.2, *) {
      modifier(EmptyModifier())
    } else if #available(iOS 16.1, *) {
      modifier(iOS16ListInitialOffsetFixViewModifier())
    } else {
      modifier(EmptyModifier())
    }
#elseif swift(>=5.7.1)
    if #available(iOS 16.1, *) {
      modifier(iOS16ListInitialOffsetFixViewModifier())
    } else {
      modifier(EmptyModifier())
    }
#else
    modifier(EmptyModifier())
#endif
  }
}
