// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import UIKit

class FavoritesCollectionViewCell: UICollectionViewCell, CollectionViewReusable {
  static let placeholderImage = UIImage(
    named: "defaultTopSiteIcon",
    in: .module,
    compatibleWith: nil
  )!

  let imageContainer = UIView().then {
    $0.layer.cornerRadius = 12.0
    $0.layer.borderWidth = 0.5
    $0.layer.borderColor = UIColor(braveSystemName: .dividerSubtle).cgColor
  }

  let imageView = LargeFaviconView()

  let textLabel = UILabel().then {
    $0.font = DynamicFontHelper.defaultHelper.defaultSmallFont
    $0.textAlignment = .center
    $0.lineBreakMode = NSLineBreakMode.byTruncatingTail
  }

  override var isHighlighted: Bool {
    didSet {
      UIView.animate(
        withDuration: 0.25,
        delay: 0,
        options: [.beginFromCurrentState],
        animations: {
          self.imageContainer.alpha = self.isHighlighted ? 0.7 : 1.0
        }
      )
    }
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    backgroundColor = .clear
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    isAccessibilityElement = true

    contentView.addSubview(imageContainer)
    contentView.addSubview(textLabel)

    imageContainer.snp.makeConstraints {
      $0.height.equalTo(imageContainer.snp.width)
      $0.top.equalToSuperview()
      $0.leading.trailing.equalToSuperview().inset(4.0)
    }

    textLabel.snp.makeConstraints {
      $0.top.equalTo(imageContainer.snp.bottom).offset(8.0)
      $0.leading.bottom.trailing.equalToSuperview()
    }

    imageContainer.addSubview(imageView)
    imageView.snp.makeConstraints {
      $0.height.equalTo(imageView.snp.width)
      $0.leading.trailing.equalToSuperview().inset(8)
      $0.center.equalTo(imageContainer.snp.center)
    }

    addInteraction(UIPointerInteraction(delegate: self))
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    NotificationCenter.default.removeObserver(self, name: .thumbnailEditOn, object: nil)
    NotificationCenter.default.removeObserver(self, name: .thumbnailEditOff, object: nil)
  }
}

extension FavoritesCollectionViewCell: UIPointerInteractionDelegate {
  func pointerInteraction(
    _ interaction: UIPointerInteraction,
    styleFor region: UIPointerRegion
  ) -> UIPointerStyle? {
    let preview = UITargetedPreview(view: imageContainer)
    return UIPointerStyle(effect: .lift(preview))
  }
}
