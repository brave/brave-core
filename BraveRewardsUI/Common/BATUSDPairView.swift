/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

/// A view which pairs a BAT amount and USD amount
class BATUSDPairView: UIStackView {
  /// The BAT amount container
  let batContainer: CurrencyContainerView
  /// The USD amount container
  let usdContainer: CurrencyContainerView
  
  init(batAmountConfig: (UILabel) -> Void,
       batKindConfig: (UILabel) -> Void,
       usdConfig: (UILabel) -> Void) {
    self.batContainer = CurrencyContainerView(amountLabelConfig: batAmountConfig,
                                              kindLabelConfig: batKindConfig)
    self.usdContainer = CurrencyContainerView(uniformLabelConfig: usdConfig)
  
    super.init(frame: .zero)
    
    addArrangedSubview(batContainer)
    addArrangedSubview(usdContainer)
    
    // Defaults
    spacing = 5.0
    alignment = .bottom
    
    batContainer.kindLabel.text = "BAT"
    batContainer.amountLabel.text = "0"
    usdContainer.kindLabel.text = "USD"
    usdContainer.amountLabel.text = "0.00"
  }
  
  convenience init(amountFontSize: CGFloat = 15.0,
                   kindFontSize: CGFloat = 14.0,
                   usdFontSize: CGFloat = 13.0) {
    self.init(batAmountConfig: {
      $0.font = .systemFont(ofSize: amountFontSize, weight: .medium)
      $0.appearanceTextColor = Colors.grey100
    }, batKindConfig: {
      $0.font = .systemFont(ofSize: kindFontSize)
      $0.appearanceTextColor = Colors.grey100
    }, usdConfig: {
      $0.font = .systemFont(ofSize: usdFontSize)
      $0.appearanceTextColor = Colors.grey200
    })
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
