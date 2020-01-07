/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveRewards
import BraveShared

class WalletDetailsViewController: UIViewController, RewardsSummaryProtocol {
  private var ledgerObserver: LedgerObserver
  let state: RewardsState
  private var userWallet: ExternalWallet?
  
  init(state: RewardsState) {
    self.state = state
    ledgerObserver = LedgerObserver(ledger: state.ledger)
    state.ledger.add(ledgerObserver)
    super.init(nibName: nil, bundle: nil)
    setupLedgerObservers()
    reloadUserWallet()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  override func loadView() {
    view = View(isEmpty: state.ledger.balance?.total == 0.0)
  }
  
  var detailsView: View {
    return view as! View // swiftlint:disable:this force_cast
  }
  
  private func reloadUserWallet() {
    if Preferences.Rewards.isUsingBAP.value == true { return }
    state.ledger.externalWallet(forType: .uphold) { [weak self] wallet in
      guard let self = self else { return }
      self.userWallet = wallet
      self.updateWalletStateUI()
    }
  }
  
  private func updateWalletStateUI() {
    guard let wallet = userWallet, isViewLoaded else { return }
    detailsView.walletSection.setButtonType(
      wallet.status == .verified ? .manageFunds : .none,
      animated: true
    )
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    title = Strings.WalletDetailsTitle
    
    detailsView.walletSection.setWalletBalance(
      state.ledger.balanceString,
      crypto: Strings.WalletBalanceType,
      dollarValue: state.ledger.usdBalanceString
    )
    
    detailsView.walletSection.addFundsButton.addTarget(self, action: #selector(tappedAddFunds), for: .touchUpInside)
    detailsView.walletSection.withdrawFundsButton.addTarget(self, action: #selector(tappedWithdrawFunds), for: .touchUpInside)
    updateWalletStateUI()
    
    detailsView.activityView.monthYearLabel.text = summaryPeriod
    detailsView.activityView.rows = summaryRows
    if !disclaimerLabels.isEmpty {
      detailsView.activityView.disclaimerView = WalletDisclaimerView().then {
        $0.labels = disclaimerLabels
      }
    }
    detailsView.activityView.disclaimerView?.labels.forEach {
      $0.onLinkedTapped = { [weak self] _ in
        guard let self = self, let url = URL(string: DisclaimerLinks.unclaimedFundsURL) else { return }
        self.state.delegate?.loadNewTabWithURL(url)
      }
    }
  }
  
  // MARK: - Actions
  
  @objc private func tappedAddFunds() {
    guard let wallet = userWallet, let url = URL(string: wallet.addUrl) else { return }
    state.delegate?.loadNewTabWithURL(url)
  }
  
  @objc private func tappedWithdrawFunds() {
    guard let wallet = userWallet, let url = URL(string: wallet.withdrawUrl) else { return }
    state.delegate?.loadNewTabWithURL(url)
  }
  
  func setupLedgerObservers() {
    ledgerObserver.fetchedBalance = { [weak self] in
      if let self = self {
        self.detailsView.walletSection.setWalletBalance(
          self.state.ledger.balanceString,
          crypto: Strings.WalletBalanceType,
          dollarValue: self.state.ledger.usdBalanceString
        )
      }
    }
    ledgerObserver.balanceReportUpdated = { [weak self] in
      guard let self = self, self.isViewLoaded else {
        return
      }
      UIView.animate(withDuration: 0.15, animations: {
        self.detailsView.activityView.stackView.arrangedSubviews.forEach({
          $0.isHidden = true
          $0.alpha = 0
        })
      }, completion: { _ in
        // Update rows but set them hidden to be animated
        self.detailsView.activityView.rows = self.summaryRows.map { $0.isHidden = true
          return $0
        }
        UIView.animate(withDuration: 0.15, animations: {
          self.detailsView.activityView.stackView.arrangedSubviews.forEach({
            $0.isHidden = false
            $0.alpha = 1
          })
        })
      })
    }
    ledgerObserver.externalWalletAuthorized = { [weak self] _ in
      self?.reloadUserWallet()
    }
  }
}
