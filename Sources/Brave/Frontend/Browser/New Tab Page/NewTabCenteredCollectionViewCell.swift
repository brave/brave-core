// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import UIKit

/// A new tab collection view cell where the view is horizontally centered and themeable.
class NewTabCenteredCollectionViewCell<View: UIView>: UICollectionViewCell, CollectionViewReusable {
  /// The content view
  let view = View()

  override init(frame: CGRect) {
    super.init(frame: frame)
    contentView.addSubview(view)
    view.snp.remakeConstraints {
      $0.top.bottom.equalToSuperview()
      $0.centerX.equalToSuperview()
      $0.leading.greaterThanOrEqualToSuperview()
      $0.trailing.lessThanOrEqualToSuperview()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func preferredLayoutAttributesFitting(_ layoutAttributes: UICollectionViewLayoutAttributes) -> UICollectionViewLayoutAttributes {
    // swiftlint:disable:next force_cast
    let attributes = layoutAttributes.copy() as! UICollectionViewLayoutAttributes
    attributes.size.height = systemLayoutSizeFitting(layoutAttributes.size, withHorizontalFittingPriority: .required, verticalFittingPriority: .fittingSizeLevel).height
    return attributes
  }
}
