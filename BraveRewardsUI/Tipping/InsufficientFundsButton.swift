/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

/// Displayed when the user does not have enough tokens to complete a tip
class InsufficientFundsButton: UIControl {
  
  private struct UX {
    static let backgroundColor = Colors.blurple900
    static let imageTintColor = Colors.grey600
  }
  
  private let stackView = UIStackView().then {
    $0.spacing = 15.0
    $0.isUserInteractionEnabled = false
  }
  
  private let imageView = UIImageView(image: UIImage(frameworkResourceNamed: "icn-frowning-face").alwaysTemplate).then {
    $0.tintColor = UX.imageTintColor
  }
  
  private let textLabel = UILabel().then {
    $0.appearanceTextColor = .white
    $0.font = .systemFont(ofSize: 13.0)
    $0.numberOfLines = 0
    $0.text = Strings.tippingNotEnoughTokens
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = UX.backgroundColor
    
    addSubview(stackView)
    stackView.addArrangedSubview(imageView)
    stackView.addArrangedSubview(textLabel)
    
    let contentGuide = UILayoutGuide()
    addLayoutGuide(contentGuide)
    
    contentGuide.snp.makeConstraints {
      $0.top.equalTo(self)
      $0.leading.trailing.equalTo(self).inset(15.0)
      $0.bottom.equalTo(self.safeAreaLayoutGuide)
      $0.height.equalTo(56.0)
    }
    stackView.snp.makeConstraints {
      $0.center.equalTo(contentGuide)
      $0.leading.greaterThanOrEqualTo(contentGuide)
      $0.trailing.lessThanOrEqualTo(contentGuide)
    }
    imageView.snp.makeConstraints {
      $0.size.equalTo(self.imageView.image!.size)
    }
  }
}
