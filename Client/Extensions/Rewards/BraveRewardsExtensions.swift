// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import BraveShared

extension BraveRewards {
    
    /// Whether or not Brave Rewards is available/can be enabled
    public static var isAvailable: Bool {
        #if DEBUG
        return true
        #else
        return DCDevice.current.isSupported
        #endif
    }
    
    /// Whether or not rewards is enabled
    @objc public var isEnabled: Bool {
        get {
            ledger.isWalletCreated && ledger.isEnabled && isAdsEnabled
        }
        set {
            willChangeValue(for: \.isEnabled)
            Preferences.Rewards.rewardsToggledOnce.value = true
            createWalletIfNeeded { [weak self] in
                guard let self = self else { return }
                self.ledger.isEnabled = newValue
                self.ledger.isAutoContributeEnabled = newValue
                self.isAdsEnabled = newValue
                self.didChangeValue(for: \.isEnabled)
            }
        }
    }
    
    public func createWalletIfNeeded(_ completion: (() -> Void)? = nil) {
        if ledger.isWalletCreated {
            completion?()
            return
        }
        if isCreatingWallet {
            // completion block will be hit by previous call
            return
        }
        isCreatingWallet = true
        ledger.createWalletAndFetchDetails { [weak self] success in
            self?.isCreatingWallet = false
            completion?()
        }
    }
    
    public var isCreatingWallet: Bool {
        get { objc_getAssociatedObject(self, &AssociatedKeys.isCreatingWallet) as? Bool ?? false }
        set { objc_setAssociatedObject(self, &AssociatedKeys.isCreatingWallet, newValue, .OBJC_ASSOCIATION_ASSIGN) }
    }
}

private struct AssociatedKeys {
  static var isCreatingWallet: Int = 0
}
  
