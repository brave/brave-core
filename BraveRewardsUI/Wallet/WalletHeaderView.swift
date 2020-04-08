/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

enum UserWalletStatus {
  /// The user should not see anything user wallet related. This shows the old
  /// "Your wallet"/"Your balance" UI
  case hidden
  /// The user is not currently connected at all
  case notConnected
  /// The user has connected their wallet but they're not KYC'd on Uphold
  case connectedNotVerified
  /// The user is verified with Uphold
  case verified
  /// The users wallet has been disconnected
  case disconnected
}

class WalletHeaderView: UIView {
  
  private let backgroundImageView = UIImageView().then {
    $0.image = UIImage(frameworkResourceNamed: "header")
    $0.clipsToBounds = true
  }
  
  /// Contains the title or the user wallet buttons
  let titleContainerGuide = UILayoutGuide()
  
  let titleLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 16.0, weight: .medium)
    $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.65)
    $0.text = Strings.walletHeaderTitle
  }
  
  let connectUserWalletButton = ConnectUserWalletButton()
  let verifyUserWalletButton = VerifyUserWalletButton()
  let disconnectedUserWalletButton = DisconnectedUserWalletButton()
  let verifiedUserWalletButton = VerifiedUserWalletButton()
  
  private let altcurrencyContainerView = UIStackView().then {
    $0.spacing = 4.0
    $0.alignment = .lastBaseline
  }
  
  let balanceLabel = UILabel().then {
    $0.appearanceTextColor = .white
    $0.font = .systemFont(ofSize: 36.0)
  }
  
  let altcurrencyTypeLabel = UILabel().then {
    $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.65)
    $0.font = .systemFont(ofSize: 18.0)
  }
  
  let usdBalanceLabel = UILabel().then {
    $0.textAlignment = .center
    $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.65)
    $0.font = .systemFont(ofSize: 12.0)
  }
  
  let grantsButton = ActionButton(type: .system).then {
    $0.flipImageOrigin = true
    $0.titleLabel?.font = .systemFont(ofSize: 10.0, weight: .semibold)
    $0.setImage(UIImage(frameworkResourceNamed: "right-arrow-small").alwaysTemplate, for: .normal)
    $0.setTitle(Strings.walletHeaderGrants, for: .normal)
    $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.75)
    $0.titleEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 5.0)
    $0.contentEdgeInsets = UIEdgeInsets(top: 0, left: 10, bottom: 0, right: 10.0)
  }
  
  let addFundsButton = Button(type: .system).then {
    $0.appearanceTintColor = UIColor(white: 1.0, alpha: 0.75)
    $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.75)
    $0.setTitle(Strings.addFunds, for: .normal)
    $0.setImage(UIImage(frameworkResourceNamed: "add-funds-icon").alwaysTemplate, for: .normal)
    $0.imageEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 8)
    $0.titleEdgeInsets = UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 0)
    $0.contentEdgeInsets = UIEdgeInsets(top: 10, left: 5.0, bottom: 10, right: 5.0)
    $0.titleLabel?.font = .systemFont(ofSize: 14.0)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.isHidden = true
  }
  
  let settingsButton = Button(type: .system).then {
    $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.75)
    $0.setTitle(Strings.settings, for: .normal)
    $0.setImage(UIImage(frameworkResourceNamed: "bat-small").alwaysOriginal, for: .normal)
    $0.imageEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 8.0)
    $0.titleEdgeInsets = UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 0)
    $0.contentEdgeInsets = UIEdgeInsets(top: 10.0, left: 5.0, bottom: 10.0, right: 5.0)
    $0.titleLabel?.font = .systemFont(ofSize: 14.0)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }
  
  private let buttonsContainerView = UIStackView().then {
    $0.spacing = 20.0
    $0.alignment = .center
    $0.distribution = .fillEqually
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = .clear
    
    let stackView = UIStackView().then {
      $0.axis = .vertical
      $0.alignment = .center
      $0.spacing = 10.0
    }
    
    addSubview(backgroundImageView)
    addSubview(stackView)
    addLayoutGuide(titleContainerGuide)
    stackView.addArrangedSubview(altcurrencyContainerView)
    altcurrencyContainerView.addArrangedSubview(balanceLabel)
    altcurrencyContainerView.addArrangedSubview(altcurrencyTypeLabel)
    stackView.setCustomSpacing(4.0, after: altcurrencyContainerView)
    stackView.addArrangedSubview(usdBalanceLabel)
    stackView.setCustomSpacing(12.0, after: usdBalanceLabel)
    stackView.addArrangedSubview(grantsButton)
    stackView.addArrangedSubview(buttonsContainerView)
    buttonsContainerView.addArrangedSubview(addFundsButton)
    buttonsContainerView.addArrangedSubview(settingsButton)
    
    titleContainerGuide.snp.makeConstraints {
      $0.top.equalTo(self).inset(15.0 + PopoverArrowHeight)
      $0.leading.equalTo(self).inset(15.0)
      $0.trailing.lessThanOrEqualTo(self).inset(15)
    }
    stackView.snp.makeConstraints {
      $0.top.equalTo(titleContainerGuide.snp.bottom).offset(10.0)
      $0.leading.trailing.bottom.equalTo(self.layoutMarginsGuide).inset(15.0)
    }
    grantsButton.snp.makeConstraints {
      $0.height.equalTo(26.0)
    }
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    backgroundImageView.frame = bounds
  }
  
  private var activeWalletTitleView: UIView? {
    didSet {
      oldValue?.removeFromSuperview()
      if let activeWalletTitleView = activeWalletTitleView {
        addSubview(activeWalletTitleView)
        activeWalletTitleView.snp.makeConstraints {
          $0.edges.equalTo(titleContainerGuide)
        }
      }
    }
  }
}

extension WalletHeaderView {
  func setWalletBalance(_ value: String, crypto: String, dollarValue: String) {
    balanceLabel.text = value
    altcurrencyTypeLabel.text = crypto
    usdBalanceLabel.text = dollarValue
  }
  func setUserWalletStatus(_ status: UserWalletStatus) {
    switch status {
    case .hidden:
      activeWalletTitleView = titleLabel
    case .notConnected:
      activeWalletTitleView = connectUserWalletButton
    case .connectedNotVerified:
      activeWalletTitleView = verifyUserWalletButton
    case .verified:
      activeWalletTitleView = verifiedUserWalletButton
    case .disconnected:
      activeWalletTitleView = disconnectedUserWalletButton
    }
  }
}

