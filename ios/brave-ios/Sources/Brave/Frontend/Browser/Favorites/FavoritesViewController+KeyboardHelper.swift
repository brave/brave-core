// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Data
import Preferences
import Shared
import UIKit

// MARK: - KeyboardHelperDelegate

extension FavoritesViewController: KeyboardHelperDelegate {
  func updateKeyboardInset(_ state: KeyboardState, animated: Bool = true) {
    if collectionView.bounds.size == .zero { return }
    let keyboardHeight =
      state.intersectionHeightForView(self.view) - view.safeAreaInsets.bottom
      + additionalSafeAreaInsets.bottom
    UIViewPropertyAnimator(
      duration: animated ? state.animationDuration : 0.0,
      curve: state.animationCurve
    ) {
      self.collectionView.contentInset = self.collectionView.contentInset.with {
        $0.bottom = keyboardHeight
      }
      self.collectionView.scrollIndicatorInsets = self.collectionView.verticalScrollIndicatorInsets
        .with {
          $0.bottom = keyboardHeight
        }
    }.startAnimation()
  }

  func keyboardHelper(
    _ keyboardHelper: KeyboardHelper,
    keyboardWillShowWithState state: KeyboardState
  ) {
    updateKeyboardInset(state)
  }

  func keyboardHelper(
    _ keyboardHelper: KeyboardHelper,
    keyboardWillHideWithState state: KeyboardState
  ) {
    updateKeyboardInset(state)
  }
}
