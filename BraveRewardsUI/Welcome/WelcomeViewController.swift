/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards
import BraveShared

class WelcomeViewController: UIViewController {
  
  let state: RewardsState
  
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
  
  var welcomeView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  override func loadView() {
    view = View()
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    preferredContentSize = CGSize(width: RewardsUX.preferredPanelSize.width, height: 750)
    
    welcomeView.createWalletButtons.forEach {
      $0.addTarget(self, action: #selector(tappedCreateWallet(_:)), for: .touchUpInside)
    }
    welcomeView.createWalletButtons.forEach {
      $0.isCreatingWallet = state.ledger.isInitializingWallet
    }
    
    welcomeView.termsOfServiceLabels.forEach({
      $0.onLinkedTapped = self.tappedDisclaimerLink
    })
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
      self.welcomeView.createWalletButtons.forEach {
        $0.isCreatingWallet = false
      }
      if result == .ledgerOk || result == .walletCreated {
        self.show(WalletViewController(state: self.state), sender: self)
      } else {
        self.showFailureAlert()
      }
    }
  }
  
  func showFailureAlert() {
    let alertController = UIAlertController(title: Strings.WalletCreationErrorTitle, message: Strings.WalletCreationErrorBody, preferredStyle: .alert)
    alertController.addAction(UIAlertAction(title: Strings.OK, style: .default, handler: nil))
    self.present(alertController, animated: true)
  }
  
  @objc private func tappedCreateWallet(_ sender: CreateWalletButton) {
    if sender.isCreatingWallet {
      return
    }
    welcomeView.createWalletButtons.forEach { $0.isCreatingWallet = true }
    state.ledger.createWalletAndFetchDetails { _ in }
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
