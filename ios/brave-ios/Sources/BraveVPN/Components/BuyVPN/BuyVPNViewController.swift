// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Preferences
import SafariServices
import Shared
import StoreKit
import UIKit
import os.log

public class BuyVPNViewController: VPNSetupLoadingController {

  let iapObserver: BraveVPNInAppPurchaseObserver
  private var iapRestoreTimer: Timer?

  var activeSubcriptionChoice: VPNSubscriptionType = .yearly {
    didSet {
      buyVPNView.activeSubcriptionChoice = activeSubcriptionChoice
    }
  }

  public var openAuthenticationVPNInNewTab: (() -> Void)?

  init(iapObserver: BraveVPNInAppPurchaseObserver) {
    self.iapObserver = iapObserver
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) { fatalError() }

  private var buyVPNView = BuyVPNView(with: .yearly)

  public override func viewDidLoad() {
    super.viewDidLoad()

    title = Strings.VPN.vpnName
    view.backgroundColor = BraveVPNCommonUI.UX.purpleBackgroundColor

    navigationItem.standardAppearance = BraveVPNCommonUI.navigationBarAppearance
    navigationItem.scrollEdgeAppearance = BraveVPNCommonUI.navigationBarAppearance

    navigationItem.rightBarButtonItem = .init(
      title: Strings.VPN.restorePurchases,
      style: .done,
      target: self,
      action: #selector(restorePurchasesAction)
    )

    let buyTitle =
      Preferences.VPN.freeTrialUsed.value
      ? Strings.VPN.activateSubscriptionAction.capitalized
      : Strings.VPN.freeTrialPeriodAction.capitalized

    let buyButton = BraveGradientButton(gradient: .backgroundGradient1).then {
      $0.titleLabel?.font = .systemFont(ofSize: 15, weight: .bold)
      $0.titleLabel?.textAlignment = .center

      $0.setTitle(buyTitle, for: .normal)

      $0.snp.makeConstraints {
        $0.height.equalTo(50)
      }

      $0.layer.do {
        $0.cornerRadius = 24
        $0.cornerCurve = .continuous
        $0.masksToBounds = true
      }

      $0.addTarget(self, action: #selector(startSubscriptionAction), for: .touchUpInside)
    }

    let redeemButton = UIButton().then {
      $0.setTitle(Strings.VPN.vpnRedeemCodeButtonActionTitle, for: .normal)
      $0.titleLabel?.font = .systemFont(ofSize: 13, weight: .medium)
      $0.titleLabel?.textAlignment = .center

      $0.addTarget(self, action: #selector(redeemOfferSubscriptionCode), for: .touchUpInside)
    }

    let seperator = UIView().then {
      $0.backgroundColor = UIColor.white.withAlphaComponent(0.1)
      $0.snp.makeConstraints { make in
        make.height.equalTo(1)
      }
    }

    view.addSubview(buyVPNView)
    view.addSubview(seperator)
    view.addSubview(buyButton)
    view.addSubview(redeemButton)

    buyVPNView.snp.makeConstraints {
      $0.leading.trailing.top.equalToSuperview()
    }

    seperator.snp.makeConstraints {
      $0.top.equalTo(buyVPNView.snp.bottom)
      $0.leading.trailing.equalToSuperview()
    }

    buyButton.snp.makeConstraints {
      $0.top.equalTo(seperator.snp.bottom).inset(-12)
      $0.leading.trailing.equalToSuperview().inset(24)
    }

    redeemButton.snp.makeConstraints {
      $0.top.equalTo(buyButton.snp.bottom).inset(-12)
      $0.bottom.equalToSuperview().inset(24)
      $0.centerX.equalToSuperview()
    }

    buyVPNView.monthlySubButton
      .addTarget(self, action: #selector(monthlySubscriptionAction), for: .touchUpInside)

    buyVPNView.yearlySubButton
      .addTarget(self, action: #selector(yearlySubscriptionAction), for: .touchUpInside)

    iapObserver.delegate = self
    buyVPNView.delegate = self

    Preferences.VPN.popupShowed.value = true
  }

  public override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    // navigationItem.standardAppearance does not support tinting the back button for some
    // reason, so we still must apply a custom tint to the bar
    navigationController?.navigationBar.tintColor = .white
  }

  public override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)

    // Reset styling set above
    navigationController?.navigationBar.tintColor = UINavigationBar.appearance().tintColor
  }

  // MARK: - Button Actions

  @objc func yearlySubscriptionAction() {
    activeSubcriptionChoice = .yearly
  }

  @objc func monthlySubscriptionAction() {
    activeSubcriptionChoice = .monthly
  }

  @objc func closeView() {
    dismiss(animated: true)
  }

  @objc func restorePurchasesAction() {
    isLoading = true
    SKPaymentQueue.default().restoreCompletedTransactions()

    if iapRestoreTimer != nil {
      iapRestoreTimer?.invalidate()
      iapRestoreTimer = nil
    }

    // Adding 1 minute timer for restore
    iapRestoreTimer = Timer.scheduledTimer(
      timeInterval: 1.minutes,
      target: self,
      selector: #selector(handleRestoreTimeoutFailure),
      userInfo: nil,
      repeats: false
    )
  }

  @objc func startSubscriptionAction() {
    addPaymentForSubcription(type: activeSubcriptionChoice)
  }

  @objc func redeemOfferSubscriptionCode() {
    // Open the redeem code sheet
    SKPaymentQueue.default().presentCodeRedemptionSheet()
  }

  private func addPaymentForSubcription(type: VPNSubscriptionType) {
    var subscriptionProduct: SKProduct?

    switch type {
    case .yearly:
      subscriptionProduct = BraveVPNProductInfo.yearlySubProduct
    case .monthly:
      subscriptionProduct = BraveVPNProductInfo.monthlySubProduct
    }

    guard let subscriptionProduct = subscriptionProduct else {
      Logger.module.error("Failed to retrieve \(type.rawValue) subcription product")
      return
    }

    isLoading = true
    let payment = SKPayment(product: subscriptionProduct)
    SKPaymentQueue.default().add(payment)
  }
}

// MARK: - IAPObserverDelegate

extension BuyVPNViewController: BraveVPNInAppPurchaseObserverDelegate {
  public func purchasedOrRestoredProduct(validateReceipt: Bool) {
    DispatchQueue.main.async {
      self.isLoading = false
    }

    // Not using `push` since we don't want the user to go back.
    DispatchQueue.main.async {
      self.navigationController?.setViewControllers(
        [BraveVPNInstallViewController()],
        animated: true
      )
    }

    if validateReceipt {
      Task {
        _ = try await BraveVPN.validateReceiptData()
      }
    }
  }

