/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

/// A currency label container (i.e. "30.0 BAT", "10.0 USD") which are aligned next to each other by baseline, but
/// may or may not have the same styles/colors
final class CurrencyContainerView: UIStackView {
  /// The amount value label of the token (eg "30.0")
  let amountLabel: UILabel
  /// The amount kind label (eg "BAT"/"USD")
  let kindLabel: UILabel
  /// Create a currency container with pre-setup amount and token labels
  init(amountLabel: UILabel, kindLabel: UILabel) {
    self.amountLabel = amountLabel
    self.kindLabel = kindLabel
    
    super.init(frame: .zero)
    
    alignment = .firstBaseline
    spacing = 3.0
    
    isAccessibilityElement = true
    
    amountLabel.setContentCompressionResistancePriority(.required, for: .horizontal)
    kindLabel.setContentCompressionResistancePriority(.required, for: .horizontal)
    
    addArrangedSubview(amountLabel)
    addArrangedSubview(kindLabel)
  }
  /// Create a currency container which creates the labels. Config blocks will be used to setup appropriate styles
  convenience init(amountLabelConfig: (UILabel) -> Void, kindLabelConfig: (UILabel) -> Void) {
    self.init(amountLabel: UILabel().then(amountLabelConfig), kindLabel: UILabel().then(kindLabelConfig))
  }
  /// Create a currency container which creates uniformly styled labels.
  convenience init(uniformLabelConfig: (UILabel) -> Void) {
    self.init(amountLabel: UILabel().then(uniformLabelConfig), kindLabel: UILabel().then(uniformLabelConfig))
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  // MARK: - Accessibility
  
  override var accessibilityLabel: String? {
    get { return "\(amountLabel.text ?? "") \(kindLabel.text ?? "")" }
    set { } // swiftlint:disable:this unused_setter_value
  }
}
