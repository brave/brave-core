// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

extension BrowserViewController: SnackBarTabHelperDelegate {
  func tab(_ tab: Tab, didAddSnackbar bar: SnackBar) {
    showBar(bar, animated: true)
  }

  func tab(_ tab: Tab, didRemoveSnackbar bar: SnackBar) {
    removeBar(bar, animated: true)
  }

  func showBar(_ bar: SnackBar, animated: Bool) {
    view.layoutIfNeeded()
    UIView.animate(
      withDuration: animated ? 0.25 : 0,
      animations: {
        self.alertStackView.insertArrangedSubview(bar, at: 0)
        self.view.layoutIfNeeded()
      }
    )
  }

  func removeBar(_ bar: SnackBar, animated: Bool) {
    UIView.animate(
      withDuration: animated ? 0.25 : 0,
      animations: {
        bar.removeFromSuperview()
      }
    )
  }

  func removeAllBars() {
    alertStackView.arrangedSubviews.forEach { $0.removeFromSuperview() }
  }
}
