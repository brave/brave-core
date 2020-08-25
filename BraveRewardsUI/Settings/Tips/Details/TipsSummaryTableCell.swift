/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class TipsSummaryTableCell: UITableViewCell, TableViewReusable {
  
  let batValueView = CurrencyContainerView(amountLabelConfig: {
    $0.appearanceTextColor = Colors.neutral700
    $0.font = .systemFont(ofSize: 14.0, weight: .semibold)
  }, kindLabelConfig: {
    $0.appearanceTextColor = Colors.neutral700
    $0.text = Strings.BAT
    $0.font = .systemFont(ofSize: 13.0)
  })
  
  let usdValueView = CurrencyContainerView(uniformLabelConfig: {
    $0.appearanceTextColor = SettingsUX.bodyTextColor
    $0.font = .systemFont(ofSize: 13.0)
  }).then {
    $0.kindLabel.text = "USD"
    $0.isHidden = true
  }
  
  let totalTipsThisMonthLabel = UILabel().then {
    $0.text = Strings.tipsTotalThisMonth
    $0.appearanceTextColor = .black
    $0.font = SettingsUX.bodyFont
    $0.numberOfLines = 0
    $0.setContentCompressionResistancePriority(.defaultHigh, for: .horizontal)
  }
  
  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: style, reuseIdentifier: reuseIdentifier)
    
    backgroundColor = .white
    
    let currenciesStackView = UIStackView().then {
      $0.spacing = 10.0
      $0.setContentHuggingPriority(.required, for: .horizontal)
      $0.addArrangedSubview(batValueView)
      $0.addArrangedSubview(usdValueView)
    }
    
    selectionStyle = .none
    
    contentView.addSubview(totalTipsThisMonthLabel)
    contentView.addSubview(currenciesStackView)
    
    totalTipsThisMonthLabel.snp.makeConstraints {
      $0.top.equalTo(contentView).offset(15)
      $0.leading.equalTo(contentView).offset(15.0)
      $0.trailing.lessThanOrEqualTo(currenciesStackView.snp.leading).offset(-10.0)
      $0.bottom.equalTo(contentView).inset(15)
    }
    currenciesStackView.snp.makeConstraints {
      $0.top.equalTo(totalTipsThisMonthLabel)
      $0.trailing.equalTo(contentView).inset(15.0)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
