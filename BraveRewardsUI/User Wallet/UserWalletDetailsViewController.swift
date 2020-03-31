// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import BraveUI

final class UserWalletDetailsViewController: UIViewController {
 
  private let state: RewardsState
  private let wallet: ExternalWallet
  
  private var disconnectedWalletHandler: () -> Void
  
  init(state: RewardsState, wallet: ExternalWallet, disconnectedWalletHandler: @escaping () -> Void) {
    self.state = state
    self.wallet = wallet
    self.disconnectedWalletHandler = disconnectedWalletHandler
    
    super.init(nibName: nil, bundle: nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func loadView() {
    view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = Strings.userWalletDetailsTitle
    
    let view = self.view as! View // swiftlint:disable:this force_cast
    view.summaryView.nameLabel.text = wallet.userName
    view.summaryView.verifiedLabel.isHidden = wallet.status != .verified
    view.summaryView.manageFundsStackView.isHidden = wallet.status != .verified
    view.summaryView.continueVerificationStackView.isHidden = wallet.status == .verified
    
    view.summaryView.upholdAccountButton.addTarget(self, action: #selector(tappedUpholdAccountButton), for: .touchUpInside)
    view.summaryView.completeVerificationButton.addTarget(self, action: #selector(tappedCompleteVerificationButton), for: .touchUpInside)
    view.summaryView.addFundsButton.addTarget(self, action: #selector(tappedAddFundsButton), for: .touchUpInside)
    view.summaryView.withdrawFundsButton.addTarget(self, action: #selector(tappedWithdrawFundsButton), for: .touchUpInside)
    view.disconnectFromRewardsButton.addTarget(self, action: #selector(tappedDisconnectButton), for: .touchUpInside)
  }
  
  @objc private func tappedAddFundsButton() {
    dismiss(animated: true) {
      guard let url = URL(string: self.wallet.addUrl) else { return }
      self.state.delegate?.loadNewTabWithURL(url)
    }
  }
  
  @objc private func tappedWithdrawFundsButton() {
    dismiss(animated: true) {
      guard let url = URL(string: self.wallet.withdrawUrl) else { return }
      self.state.delegate?.loadNewTabWithURL(url)
    }
  }
  
  @objc private func tappedUpholdAccountButton() {
    dismiss(animated: true) {
      guard let url = URL(string: self.wallet.accountUrl) else { return }
      self.state.delegate?.loadNewTabWithURL(url)
    }
  }
  
  @objc private func tappedCompleteVerificationButton() {
    dismiss(animated: true) {
      guard let percentEncoded = self.wallet.verifyUrl.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed),
        let url = URL(string: percentEncoded) else { return }
      self.state.delegate?.loadNewTabWithURL(url)
    }
  }
  
  @objc private func tappedDisconnectButton() {
    disconnectedWalletHandler()
  }
}

private extension UserWalletDetailsViewController {
  
  class View: UIView {
    fileprivate let summaryView = UserWalletSummarySectionView()
    fileprivate let disconnectFromRewardsButton = UserWalletButton(type: .system).then {
      $0.appearanceTintColor = Colors.red600
      $0.setTitle(Strings.userWalletDetailsDisconnectButtonTitle, for: .normal)
    }
    
    override init(frame: CGRect) {
      super.init(frame: frame)

      let scrollView = UIScrollView().then {
        $0.alwaysBounceVertical = true
      }
      
      let stackView = UIStackView().then {
        $0.axis = .vertical
        $0.spacing = 10
      }
      
      backgroundColor = SettingsUX.backgroundColor
      
      addSubview(scrollView)
      scrollView.addSubview(stackView)
      
      stackView.addStackViewItems(
        .view(summaryView),
        .view(SettingsSectionView().then {
          $0.addSubview(disconnectFromRewardsButton)
          disconnectFromRewardsButton.snp.makeConstraints {
            $0.edges.equalToSuperview()
          }
        })
      )
      
      scrollView.snp.makeConstraints {
        $0.edges.equalTo(self)
      }
      scrollView.contentLayoutGuide.snp.makeConstraints {
        $0.width.equalTo(self)
      }
      stackView.snp.makeConstraints {
        $0.edges.equalTo(scrollView.contentLayoutGuide).inset(10.0)
      }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
  }
}

private class UserWalletSummarySectionView: SettingsSectionView {
  
  // Always Uphold for now
  let userWalletIcon = UIImageView(image: UIImage(frameworkResourceNamed: "uphold").alwaysTemplate).then {
    $0.tintColor = BraveUX.upholdGreen
    $0.setContentHuggingPriority(.required, for: .horizontal)
  }
  
  let nameLabel = UILabel().then {
    $0.appearanceTextColor = .black
    $0.font = .systemFont(ofSize: 15.0)
  }
  
  lazy var continueVerificationStackView = UIStackView().then {
    $0.axis = .vertical
    $0.isHidden = true
    $0.addStackViewItems(
      .view(completeVerificationButton),
      .view(SeparatorView())
    )
  }
  
  lazy var manageFundsStackView = UIStackView().then {
    $0.axis = .vertical
    $0.isHidden = true
    $0.addStackViewItems(
      .view(self.addFundsButton),
      .view(SeparatorView()),
      .view(self.withdrawFundsButton),
      .view(SeparatorView())
    )
  }
  
  let verifiedLabel = UILabel().then {
    $0.text = Strings.userWalletDetailsVerified
    $0.font = .systemFont(ofSize: 15.0)
    $0.appearanceTextColor = BraveUX.upholdGreen
    $0.isHidden = true
    $0.setContentCompressionResistancePriority(.required, for: .horizontal)
    $0.setContentHuggingPriority(.defaultHigh, for: .horizontal)
  }
  
  let upholdAccountButton = UserWalletButton(type: .system).then {
    $0.setTitle(Strings.userWalletDetailsAccountButtonTitle, for: .normal)
  }
  
  let completeVerificationButton = UserWalletButton(type: .system).then {
    $0.setTitle(Strings.userWalletDetailsCompleteVerificationButtonTitle, for: .normal)
  }
  
  let addFundsButton = UserWalletButton(type: .system).then {
    $0.setTitle(Strings.addFunds, for: .normal)
  }
  
  let withdrawFundsButton = UserWalletButton(type: .system).then {
    $0.setTitle(Strings.withdrawFunds, for: .normal)
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    let stackView = UIStackView().then {
      $0.axis = .vertical
    }
    
    let nameStackView = UIStackView().then {
      $0.spacing = 8
      $0.layoutMargins = UIEdgeInsets(top: 16, left: 16, bottom: 16, right: 16)
      $0.isLayoutMarginsRelativeArrangement = true
    }
    
    nameStackView.addStackViewItems(
      .view(userWalletIcon),
      .view(nameLabel),
      .view(verifiedLabel)
    )
    
    addSubview(stackView)
    stackView.addStackViewItems(
      .view(nameStackView),
      .view(SeparatorView()),
      .view(continueVerificationStackView),
      .view(manageFundsStackView),
      .view(upholdAccountButton)
    )
    
    stackView.snp.makeConstraints {
      $0.edges.equalTo(self)
    }
  }
}

final private class UserWalletButton: Button {
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    
    contentHorizontalAlignment = .left
    titleLabel?.font = .systemFont(ofSize: 14)
    contentEdgeInsets = UIEdgeInsets(top: 12, left: 16, bottom: 12, right: 16)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
