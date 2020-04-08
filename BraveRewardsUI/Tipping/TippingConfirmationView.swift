/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class TippingConfirmationView: UIView {
  struct UX {
    static let backgroundColor = Colors.blurple900.withAlphaComponent(0.95)
    static let faviconBackgroundColor = Colors.neutral100
    static let faviconSize = CGSize(width: 92.0, height: 92.0)
    static let confirmationTextColor = Colors.grey300
  }
  
  let stackView = UIStackView().then {
    $0.alignment = .center
    $0.axis = .vertical
    $0.spacing = 20.0
  }
  
  let faviconImageView = UIImageView().then {
    $0.backgroundColor = UX.faviconBackgroundColor
    $0.clipsToBounds = true
    $0.layer.cornerRadius = UX.faviconSize.width / 2.0
    $0.layer.borderColor = UIColor.white.cgColor
    $0.layer.borderWidth = 2.0
  }
  
  let titleLabel = UILabel().then {
    $0.text = Strings.tippingConfirmation
    $0.appearanceTextColor = UX.confirmationTextColor
    $0.font = .systemFont(ofSize: 28.0, weight: .bold)
    $0.textAlignment = .center
    $0.numberOfLines = 0
  }
  
  let subtitleLabel = UILabel().then {
    $0.appearanceTextColor = UX.confirmationTextColor
    $0.font = .systemFont(ofSize: 14.0, weight: .medium)
    $0.textAlignment = .center
    $0.numberOfLines = 0
  }
  
  let infoLabel = UILabel().then {
    $0.appearanceTextColor = UX.confirmationTextColor
    $0.font = .systemFont(ofSize: 20.0, weight: .medium)
    $0.textAlignment = .center
    $0.numberOfLines = 0
  }
  
  let monthlyTipLabel = UILabel().then {
    $0.appearanceTextColor = UX.confirmationTextColor
    $0.font = .systemFont(ofSize: 14.0, weight: .medium)
    $0.textAlignment = .center
    $0.numberOfLines = 0
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = UX.backgroundColor
    
    addSubview(stackView)
    stackView.addArrangedSubview(faviconImageView)
    stackView.addArrangedSubview(titleLabel)
    stackView.addArrangedSubview(subtitleLabel)
    stackView.addArrangedSubview(infoLabel)
    stackView.addArrangedSubview(monthlyTipLabel)
    
    stackView.setCustomSpacing(4.0, after: titleLabel)
    stackView.setCustomSpacing(25.0, after: infoLabel)
    
    stackView.snp.makeConstraints {
      $0.leading.trailing.equalTo(self).inset(20.0)
      $0.centerY.equalTo(self)
      $0.top.greaterThanOrEqualTo(self).offset(20.0)
      $0.bottom.lessThanOrEqualTo(self).offset(-20.0)
    }
    
    faviconImageView.snp.makeConstraints {
      $0.size.equalTo(UX.faviconSize)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
