// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import StoreKit

private let log = Logger.browserLogger

class BuyVPNViewController: UIViewController {
    
    private var buyVPNView: View {
      return view as! View // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = View()
    }
    
    /// View to show when the vpn purchase is pending.
    private var overlayView: UIView?
    
    private var isLoading: Bool = false {
        didSet {
            overlayView?.removeFromSuperview()
            
            // Toggle 'restore' button.
            navigationItem.rightBarButtonItem?.isEnabled = !isLoading
            
            // Prevent dismissing the modal by swipe when the VPN is being configured
            if #available(iOS 13.0, *) {
                navigationController?.isModalInPresentation = isLoading == true
            }
            
            if !isLoading { return }
            
            let overlay = UIView().then {
                $0.backgroundColor = UIColor.black.withAlphaComponent(0.5)
                let activityIndicator = UIActivityIndicatorView().then { indicator in
                    indicator.startAnimating()
                    indicator.autoresizingMask = [.flexibleWidth, .flexibleHeight]
                }
                
                $0.addSubview(activityIndicator)
            }
            
            buyVPNView.addSubview(overlay)
            overlay.snp.makeConstraints {
                $0.edges.equalToSuperview()
            }
            
            overlayView = overlay
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.VPN.vpnName
        
        navigationItem.rightBarButtonItem = .init(title: Strings.VPN.restorePurchases, style: .done,
                                                  target: self, action: #selector(restorePurchasesAction))
        
        buyVPNView.monthlySubButton
            .addTarget(self, action: #selector(monthlySubscriptionAction), for: .touchUpInside)
        
        buyVPNView.yearlySubButton
            .addTarget(self, action: #selector(yearlySubscriptionAction), for: .touchUpInside)
        
        (UIApplication.shared.delegate as? AppDelegate)?.iapObserver.delegate = self
        
        Preferences.VPN.popupShowed.value = true
    }
    
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        
        // iOS 12 bug, navigation bar color doesn't update in `viewWillAppear`.
        if #available(iOS 13.0, *) {
        } else {
            styleNavigationBar()
        }
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        // For some reason setting `barTintColor` for `formSheet` type of modal doesn't work
        // in `viewDidLoad` method, doing it later as a workaround.
        if #available(iOS 13.0, *) {
            styleNavigationBar()
        }
    }
    
    private func styleNavigationBar() {
        navigationController?.navigationBar.do {
            $0.tintColor = .white
            $0.barTintColor = BraveVPNCommonUI.UX.purpleBackgroundColor
            $0.titleTextAttributes = [NSAttributedString.Key.foregroundColor: UIColor.white]
        }
    }
    
    // MARK: - Button actions
    @objc func monthlySubscriptionAction() {
        guard let monthlySub = VPNProductInfo.monthlySubProduct else {
            log.error("Failed to retrieve monthly subcription product")
            return
        }
        isLoading = true
        let payment = SKPayment(product: monthlySub)
        SKPaymentQueue.default().add(payment)
    }
    
    @objc func closeView() {
        dismiss(animated: true)
    }
    
    @objc func yearlySubscriptionAction() {
        guard let yearlySub = VPNProductInfo.yearlySubProduct else {
            log.error("Failed to retrieve yearly subcription product")
            return
        }
        isLoading = true
        let payment = SKPayment(product: yearlySub)
        SKPaymentQueue.default().add(payment)
    }
    
    @objc func restorePurchasesAction() {
        isLoading = true
        SKPaymentQueue.default().restoreCompletedTransactions()
    }
}

// MARK: - IAPObserverDelegate

extension BuyVPNViewController: IAPObserverDelegate {
    func purchasedOrRestoredProduct() {
        BraveVPN.configureFirstTimeUser { completion in
            DispatchQueue.main.async {
                self.isLoading = false
            }
            
            switch completion {
            case .success:
                // Not using `push` since we don't want the user to go back.
                DispatchQueue.main.async {
                    self.navigationController?.setViewControllers([InstallVPNViewController()], animated:
                    true)
                }
                
                // get the receipt from the server
                BraveVPN.validateReceipt()
                
            case .error(let type):
                let (title, message) = { () -> (String, String) in
                    switch type {
                    // At the moment all errors have the same message.
                    // Still keeping a switch here to remind us of this place if new error type is added.
                    case .connectionProblems, .provisioning, .unknown:
                        return (Strings.VPN.vpnConfigPermissionDeniedErrorTitle,
                                Strings.VPN.vpnConfigPermissionDeniedErrorBody)
                    }
                }()
                
                DispatchQueue.main.async {
                    let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
                    let ok = UIAlertAction(title: Strings.OKString, style: .default, handler: nil)
                    alert.addAction(ok)
                    self.present(alert, animated: true)
                }
            }
        }
    }
    
    func purchaseFailed(error: IAPObserver.PurchaseError) {
        DispatchQueue.main.async {
            self.isLoading = false
            
            // User intentionally tapped to cancel purchase , no need to show any alert on our side.
            if case .transactionError(let err) = error, err?.code == SKError.paymentCancelled {
                return
            }
            
            // For all other errors, we attach associated code for easier debugging.
            // See SKError.h for list of all codes.
            let message = Strings.VPN.vpnErrorPurchaseFailedBody
            
            let alert = UIAlertController(title: Strings.VPN.vpnErrorPurchaseFailedTitle,
                                          message: message,
                                          preferredStyle: .alert)
            let ok = UIAlertAction(title: Strings.OKString, style: .default, handler: nil)
            alert.addAction(ok)
            self.present(alert, animated: true)
        }
    }
}
