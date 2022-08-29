// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Storage
import BraveUI
import SnapKit
import UIKit

/// Defines the basic feed card cell. A feed card can display 1 or more feed
/// items. This cell is defined by the `View` type
public class FeedCardCell<Content: FeedCardContent>: UICollectionViewCell, CollectionViewReusable {
  public var content = Content()
  private var widthConstraint: Constraint?

  public override init(frame: CGRect) {
    super.init(frame: frame)

    contentView.addSubview(content.view)
    content.view.snp.makeConstraints {
      $0.top.bottom.equalToSuperview()
      widthConstraint = $0.width.equalTo(375).constraint
      $0.centerX.equalToSuperview()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  public override func preferredLayoutAttributesFitting(_ layoutAttributes: UICollectionViewLayoutAttributes) -> UICollectionViewLayoutAttributes {
    // swiftlint:disable:next force_cast
    let attributes = layoutAttributes.copy() as! UICollectionViewLayoutAttributes
    // Let iPads have a bit larger cards since theres more room
    if traitCollection.horizontalSizeClass == .regular {
      widthConstraint?.update(offset: min(600, attributes.size.width))
    } else {
      widthConstraint?.update(offset: min(400, attributes.size.width))
    }
    attributes.size.height =
      systemLayoutSizeFitting(
        UIView.layoutFittingCompressedSize,
        withHorizontalFittingPriority: .required,
        verticalFittingPriority: .fittingSizeLevel
      ).height
    return attributes
  }
}
