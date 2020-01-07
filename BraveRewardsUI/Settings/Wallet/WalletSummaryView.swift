/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

class SettingsWalletSectionView: SettingsSectionView {
  enum ButtonType {
    case none
    case viewDetails
    case manageFunds
  }
  
  func setWalletBalance(_ value: String, crypto: String, dollarValue: String) {
    balanceLabel.text = value
    altcurrencyTypeLabel.text = crypto
    usdBalanceLabel.text = dollarValue
  }
  
  let viewDetailsButton = SettingsViewDetailsButton(type: .system).then {
    $0.tintColor = .white
    $0.appearanceTextColor = .white
    $0.isHidden = true
  }
  
  let addFundsButton = Button(type: .system).then {
    $0.appearanceTintColor = UIColor(white: 1.0, alpha: 0.75)
    $0.setTitle(Strings.AddFunds, for: .normal)
    $0.setImage(UIImage(frameworkResourceNamed: "add-funds-icon").alwaysTemplate, for: .normal)
    $0.imageEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 8)
    $0.titleEdgeInsets = UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 0)
    $0.contentEdgeInsets = UIEdgeInsets(top: 10, left: 5.0, bottom: 10, right: 5.0)
    $0.titleLabel?.font = .systemFont(ofSize: 14.0)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }
  
  let withdrawFundsButton = Button(type: .system).then {
    $0.appearanceTintColor = UIColor(white: 1.0, alpha: 0.75)
    $0.setTitle(Strings.WithdrawFunds, for: .normal)
    $0.setImage(UIImage(frameworkResourceNamed: "withdraw-funds-icon").alwaysTemplate, for: .normal)
    $0.imageEdgeInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 8)
    $0.titleEdgeInsets = UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 0)
    $0.contentEdgeInsets = UIEdgeInsets(top: 10, left: 5.0, bottom: 10, right: 5.0)
    $0.titleLabel?.font = .systemFont(ofSize: 14.0)
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
  }
  
  private lazy var buttonsStackView = UIStackView().then {
    $0.spacing = 10
    $0.alignment = .center
    $0.isHidden = true
    $0.addArrangedSubview(self.addFundsButton)
    $0.addArrangedSubview(self.withdrawFundsButton)
  }
  
  private let backgroundView = GradientView().then {
    $0.gradientLayer.colors = [ UIColor(57, 45, 209).cgColor,
                                UIColor(87, 19, 166).cgColor ]
    $0.gradientLayer.startPoint = CGPoint(x: 0.2, y: 0)
    $0.gradientLayer.endPoint = CGPoint(x: 1.0, y: 1.5)
  }
  
  private let watermarkImageView = UIImageView(image: UIImage(frameworkResourceNamed: "bat-watermark"))
  
  private let stackView = UIStackView().then {
    $0.axis = .vertical
    $0.spacing = 4.0
    $0.alignment = .center
  }
  
  private let titleLabel = UILabel().then {
    $0.font = .systemFont(ofSize: 14.0, weight: .medium)
    $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.65)
    $0.text = Strings.WalletHeaderTitle
  }
  
  private let altcurrencyContainerView = UIStackView().then {
    $0.spacing = 4.0
    $0.alignment = .lastBaseline
  }
  
  let balanceLabel = UILabel().then {
    $0.appearanceTextColor = .white
    $0.font = .systemFont(ofSize: 30.0)
  }
  
  let altcurrencyTypeLabel = UILabel().then {
    $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.65)
    $0.font = .systemFont(ofSize: 16.0)
  }
  
  let usdBalanceLabel = UILabel().then {
    $0.textAlignment = .center
    $0.appearanceTextColor = UIColor(white: 1.0, alpha: 0.65)
    $0.font = .systemFont(ofSize: 12.0)
  }
  
  private var activeButtonTypeView: UIView? {
    didSet {
      oldValue?.isHidden = true
      if let view = activeButtonTypeView {
        view.isHidden = false
      }
    }
  }
  
  private func viewForButtonType(_ buttonType: ButtonType) -> UIView? {
    switch buttonType {
    case .none: return nil
    case .manageFunds: return buttonsStackView
    case .viewDetails: return viewDetailsButton
    }
  }
  
  func setButtonType(_ type: ButtonType, animated: Bool = false) {
    if !animated {
      activeButtonTypeView = viewForButtonType(type)
    } else {
      let view = viewForButtonType(type)
      view?.alpha = 0.0
      UIView.animate(withDuration: 0.25) {
        self.activeButtonTypeView?.alpha = 0.0
        self.activeButtonTypeView = view
        view?.alpha = 1.0
      }
    }
  }
  
  init(buttonType: ButtonType) {
    super.init(frame: .zero)
    
    clippedContentView.addSubview(backgroundView)
    clippedContentView.addSubview(watermarkImageView)
    clippedContentView.addSubview(titleLabel)
    clippedContentView.addSubview(stackView)
    stackView.addArrangedSubview(altcurrencyContainerView)
    stackView.addArrangedSubview(usdBalanceLabel)
    stackView.setCustomSpacing(8.0, after: usdBalanceLabel)
    altcurrencyContainerView.addArrangedSubview(balanceLabel)
    altcurrencyContainerView.addArrangedSubview(altcurrencyTypeLabel)
    
    stackView.addArrangedSubview(viewDetailsButton)
    stackView.addArrangedSubview(buttonsStackView)
    
    setButtonType(buttonType)
    
    backgroundView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
    watermarkImageView.snp.makeConstraints {
      $0.top.equalTo(self)
      $0.leading.equalTo(self).offset(10.0)
    }
    titleLabel.snp.makeConstraints {
      $0.top.leading.trailing.equalTo(self).inset(10.0)
    }
    stackView.snp.makeConstraints {
      $0.top.equalTo(self.titleLabel.snp.bottom)
      $0.leading.trailing.bottom.equalTo(self).inset(15.0)
    }
  }
}
