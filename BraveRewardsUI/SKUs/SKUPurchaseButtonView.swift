// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveUI

/// Container for the "Buy with BAT" and T&C's buttons
class SKUPurchaseButtonView: UIView {
  
  var backgroundVisible: Bool = false {
    didSet {
      UIView.animate(withDuration: 0.25, delay: 0, options: [.beginFromCurrentState], animations: {
        self.layer.shadowOpacity = self.backgroundVisible ? 0.3 : 0.0
        self.backgroundView.alpha = self.backgroundVisible ? 1.0 : 0.0
      })
    }
  }
  
  let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .light)).then {
    $0.alpha = 0.0
  }
  
  let buyButton = FilledActionButton(type: .system).then {
    $0.appearanceBackgroundColor = Colors.blurple500
    $0.setTitle(String(format: Strings.SKUPurchaseBuyWithBAT, Strings.BAT), for: .normal)
    $0.titleLabel?.font = .systemFont(ofSize: 16.0, weight: .semibold)
  }
  
  let disclaimerLabel = LinkLabel().then {
    let buyWithBat = String(format: Strings.SKUPurchaseBuyWithBAT, Strings.BAT)
    $0.linkColor = BraveUX.braveOrange
    $0.text = String(format: Strings.SKUPurchaseDisclaimerInformation, buyWithBat, Strings.SKUPurchaseDisclaimerInformationLink)
    $0.appearanceTextColor = Colors.grey900
    $0.font = .systemFont(ofSize: 12.0)
    $0.textAlignment = .center
    $0.updateText(urlInfo: [Strings.SKUPurchaseDisclaimerInformationLink: "terms-of-sale"])
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    layer.shadowOffset = CGSize(width: 0, height: -1)
    layer.shadowOpacity = 0
    layer.shadowRadius = 3
    
    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.alignment = .center
      $0.spacing = 12
    }
    
    addSubview(backgroundView)
    addSubview(stackView)
    stackView.addArrangedSubview(buyButton)
    stackView.addArrangedSubview(disclaimerLabel)
    
    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    stackView.snp.makeConstraints {
      $0.edges.equalTo(safeAreaLayoutGuide).inset(16)
    }
    buyButton.snp.makeConstraints {
      $0.height.equalTo(44)
      $0.width.equalTo(stackView)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
