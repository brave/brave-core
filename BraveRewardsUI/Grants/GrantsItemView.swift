/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class GrantsItemView: SettingsSectionView {
  
  init(amount: String, expirationDate: Date?) {
    super.init(frame: .zero)
  
    let dateFormatter = DateFormatter().then {
      $0.dateStyle = .short
      $0.timeStyle = .none
    }
    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.alignment = .leading
      $0.spacing = 5.0
    }
    let amountView = CurrencyContainerView(amountLabelConfig: {
      $0.appearanceTextColor = Colors.grey800
      $0.font = .systemFont(ofSize: 16.0, weight: .medium)
      $0.text = amount
    }, kindLabelConfig: {
      $0.appearanceTextColor = Colors.grey700
      $0.font = .systemFont(ofSize: 13.0)
      $0.text = Strings.BAT
    })
    
    addSubview(stackView)
    stackView.addArrangedSubview(amountView)
    
    if let date = expirationDate {
      let expirationLabel = UILabel().then {
        $0.appearanceTextColor = Colors.grey600
        $0.font = .systemFont(ofSize: 14.0)
        $0.numberOfLines = 0
        $0.text = String(
          format: Strings.grantListExpiresOn,
          dateFormatter.string(from: date)
        )
      }
      stackView.addArrangedSubview(expirationLabel)
    }
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(layoutMarginsGuide)
    }
  }
}
