/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveUI

class TippingOption: NSObject {
  var value: BATValue
  var crypto: String
  var cryptoImage: UIImage
  var dollarValue: String
  
  fileprivate var view: TippingOptionView?
  
  init(value: BATValue, crypto: String, cryptoImage: UIImage, dollarValue: String) {
    self.value = value
    self.crypto = crypto
    self.cryptoImage = cryptoImage
    self.dollarValue = dollarValue
    
    super.init()
  }
  
  class func batAmount(_ value: BATValue, dollarValue: String) -> TippingOption {
    return TippingOption(
      value: value,
      crypto: Strings.BAT,
      cryptoImage: UIImage(frameworkResourceNamed: "bat-small"),
      dollarValue: dollarValue
    )
  }
}

class TippingSelectionView: UIView {
  
  func setEnoughFundsAvailable(_ enoughFunds: Bool) {
    sendTipButton.isHidden = !enoughFunds
    insufficientFundsButton.isHidden = enoughFunds
  }
  
  lazy var sendTipButton: SendTipButton = {
    let button = SendTipButton()
    button.isEnabled = selectedOptionIndex != nil
    return button
  }()
  
  let insufficientFundsButton = InsufficientFundsButton().then {
    $0.isHidden = true
  }
  
  // MARK: - Wallet Balance
  
  func setWalletBalance(_ value: String, crypto: String) {
    walletBalanceView.valueLabel.text = value
    walletBalanceView.cryptoLabel.text = crypto
  }
  
  // MARK: - Options
  
  var isMonthly: Bool = false {
    didSet {
      monthlyToggleButton.setImage(UIImage(frameworkResourceNamed: isMonthly ? "checkbox-checked" : "checkbox").alwaysOriginal, for: .normal)
      sendTipButton.isMonthly = isMonthly
    }
  }
  
  var selectedOptionIndex: Int? {
    get {
      return options.firstIndex(where: { $0.view?.isSelected == true })
    }
    set {
      guard let selectedOption = newValue else { return }
      
      if selectedOption < options.count {
        options[selectedOption].view?.isSelected = true
        sendTipButton.isEnabled = true
      }
    }
  }
  
  var optionChanged: ((TippingOption) -> Void)?
  
  var options: [TippingOption] = [] {
    willSet {
      options.forEach { $0.view?.removeFromSuperview() }
    }
    didSet {
      for option in options {
        let view = TippingOptionView()
        view.cryptoLabel.text = option.crypto
        view.cryptoLogoImageView.image = option.cryptoImage
        view.valueLabel.text = option.value.displayString
        view.dollarLabel.text = option.dollarValue
        option.view = view
        
        view.addTarget(self, action: #selector(tappedOption(_:)), for: .touchUpInside)
        optionsStackView.addArrangedSubview(view)
      }
    }
  }
  
  // MARK: - Private UI
  
  private let titleLabel = UILabel().then {
    $0.appearanceTextColor = .white
    $0.font = .systemFont(ofSize: 18.0, weight: .bold)
    $0.text = Strings.tippingAmountTitle
  }
  
  private let walletBalanceView = WalletBalanceView()
  
  private let optionsStackView = UIStackView().then {
    $0.spacing = 10.0
    $0.distribution = .fillEqually
  }
  
  private let monthlyToggleButton = Button(type: .system).then {
    $0.setTitle(Strings.tippingMakeMonthly, for: .normal)
    $0.setImage(UIImage(frameworkResourceNamed: "checkbox"), for: .normal)
    $0.imageEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 10)
    $0.titleEdgeInsets = UIEdgeInsets(top: 0, left: 10, bottom: 0, right: 0)
    $0.tintColor = .white
  }
  
  private let layoutGuide = UILayoutGuide()
  
  // MARK: -
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    backgroundColor = Colors.blurple400
    
    monthlyToggleButton.addTarget(self, action: #selector(tappedMonthlyToggle), for: .touchUpInside)
    
    addSubview(titleLabel)
    addSubview(walletBalanceView)
    addSubview(optionsStackView)
    addSubview(monthlyToggleButton)
    addSubview(sendTipButton)
    addSubview(insufficientFundsButton)
    addLayoutGuide(layoutGuide)
    
    titleLabel.snp.makeConstraints {
      $0.top.equalTo(self).offset(20.0)
      $0.leading.equalTo(layoutGuide).offset(15.0)
      $0.trailing.lessThanOrEqualTo(self.walletBalanceView.snp.leading).offset(-15.0)
    }
    walletBalanceView.snp.makeConstraints {
      $0.firstBaseline.equalTo(self.titleLabel)
      $0.trailing.equalTo(layoutGuide).inset(15.0)
    }
    optionsStackView.snp.makeConstraints {
      $0.top.equalTo(self.titleLabel.snp.bottom).offset(20.0)
      $0.leading.trailing.equalTo(layoutGuide).inset(15.0)
    }
    monthlyToggleButton.snp.makeConstraints {
      $0.top.equalTo(self.optionsStackView.snp.bottom).offset(20.0)
      $0.leading.greaterThanOrEqualTo(layoutGuide).offset(25.0)
      $0.trailing.lessThanOrEqualTo(layoutGuide).offset(25.0)
      $0.centerX.equalTo(self)
    }
    sendTipButton.snp.makeConstraints {
      $0.top.equalTo(self.monthlyToggleButton.snp.bottom).offset(15.0)
      $0.leading.trailing.bottom.equalTo(self)
    }
    insufficientFundsButton.snp.makeConstraints {
      $0.edges.equalTo(self.sendTipButton)
    }
    updateForTraits()
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateForTraits()
  }
  
  func updateForTraits() {
    let isWideLayout = traitCollection.horizontalSizeClass == .regular
    layoutGuide.snp.remakeConstraints {
      if isWideLayout {
        $0.centerX.equalTo(self)
        $0.width.equalTo(375.0)
        $0.top.bottom.equalTo(self)
      } else {
        $0.edges.equalTo(self)
      }
    }
  }
  
  // MARK: - Actions
  
  @objc private func tappedMonthlyToggle() {
    isMonthly.toggle()
  }
  
  @objc private func tappedOption(_ optionView: TippingOptionView) {
    for option in options {
      option.view?.isSelected = option.view === optionView
    }
    
    if let index = selectedOptionIndex {
      optionChanged?(options[index])
      sendTipButton.isEnabled = true
    }
  }
}

extension TippingSelectionView {
  // "wallet balance X BAT"
  fileprivate class WalletBalanceView: UIStackView {
    let titleLabel = UILabel().then {
      $0.appearanceTextColor = Colors.blurple200
      $0.font = .systemFont(ofSize: 12.0)
      $0.text = Strings.tippingWalletBalanceTitle
    }
    let valueLabel = UILabel().then {
      $0.appearanceTextColor = .white
      $0.font = .systemFont(ofSize: 12.0, weight: .medium)
    }
    let cryptoLabel = UILabel().then {
      $0.appearanceTextColor = .white
      $0.font = .systemFont(ofSize: 12.0, weight: .medium)
    }
    
    override init(frame: CGRect) {
      super.init(frame: frame)
      
      spacing = 2.0
      alignment = .firstBaseline
      setContentCompressionResistancePriority(UILayoutPriority(rawValue: 850), for: .horizontal)
      setContentHuggingPriority(.required, for: .horizontal)
      
      addArrangedSubview(titleLabel)
      addArrangedSubview(valueLabel)
      addArrangedSubview(cryptoLabel)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}
