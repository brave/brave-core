// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

class BraveTalkOptInSuccessViewController: UIViewController, PopoverContentComponent {

  private var braveTalkView: View {
    view as! View  // swiftlint:disable:this force_cast
  }

  override func loadView() {
    view = View()
    updatePreferredContentSize()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    if previousTraitCollection?.preferredContentSizeCategory
      != traitCollection.preferredContentSizeCategory {

      updatePreferredContentSize()
    }
  }

  private func updatePreferredContentSize() {
    let baseHeight: CGFloat = 250
    let scale = UIFontMetrics.default

    // For phones in portrait we leave extra space to dismiss the popup by tapping outside of it.
    if traitCollection.horizontalSizeClass == .compact && traitCollection.verticalSizeClass == .regular {
      let scaledHeight = scale.scaledValue(for: baseHeight)
      let height = min(scaledHeight, UIScreen.main.bounds.height - 150)

      preferredContentSize = .init(width: 350, height: height)
    } else {
      preferredContentSize = .init(width: 350, height: scale.scaledValue(for: baseHeight))
    }
  }
}