  public func purchaseFailed(error: BraveVPNInAppPurchaseObserver.PurchaseError) {
    // Handle Transaction or Restore error
    guard isLoading else {
      return
    }

    handleTransactionError(error: error)
  }

  public func handlePromotedInAppPurchase() {
    // No-op In app purchase promotion is handled on bvc
  }

  @objc func handleRestoreTimeoutFailure() {
    // Handle Restore error from timeout
    guard isLoading else {
      return
    }

    let errorRestore = SKError(SKError.unknown, userInfo: ["detail": "time-out"])
    handleTransactionError(error: .transactionError(error: errorRestore))
  }

  private func handleTransactionError(error: BraveVPNInAppPurchaseObserver.PurchaseError) {
    DispatchQueue.main.async {
      self.isLoading = false

      // User intentionally tapped to cancel purchase , no need to show any alert on our side.
      if case .transactionError(let err) = error, err?.code == SKError.paymentCancelled {
        return
      }

      // For all other errors, we attach associated code for easier debugging.
      // See SKError.h for list of all codes.
      let message = Strings.VPN.vpnErrorPurchaseFailedBody

      let alert = UIAlertController(
        title: Strings.VPN.vpnErrorPurchaseFailedTitle,
        message: message,
        preferredStyle: .alert
      )
      let ok = UIAlertAction(title: Strings.OKString, style: .default, handler: nil)
      alert.addAction(ok)
      self.present(alert, animated: true)
    }
  }
}

// MARK: - BuyVPNVActionDelegate

extension BuyVPNViewController: BuyVPNView.ActionDelegate {

  func refreshSiteCredentials() {
    openAuthenticationVPNInNewTab?()

    dismiss(animated: true)
  }
}

public class VPNSetupLoadingController: UIViewController {

  private var overlayView: UIView?

  var isLoading: Bool = false {
    didSet {
      overlayView?.removeFromSuperview()

      // Disable Action bar button while loading
      navigationItem.rightBarButtonItem?.isEnabled = !isLoading

      // Prevent dismissing the modal by swipe
      navigationController?.isModalInPresentation = isLoading == true

      if !isLoading { return }

      let overlay = UIView().then {
        $0.backgroundColor = UIColor.black.withAlphaComponent(0.5)
        let activityIndicator = UIActivityIndicatorView().then {
          $0.startAnimating()
          $0.autoresizingMask = [.flexibleWidth, .flexibleHeight]
          $0.style = .large
          $0.color = .white
        }

        $0.addSubview(activityIndicator)
      }

      view.addSubview(overlay)
      overlay.snp.makeConstraints {
        $0.edges.equalToSuperview()
      }

      overlayView = overlay
    }
  }
}

extension BraveGradient {

  public static var backgroundGradient1: BraveGradient {
    .init(
      stops: [
        .init(color: UIColor(rgb: 0x8b10c6), position: 0.22),
        .init(color: UIColor(rgb: 0xd02480), position: 0.7),
        .init(color: UIColor(rgb: 0xe95e3c), position: 0.96),
      ],
      angle: .figmaDegrees(138)
    )
  }
}
