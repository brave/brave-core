/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class CreateWalletViewController: UIViewController {
  
  let state: RewardsState
  
  var createWalletView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  init(state: RewardsState) {
    self.state = state
    super.init(nibName: nil, bundle: nil)
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
  
  // MARK: - Actions
  
  @objc private func tappedCreateWallet() {
    if createWalletView.createWalletButton.isCreatingWallet {
      return
    }
    createWalletView.createWalletButton.isCreatingWallet = true
    state.ledger.createWalletAndFetchDetails { [weak self] success in
      guard let self = self else { return }
      if !success {
        let alertController = UIAlertController(title: "Error", message: "Oops! Something went wrong. Please try again.", preferredStyle: .alert)
        alertController.addAction(UIAlertAction(title: "OK", style: .default, handler: nil))
        self.present(alertController, animated: true)
        self.createWalletView.createWalletButton.isCreatingWallet = false
        return
      }
      defer { self.createWalletView.createWalletButton.isCreatingWallet = false }
      self.show(WalletViewController(state: self.state), sender: self)
    }
  }
  
  @objc private func tappedLearnMore() {
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
