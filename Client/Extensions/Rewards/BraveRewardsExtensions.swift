// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import BraveShared
import Shared

extension BraveRewards {
    
    /// Whether or not Brave Rewards is available/can be enabled
    public static var isAvailable: Bool {
        #if MOZ_CHANNEL_DEBUG
        return true
        #else
        return DCDevice.current.isSupported
        #endif
    }
    
    /// Whether or not rewards is enabled
    @objc public var isEnabled: Bool {
        get {
            isAdsEnabled
        }
        set {
            willChangeValue(for: \.isEnabled)
            Preferences.Rewards.rewardsToggledOnce.value = true
            createWalletIfNeeded { [weak self] in
                guard let self = self else { return }
                self.ledger?.isAutoContributeEnabled = newValue
                self.isAdsEnabled = newValue
                self.didChangeValue(for: \.isEnabled)
            }
        }
    }
    
    public func createWalletIfNeeded(_ completion: (() -> Void)? = nil) {
        if isCreatingWallet {
            // completion block will be hit by previous call
            return
        }
        isCreatingWallet = true
        startLedgerService {
            guard let ledger = self.ledger else { return }
            ledger.createWalletAndFetchDetails { [weak self] success in
                self?.isCreatingWallet = false
                completion?()
            }
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

extension DrainStatus: RepresentableOptionType {
    public var displayString: String {
        switch self {
        case .invalid:
            return Strings.Rewards.legacyWalletTransferStatusShortformInvalid
        case .pending:
            return Strings.Rewards.legacyWalletTransferStatusShortformPending
        case .inProgress:
            return Strings.Rewards.legacyWalletTransferStatusShortformInProgress
        case .delayed:
            return Strings.Rewards.legacyWalletTransferStatusShortformDelayed
        case .complete:
            return Strings.Rewards.legacyWalletTransferStatusShortformCompleted
        @unknown default:
            return ""
        }
    }
    var statusButtonTitle: String {
        switch self {
        case .invalid:
            return Strings.Rewards.legacyWalletTransferStatusButtonInvalidTitle
        case .pending:
            return Strings.Rewards.legacyWalletTransferStatusButtonPendingTitle
        case .inProgress:
            return Strings.Rewards.legacyWalletTransferStatusButtonInProgressTitle
        case .delayed:
            return Strings.Rewards.legacyWalletTransferStatusButtonDelayedTitle
        case .complete:
            return Strings.Rewards.legacyWalletTransferStatusButtonCompletedTitle
        @unknown default:
            return ""
        }
    }
    var transferStatusTitle: String {
        switch self {
        case .invalid:
            return Strings.Rewards.legacyWalletTransferStatusInvalidTitle
        case .pending:
            return Strings.Rewards.legacyWalletTransferStatusPendingTitle
        case .inProgress:
            return Strings.Rewards.legacyWalletTransferStatusInProgressTitle
        case .delayed:
            return Strings.Rewards.legacyWalletTransferStatusDelayedTitle
        case .complete:
            return Strings.Rewards.legacyWalletTransferStatusCompletedTitle
        @unknown default:
            return ""
        }
    }
    var transferStatusBody: String {
        switch self {
        case .invalid:
            return Strings.Rewards.legacyWalletTransferStatusInvalidBody
        case .pending:
            return Strings.Rewards.legacyWalletTransferStatusPendingBody
        case .inProgress:
            return Strings.Rewards.legacyWalletTransferStatusInProgressBody
        case .delayed:
            return Strings.Rewards.legacyWalletTransferStatusDelayedBody
        case .complete:
            return Strings.Rewards.legacyWalletTransferStatusCompletedBody
        @unknown default:
            return ""
        }
    }
}
