/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards
import BraveShared

class CreateWalletViewController: UIViewController {
  
  let state: RewardsState
  
  var createWalletView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  init(state: RewardsState) {
    self.state = state
    self.observer = LedgerObserver(ledger: state.ledger)
    super.init(nibName: nil, bundle: nil)
    setupLedgerObserver()
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func loadView() {
    self.view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    navigationController?.setNavigationBarHidden(true, animated: false)
    
    createWalletView.createWalletButton.addTarget(self, action: #selector(tappedCreateWallet), for: .touchUpInside)
    createWalletView.learnMoreButton.addTarget(self, action: #selector(tappedLearnMore), for: .touchUpInside)
    createWalletView.termsOfServiceLabel.onLinkedTapped = tappedDisclaimerLink
    createWalletView.createWalletButton.isCreatingWallet = state.ledger.isInitializingWallet
    
    let size = CGSize(width: RewardsUX.preferredPanelSize.width, height: UIScreen.main.bounds.height)
    preferredContentSize = view.systemLayoutSizeFitting(
      size,
      withHorizontalFittingPriority: .required,
      verticalFittingPriority: .fittingSizeLevel
    )
  }
  
  override func viewDidDisappear(_ animated: Bool) {
    super.viewDidDisappear(animated)
    
    // When this view disappears we want to remove it from the naviagation stack
    if let nc = navigationController, nc.viewControllers.first === self {
      nc.viewControllers = [UIViewController](nc.viewControllers.dropFirst())
    }
  }
  
  let observer: LedgerObserver
  
  func setupLedgerObserver() {
    state.ledger.add(observer)
    observer.walletInitalized = { [weak self] result in
      guard let self = self else { return }
      self.createWalletView.createWalletButton.isCreatingWallet = false
      if result == .ledgerOk || result == .walletCreated {
        self.show(WalletViewController(state: self.state), sender: self)
      } else {
        self.showFailureAlert()
      }
    }
  }
  
  // MARK: - Actions
  
  func showFailureAlert() {
    let alertController = UIAlertController(title: Strings.GenericErrorTitle, message: Strings.GenericErrorBody, preferredStyle: .alert)
    alertController.addAction(UIAlertAction(title: Strings.OK, style: .default, handler: nil))
    self.present(alertController, animated: true)
  }
  
  @objc private func tappedCreateWallet() {
    if createWalletView.createWalletButton.isCreatingWallet {
      return
    }
    createWalletView.createWalletButton.isCreatingWallet = true
    state.ledger.createWalletAndFetchDetails { _ in }
  }
  
  @objc private func tappedLearnMore() {
    state.ledger.remove(observer) // No way to come back to this screen anyways
    let controller = WelcomeViewController(state: state)
    navigationController?.pushViewController(controller, animated: true)
  }
  
  private func tappedDisclaimerLink(_ url: URL) {
    switch url.path {
    case "terms":
      guard let url = URL(string: DisclaimerLinks.termsOfUseURL) else { return }
      state.delegate?.loadNewTabWithURL(url)
      
    case "policy":
      guard let url = URL(string: DisclaimerLinks.policyURL) else { return }
      state.delegate?.loadNewTabWithURL(url)
      
    default:
      assertionFailure()
    }
  }
}
