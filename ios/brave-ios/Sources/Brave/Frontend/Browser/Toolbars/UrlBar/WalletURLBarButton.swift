// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

class WalletURLBarButton: UIButton {

  enum ButtonState {
    case inactive
    case active
    case activeWithPendingRequest
  }

  var buttonState: ButtonState = .inactive {
    didSet {
      // We may end up having different states here where active is actually blurple
      tintColor = UIColor(braveSystemName: .textPrimary)

      if buttonState == .activeWithPendingRequest {
        addBadgeIfNeeded()
      }
      badgeView.isHidden = buttonState != .activeWithPendingRequest
    }
  }

  private let badgeView = UIView()
  private let badgeSize = 10.0

  override init(frame: CGRect) {
    super.init(frame: frame)

    var configuration = UIButton.Configuration.plain()
    configuration.baseForegroundColor = UIColor(braveSystemName: .textPrimary)
    configuration.baseBackgroundColor = .clear
    configuration.contentInsets = .init(top: 3, leading: 3, bottom: 3, trailing: 3)
    configuration.image = UIImage(braveSystemNamed: "leo.product.brave-wallet")
    configuration.imageColorTransformer = .init { _ in
      if self.isHighlighted {
        return UIColor(braveSystemName: .textInteractive)
      }
      return UIColor(braveSystemName: .textPrimary)
    }
    self.configuration = configuration

    updateIconSize()

    registerForTraitChanges([UITraitPreferredContentSizeCategory.self]) { (self: Self, _) in
      self.updateIconSize()
    }
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  func addBadgeIfNeeded() {
    guard badgeView.superview == nil else { return }

    badgeView.backgroundColor = UIColor(braveSystemName: .systemfeedbackErrorIcon)
    badgeView.layer.cornerRadius = badgeView.frame.height / 2
    badgeView.layer.masksToBounds = true
    addSubview(badgeView)

    if let imageView = imageView {
      badgeView.snp.makeConstraints { make in
        make.size.equalTo(badgeSize)
        make.centerX.equalTo(imageView.snp.trailing).inset(badgeSize / 4)
        make.centerY.equalTo(imageView.snp.top).inset(badgeSize / 4)
      }
    }
  }

  private func updateIconSize() {
    var configuration = self.configuration
    let sizeCategory = traitCollection.toolbarButtonContentSizeCategory
    // Inset the icon by 3 points to match the size of the other URL bar icons
    let pointSize =
      UIFont.preferredFont(
        forTextStyle: .body,
        compatibleWith: .init(preferredContentSizeCategory: sizeCategory)
      ).pointSize - 3
    configuration?.preferredSymbolConfigurationForImage =
      .init(pointSize: pointSize, weight: .regular, scale: .large)
    self.configuration = configuration
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    badgeView.layer.cornerRadius = badgeView.frame.height / 2
  }
}
