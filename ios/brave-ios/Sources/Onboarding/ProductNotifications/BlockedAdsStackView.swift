// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import DesignSystem
import Foundation
import Shared
import SnapKit
import UIKit

class BlockedAdsStackView: UIStackView {

  init(edgeInsets: UIEdgeInsets, spacing: CGFloat? = 0) {
    super.init(frame: .zero)
    if let spacing = spacing {
      self.spacing = spacing
    }

    alignment = .center
    layoutMargins = edgeInsets
    isLayoutMarginsRelativeArrangement = true
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  /// Adds Background to StackView with Color and Corner Radius
  public func addBackground(color: UIColor, cornerRadius: CGFloat? = nil) {
    let backgroundView = UIView(frame: bounds).then {
      $0.backgroundColor = color
      $0.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    }

    if let radius = cornerRadius {
      backgroundView.layer.cornerRadius = radius
      backgroundView.layer.cornerCurve = .continuous
    }

    insertSubview(backgroundView, at: 0)
  }
}
