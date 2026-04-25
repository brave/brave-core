// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Shared
import UIKit

class RewardsButton: UIButton {

  enum IconState {
    case initial
    case enabled
    case disabled
  }

  var iconState: IconState = .initial {
    didSet {
      updateView()
    }
  }

  private let lookAtMeBadge = UIImageView(
    image: UIImage(named: "rewards-look-at-me", in: .module, compatibleWith: nil)!
  ).then {
    $0.isHidden = true
    $0.isUserInteractionEnabled = false
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    adjustsImageWhenHighlighted = false
    imageView?.contentMode = .scaleAspectFit
    contentHorizontalAlignment = .fill
    contentVerticalAlignment = .fill
    imageView?.adjustsImageSizeForAccessibilityContentSizeCategory = true

    accessibilityLabel = Strings.rewardsPanel
    accessibilityIdentifier = "urlBar-rewardsButton"

    addSubview(lookAtMeBadge)

    updateView()

    lookAtMeBadge.snp.makeConstraints {
      $0.centerY.equalTo(imageView!.snp.centerY).offset(-8)
      $0.leading.equalTo(imageView!.snp.centerX)
      $0.size.equalTo(16)
    }
  }

  private func updateView() {
    switch iconState {
    case .initial:
      setImage(UIImage(sharedNamed: "brave.basicattentiontoken")!, for: .normal)
      lookAtMeBadge.isHidden = false
    case .enabled:
      setImage(UIImage(sharedNamed: "brave.basicattentiontoken")!, for: .normal)
      lookAtMeBadge.isHidden = true
    case .disabled:
      setImage(UIImage(sharedNamed: "brave.basicattentiontoken.greyscale")!, for: .normal)
      lookAtMeBadge.isHidden = true
    }
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) { fatalError() }
}
