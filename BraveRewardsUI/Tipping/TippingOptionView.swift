/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class TippingOptionView: UIControl {
  private struct UX {
    static let amountHeight: CGFloat = 40.0
    static let borderColor = UIColor(white: 1.0, alpha: 0.3)
    static let textColor = UIColor.white
    static let unselectedDollarTextColor = Colors.blurple200
    static let selectedDollarTextColor = UIColor.white
  }
  
  // Thing that holds the BAT amount + USD label
  let containerStackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 6.0
    $0.isUserInteractionEnabled = false
  }
  
  let amountView = UIView().then {
    $0.layer.cornerRadius = UX.amountHeight / 2.0
    $0.layer.borderColor = UX.borderColor.cgColor
    $0.layer.borderWidth = 1.0
  }
  
  let amountStackView = UIStackView().then {
    $0.spacing = 3.0
    $0.alignment = .center
  }
  
  let cryptoLogoImageView = UIImageView().then {
    $0.contentMode = .scaleAspectFit
    $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
  }
  
  let valueLabel = UILabel().then {
    $0.appearanceTextColor = UX.textColor
    $0.font = .systemFont(ofSize: 13.0, weight: .bold)
  }
  
  let cryptoLabel = UILabel().then {
    $0.appearanceTextColor = UX.textColor
    $0.font = .systemFont(ofSize: 12.0)
  }
  
  let dollarLabel = UILabel().then {
    $0.appearanceTextColor = UX.unselectedDollarTextColor
    $0.font = .systemFont(ofSize: 12.0)
    $0.textAlignment = .center
    $0.isHidden = true
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    addSubview(containerStackView)
    containerStackView.addArrangedSubview(amountView)
    containerStackView.addArrangedSubview(dollarLabel)
    
    amountView.addSubview(amountStackView)
    amountStackView.addArrangedSubview(cryptoLogoImageView)
    amountStackView.addArrangedSubview(valueLabel)
    amountStackView.addArrangedSubview(cryptoLabel)
    amountStackView.setCustomSpacing(6.0, after: self.cryptoLogoImageView)
    
    containerStackView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    amountView.snp.makeConstraints {
      $0.height.equalTo(UX.amountHeight)
    }
    amountStackView.snp.makeConstraints {
      $0.leading.greaterThanOrEqualTo(self.amountView).offset(10.0)
      $0.trailing.lessThanOrEqualTo(self.amountView).offset(-10.0)
      $0.center.equalTo(self.amountView)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  // MARK: -
  
  override var isSelected: Bool {
    didSet {
      amountView.layer.borderWidth = isSelected ? 0 : 1
      UIView.animate(withDuration: 0.4, delay: 0, usingSpringWithDamping: 1000, initialSpringVelocity: 0, options: [], animations: {
        self.amountView.backgroundColor = self.isSelected ? UX.borderColor : .clear
        self.dollarLabel.appearanceTextColor = self.isSelected ? UX.selectedDollarTextColor : UX.unselectedDollarTextColor
      }, completion: nil)
    }
  }
}
