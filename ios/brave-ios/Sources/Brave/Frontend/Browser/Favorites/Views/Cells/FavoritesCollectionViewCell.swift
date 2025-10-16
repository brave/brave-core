// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import Shared
import UIKit

class FavoritesCollectionViewCell: UICollectionViewCell, CollectionViewReusable {

  let imageContainer = UIView()

  let imageView = LargeFaviconView()

  let textLabel = UILabel()

  var isPrivateBrowsing: Bool = false {
    didSet {
      setTheme()
    }
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

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)

    setTheme()
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    isAccessibilityElement = true

    setTheme()
    doLayout()

    addInteraction(UIPointerInteraction(delegate: self))
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setTheme() {
    imageContainer.do {
      $0.layer.cornerRadius = 12.0
      $0.layer.borderWidth = isPrivateBrowsing ? 0 : 0.5
      $0.layer.cornerCurve = .continuous
      $0.layer.borderColor =
        isPrivateBrowsing ? nil : UIColor(braveSystemName: .dividerSubtle).cgColor
      $0.backgroundColor = isPrivateBrowsing ? UIColor(braveSystemName: .containerBackground) : nil
    }

    textLabel.do {
      $0.textAlignment = .center
      $0.numberOfLines = 1

      let clampedTraitCollection = self.traitCollection.clampingSizeCategory(
        maximum: .accessibilityExtraLarge
      )
      let font = UIFont.preferredFont(
        forTextStyle: .caption2,
        compatibleWith: clampedTraitCollection
      )
      $0.font = font
      $0.lineBreakMode = NSLineBreakMode.byTruncatingTail
      $0.textColor = isPrivateBrowsing ? .white : UIColor(braveSystemName: .textSecondary)
    }
  }

  private func doLayout() {
    contentView.addSubview(imageContainer)
    contentView.addSubview(textLabel)

    imageContainer.snp.makeConstraints {
      $0.height.equalTo(imageContainer.snp.width)
      $0.top.equalToSuperview().inset(8)
      $0.leading.trailing.equalToSuperview().inset(4.0)
    }

    imageContainer.addSubview(imageView)
    imageView.snp.makeConstraints {
      $0.edges.equalToSuperview().inset(8)
    }

    textLabel.snp.makeConstraints {
      $0.top.equalTo(imageContainer.snp.bottom).offset(8.0)
      $0.leading.trailing.equalToSuperview()
      $0.bottom.equalToSuperview().inset(8)
    }
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
