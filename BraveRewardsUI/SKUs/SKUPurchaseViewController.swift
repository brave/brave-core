// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import BraveShared
import BraveUI

public class SKUPurchaseViewController: UIViewController, UIViewControllerTransitioningDelegate {
  
  private let rewards: BraveRewards
  private let publisher: PublisherInfo
  private let request: PaymentRequest
  private let ledgerObserver: LedgerObserver
  private var openBraveTermsOfSale: () -> Void
  private let responseHandler: (_ response: PaymentRequestResponse) -> Void
  
  public init(
    rewards: BraveRewards,
    publisher: PublisherInfo,
    request: PaymentRequest,
    responseHandler: @escaping (_ response: PaymentRequestResponse) -> Void,
    openBraveTermsOfSale: @escaping () -> Void
  ) {
    self.rewards = rewards
    self.publisher = publisher
    self.request = request
    self.responseHandler = responseHandler
    self.openBraveTermsOfSale = openBraveTermsOfSale
    self.ledgerObserver = LedgerObserver(ledger: rewards.ledger)
    
    super.init(nibName: nil, bundle: nil)
    
    modalPresentationStyle = .overCurrentContext
    if #available(iOS 13.0, *) {
      isModalInPresentation = true
    }
    transitioningDelegate = self
    
    self.rewards.ledger.add(self.ledgerObserver)
    setupLedgerObserver()
    
    rewards.ledger.fetchBalance(nil)
  }
  
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
  
  private var purchaseView: SKUPurchaseView {
    return view as! SKUPurchaseView // swiftlint:disable:this force_cast
  }
  
  public override func loadView() {
    view = SKUPurchaseView()
  }
  
  public override func viewDidLoad() {
    super.viewDidLoad()
    
    title = Strings.SKUPurchaseTitle
    
    purchaseView.detailView.itemDetailValueLabel.text = request.details.displayItems.map { $0.label }.joined(separator: ", ")
    purchaseView.detailView.orderAmountLabels.batContainer.amountLabel.text = request.details.total.amount.value
    
    purchaseView.detailView.dismissButton.addTarget(self, action: #selector(tappedDismissButton), for: .touchUpInside)
    purchaseView.gesturalDismissExecuted = { [unowned self] in
      self.tappedDismissButton()
    }
    purchaseView.buyButton.buyButton.addTarget(self, action: #selector(tappedBuyButton), for: .touchUpInside)
    purchaseView.buyButton.disclaimerLabel.onLinkedTapped = { [weak self] _ in
      self?.openBraveTermsOfSale()
    }
    
    updateViewForBalance()
  }
  
  func setupLedgerObserver() {
    ledgerObserver.fetchedBalance = { [weak self] in
      self?.updateViewForBalance()
    }
  }
  
  @objc private func tappedDismissButton() {
    if purchaseView.viewState == .overview {
      responseHandler(.cancelled)
    }
    dismiss(animated: true)
  }
  
  @objc private func tappedBuyButton() {
    let items = zip(request.details.skuTokens, request.details.displayItems).map { (sku, item) in
      return SKUOrderItem().then {
        $0.sku = sku
        $0.quantity = 1
      }
    }
    
    // Start order transactions
    purchaseView.viewState = .processing
    
    rewards.ledger.processSKUItems(items) { (result, orderID) in
      // Allow animations to complete
      DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
        self.purchaseView.viewState = .complete
      }
      self.responseHandler(.completed(orderID))
    }
  }
  
  private func updateViewForBalance() {
    guard isViewLoaded, let balance = rewards.ledger.balance, let amount = Double(request.details.total.amount.value) else { return }
    purchaseView.detailView.orderAmountLabels.usdContainer.amountLabel.text = rewards.ledger.dollarStringForBATAmount(request.details.total.amount.value, includeCurrencyCode: false)
    purchaseView.detailView.balanceView.amountLabels.batContainer.amountLabel.text = "\(balance.total)"
    purchaseView.detailView.balanceView.amountLabels.usdContainer.amountLabel.text = rewards.ledger.dollarStringForBATAmount(balance.total, includeCurrencyCode: false)
    purchaseView.isShowingInsufficientFundsView = balance.total < amount
  }
  
  // MARK: -
  
  override public var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    if UIDevice.current.userInterfaceIdiom == .phone {
      return .portrait
    }
    return .all
  }
  
  // MARK: - UIViewControllerTransitioningDelegate
  
  public func animationController(forPresented presented: UIViewController, presenting: UIViewController, source: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: purchaseView, direction: .presenting)
  }
  
  public func animationController(forDismissed dismissed: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return BasicAnimationController(delegate: purchaseView, direction: .dismissing)
  }
}
