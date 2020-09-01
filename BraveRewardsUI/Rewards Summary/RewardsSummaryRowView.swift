/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

//extension RewardsSummaryView {

  class RowView: UIView {
    private struct UX {
      static let titleColor = Colors.grey900
      static let cryptoCurrencyColor = Colors.grey700
      static let dollarValueColor = Colors.grey700
    }
    
    let stackView = UIStackView().then {
      $0.spacing = 10.0
      $0.alignment = .firstBaseline
    }
    
    let titleLabel = UILabel().then {
      $0.appearanceTextColor = UX.titleColor
      $0.font = .systemFont(ofSize: 15.0)
      $0.numberOfLines = 0
      $0.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
    }
    
    let cryptoValueLabel = UILabel().then {
      $0.font = .systemFont(ofSize: 14.0, weight: .semibold)
      $0.setContentHuggingPriority(.required, for: .horizontal)
    }
    
    let cryptoCurrencyLabel = UILabel().then {
      $0.appearanceTextColor = UX.cryptoCurrencyColor
      $0.font = .systemFont(ofSize: 12.0)
      $0.setContentHuggingPriority(.required, for: .horizontal)
    }
    
    let dollarValueLabel = UILabel().then {
      $0.appearanceTextColor = UX.dollarValueColor
      $0.font = .systemFont(ofSize: 10.0)
      $0.textAlignment = .right
      $0.setContentHuggingPriority(.required, for: .horizontal)
    }
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      let paddingGuide = UILayoutGuide()
      addLayoutGuide(paddingGuide)
      
      addSubview(stackView)
      stackView.addArrangedSubview(titleLabel)
      stackView.addArrangedSubview(cryptoValueLabel)
      stackView.setCustomSpacing(4.0, after: cryptoValueLabel)
      stackView.addArrangedSubview(cryptoCurrencyLabel)
//      stackView.addArrangedSubview(dollarValueLabel)
      
      paddingGuide.snp.makeConstraints {
        $0.top.bottom.equalTo(self).inset(12.0)
        $0.leading.trailing.equalTo(self)
      }
      stackView.snp.makeConstraints {
        $0.edges.equalTo(paddingGuide)
      }
//      dollarValueLabel.snp.makeConstraints {
//        $0.width.greaterThanOrEqualTo(60.0)
//      }
    }
    
    convenience init(title: String, cryptoValueColor: UIColor = .black,
                     batValue: String, usdDollarValue: String) {
      self.init(frame: .zero)
      titleLabel.text = title
      cryptoCurrencyLabel.text = Strings.BAT
      cryptoValueLabel.text = batValue
      cryptoValueLabel.appearanceTextColor = cryptoValueColor
//      dollarValueLabel.text = usdDollarValue
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
//}
